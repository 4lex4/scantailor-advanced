/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TextLineSegmenter.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "Grid.h"
#include "VecNT.h"
#include "MinMaxAccumulator.h"
#include "ToLineProjector.h"
#include "LineIntersectionScalar.h"
#include "LineBoundedByRect.h"
#include "imageproc/AffineTransformedImage.h"
#include "imageproc/AffineTransform.h"
#include "imageproc/Grayscale.h"
#include "imageproc/GrayImage.h"
#include "imageproc/GaussBlur.h"
#include "imageproc/RasterOp.h"
#include "imageproc/RasterOpGeneric.h"
#include "imageproc/Constants.h"
#include "imageproc/BWPixelProxy.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/Binarize.h"
#include "imageproc/FindPeaksGeneric.h"
#include "imageproc/Connectivity.h"
#include "imageproc/ConnectivityMap.h"
#include "imageproc/InfluenceMap.h"
#include "imageproc/Morphology.h"
#include "imageproc/MorphGradientDetect.h"
#include "imageproc/SeedFill.h"
#include "imageproc/PolygonRasterizer.h"
#include "imageproc/Constants.h"
#include "imageproc/SkewFinder.h"
#include "imageproc/WienerFilter.h"
#include "imageproc/WatershedSegmentation.h"
#include "imageproc/PlugHoles.h"
#include "imageproc/IntegralImage.h"
#include <boost/dynamic_bitset.hpp>
#include <QPoint>
#include <QPointF>
#include <QSizeF>
#include <QRect>
#include <QRectF>
#include <QPolygonF>
#include <QLineF>
#include <QImage>
#include <QTransform>
#include <QPainter>
#include <Qt>
#include <algorithm>
#include <array>
#include <list>
#include <vector>
#include <deque>
#include <queue>
#include <stack>
#include <limits>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <utility>

using namespace imageproc;

namespace {
    QImage visualizeGrid(Grid<float> const& grid) {
        QSize const size(grid.width(), grid.height());

        MinMaxAccumulator<float> range;
        rasterOpGeneric([&range](float val) {
            range(val);
        }, grid);

        float const scale = 255.f / (range.max() - range.min());
        float const bias = -range.min() * scale;

        GrayImage canvas(size);
        if (!std::isfinite(scale)) {
            canvas.fill(0x00);
        } else {
            rasterOpGeneric(
                    [scale, bias](float val, uint8_t& pixel) {
                        pixel = static_cast<uint8_t>(val * scale + bias);
                    },
                    grid, canvas
            );
        }

        return canvas;
    }
}  // anonymous namespace

namespace dewarping {
    TextLineSegmenter::Result TextLineSegmenter::process(AffineTransformedImage const& image,
                                                         std::shared_ptr<AcceleratableOperations> const& accel_ops,
                                                         TaskStatus const& status,
                                                         DebugImages* dbg) {
        status.throwIfCancelled();

        AffineTransformedImage const downscaled = image.withAdjustedTransform(
                [](AffineImageTransform& xform) {
                    xform.scaleTo(QSizeF(800, 800), Qt::KeepAspectRatio);
                    xform.translateSoThatPointBecomes(
                            xform.transformedCropArea().boundingRect().topLeft(), QPointF(0, 0)
                    );
                }
        );

        QPolygonF const downscaled_crop_area(downscaled.xform().transformedCropArea());
        QRect const downscaled_rect(downscaled_crop_area.boundingRect().toRect());
        assert(downscaled_rect.topLeft() == QPoint(0, 0));

        GrayImage downscaled_image(
                accel_ops->affineTransform(
                        GrayImage(downscaled.origImage()), downscaled.xform().transform(),
                        downscaled_rect, OutsidePixels::assumeWeakNearest()
                )
        );

        if (dbg) {
            dbg->add(downscaled_image, "downscaled");
        }

        Result result = processDownscaled(
                downscaled_image, downscaled_crop_area, accel_ops, status, dbg
        );

        // Transform from downscaled_image to image.origImage() coordinates.
        QTransform const transform(downscaled.xform().transform().inverted());
        for (auto& polyline : result.tracedCurves) {
            for (QPointF& pt : polyline) {
                pt = transform.map(pt);
            }
        }

        return std::move(result);
    } // TextLineSegmenter::process

    TextLineSegmenter::Result TextLineSegmenter::processDownscaled(GrayImage const& image,
                                                                   QPolygonF const& crop_area,
                                                                   std::shared_ptr<AcceleratableOperations> const& accel_ops,
                                                                   TaskStatus const& status,
                                                                   DebugImages* dbg) {
        int const width = image.width();
        int const height = image.height();

        Grid<float> src(width, height,  /*padding=*/ 0);
        rasterOpGeneric(
                [](float& grid_pixel, uint8_t image_pixel) {
                    grid_pixel = static_cast<float>(image_pixel) / 255.f;
                },
                src, image
        );

        status.throwIfCancelled();

        std::vector<Vec2f> const sigmas = {
                { 10.f, 0.5f },
                { 20.f, 0.5f },
                { 10.f, 1.f },
                { 20.f, 1.f },
                { 10.f, 2.f },
                { 20.f, 2.f }
        };

        float const base_angle_rad = findSkewAngleRad(image, status, dbg);

        status.throwIfCancelled();

        std::vector<Vec2f> directions;
        directions.emplace_back(std::cos(base_angle_rad), std::sin(base_angle_rad));
        double const angle_step_deg = 2.0;
        for (double delta_angle_deg = angle_step_deg;
             delta_angle_deg <= 40.0; delta_angle_deg += angle_step_deg) {
            float const angle1_rad = base_angle_rad + delta_angle_deg * constants::DEG2RAD;
            float const angle2_rad = base_angle_rad - delta_angle_deg * constants::DEG2RAD;
            directions.emplace_back(std::cos(angle1_rad), std::sin(angle1_rad));
            directions.emplace_back(std::cos(angle2_rad), std::sin(angle2_rad));
        }

        std::pair<Grid<float>, Grid<uint8_t>> filterbank(
                accel_ops->textFilterBank(src, directions, sigmas, 6.f)
        );
        Grid<float>& blurred = filterbank.first;
        Grid<uint8_t>& direction_idx_map = filterbank.second;

        status.throwIfCancelled();

        Grid<Vec2f> direction_map(width, height);
        {
            IntegralImage<Vec2f> direction_int_image(width, height);
            IntegralImage<float> weight_int_image(width, height);

            float const* blurred_line = blurred.data();
            int const blurred_stride = blurred.stride();
            uint8_t const* direction_idx_line = direction_idx_map.data();
            int const direction_idx_stride = direction_idx_map.stride();

            for (int y = 0; y < height; ++y) {
                direction_int_image.beginRow();
                weight_int_image.beginRow();

                for (int x = 0; x < width; ++x) {
                    Vec2f const direction(directions[direction_idx_line[x]]);
                    float const weight = std::max<float>(0.f, blurred_line[x]);

                    direction_int_image.push(direction * weight);
                    weight_int_image.push(weight);
                }

                blurred_line += blurred_stride;
                direction_idx_line += direction_idx_stride;
            }

            QRect const bounding_rect(0, 0, width, height);
            rasterOpGenericXY(
                    [&direction_int_image, &weight_int_image, bounding_rect](Vec2f& direction, int x, int y) {
                        QRect r(0, 0, 25, 25);
                        r.moveCenter(QPoint(x, y));
                        r &= bounding_rect;
                        float const weight = weight_int_image.sum(r);
                        if (weight > 1e-6) {
                            direction = (direction_int_image.sum(r) / weight).normalized();
                        } else {
                            direction = Vec2f(1.f, 0.f);
                        }
                    },
                    direction_map
            );
        }

        direction_idx_map = Grid<uint8_t>();  // Save memory.

        if (dbg) {
            dbg->addVectorFieldView(visualizeGrid(blurred), direction_map, "filterbank");
        }

        status.throwIfCancelled();

        // Range of values.
        MinMaxAccumulator<float> range;
        rasterOpGeneric([&range](float val) {
            range(val);
        }, blurred);

        status.throwIfCancelled();

        // Normalization to [0, 1] followed by log(1 + x).
        float const scale = 1.f / (range.max() - range.min());
        float const bias = 1.f - range.min() * scale;
        if (!std::isfinite(scale)) {
            return Result { std::list<std::vector<QPointF>>(), direction_map };
        }

        rasterOpGeneric(
                [scale, bias](float& val) {
                    val = std::log(val * scale + bias);
                },
                blurred
        );

        status.throwIfCancelled();

        if (dbg) {
            dbg->add(visualizeGrid(blurred), "log(filterbank)");
        }

        // Find local maximas in log(filterbank)
        BinaryImage peaks = findPeaksGeneric<float>(
                [](float v1, float v2) {
                    return std::max(v1, v2);
                },
                [](float v1, float v2) {
                    return std::min(v1, v2);
                },
                [](float v) {
                    return std::nextafter(v, std::numeric_limits<float>::max());
                },
                QSize(9, 5), std::numeric_limits<float>::max(),
                blurred.data(), blurred.stride(), image.size()
        );

        status.throwIfCancelled();

        if (dbg) {
            QImage canvas(visualizeGrid(blurred).convertToFormat(QImage::Format_ARGB32_Premultiplied));
            {
                QPainter painter(&canvas);
                painter.drawImage(0, 0, dilateBrick(peaks, QSize(3, 3)).toAlphaMask(Qt::blue));
            }
            dbg->add(canvas, "peaks");
        }

        ConnectivityMap cmap(initialSegmentation(image, blurred, peaks.release(), status, dbg));

        status.throwIfCancelled();

        blurred = Grid<float>();  // Save memory.

        Result result;
        result.tracedCurves = refineSegmentation(image, crop_area, cmap, status, dbg);
        result.flowDirectionMap = std::move(direction_map);

        return std::move(result);
    } // TextLineSegmenter::processDownscaled

    double TextLineSegmenter::findSkewAngleRad(GrayImage const& image, TaskStatus const& status, DebugImages* dbg) {
        BinaryImage const binarized(binarizeGatos(image, QSize(9, 9), 3.0));

        status.throwIfCancelled();

        SkewFinder finder;
        Skew const skew(finder.findSkew(binarized));

        double angle_rad = 0;

        if (skew.confidence() >= skew.GOOD_CONFIDENCE) {
            angle_rad = skew.angle() * constants::DEG2RAD;
        }

        if (dbg) {
            QPointF const center(QRectF(image.rect()).center());
            QPointF const vec(std::cos(angle_rad), std::sin(angle_rad));
            QLineF line(center - vec, center + vec);
            lineBoundedByRect(line, QRectF(image.rect()));

            QImage canvas(binarized.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));
            {
                QPainter painter(&canvas);
                painter.setRenderHint(QPainter::Antialiasing);
                QPen pen(QColor(0, 0, 255, 180));
                pen.setWidthF(4.0);
                painter.setPen(pen);
                painter.drawLine(line);
            }

            dbg->add(canvas, "skew");
        }

        return angle_rad;
    } // TextLineSegmenter::findSkewAngleRad

    ConnectivityMap TextLineSegmenter::initialSegmentation(GrayImage const& image,
                                                           Grid<float> const& blurred,
                                                           BinaryImage const& peaks,
                                                           TaskStatus const& status,
                                                           DebugImages* dbg) {
        int const width = image.width();
        int const height = image.height();

        ConnectivityMap cmap(peaks, CONN4);

        struct Region {
            enum LeftRight {
                LEFT = 0,
                RIGHT = 1
            };

            // These are indexed by LEFT or RIGHT.
            std::array<int, 2> x;  // Leftmost and rightmost x coordinate.
            std::array<uint32_t, 2> connections;  // Regions (labels) we run into from left and right.

            Region() {
                x[0] = std::numeric_limits<int>::max();
                x[1] = std::numeric_limits<int>::min();
                connections[0] = 0;
                connections[1] = 0;
            }

            bool empty() const {
                return x[RIGHT] == std::numeric_limits<int>::min();
            }

            /**
             * Negative overlap can be interpreted as a horizontal distance between two regions.
             * Zero overlap means regions touch each other in horizontal direction.
             */
            int overlapLength(Region const& other) const {
                int const this_len = x[RIGHT] + 1 - x[LEFT];
                int const other_len = other.x[RIGHT] + 1 - other.x[LEFT];
                int const combined_len = std::max(x[RIGHT], other.x[RIGHT]) + 1
                                         - std::min(x[LEFT], other.x[LEFT]);
                return this_len + other_len - combined_len;
            }

            /** Absorb point (x, y) into this region. */
            void extendWith(int x, int  /*y*/) {
                this->x[LEFT] = std::min(this->x[LEFT], x);
                this->x[RIGHT] = std::max(this->x[RIGHT], x);
            }

            /**
             * @brief Absorb another region into this region.
             *
             * @param other The region to absorb.
             * @param located The location of @p other relative to *this.
             */
            void extendWith(Region const& other, LeftRight located) {
                x[LEFT] = std::min(x[LEFT], other.x[LEFT]);
                x[RIGHT] = std::max(x[RIGHT], other.x[RIGHT]);
                connections[located] = other.connections[located];
            }
        };

        // Regions are indexed by (cmap_label - 1).
        std::vector<Region> regions(cmap.maxLabel(), Region());

        struct Pos {
            int x, y;
            int dx;  // Left direction: -1, right direction: +1
            uint32_t label;  // (label - 1) indexes into regions vector.
            float quality;  // quality == blurred(x, y)

            bool operator<(Pos const& rhs) const {
                return quality < rhs.quality;
            }
        };

        std::priority_queue<Pos> queue;

        rasterOpGenericXY(
                [&regions, &blurred, &queue](uint32_t& label, int x, int y) {
                    if (label) {
                        Region& region = regions[label - 1];
                        if (region.empty()) {
                            region.extendWith(x, y);
                            float const quality = blurred(x, y);
                            queue.push({ x, y, 1, label, quality });
                            queue.push({ x, y, -1, label, quality });
                        }
                        label = 0;
                    }
                },
                cmap
        );

        status.throwIfCancelled();

        // label_remapper is a union-find tree rather than a flat lookup table.
        // At one point we do flatten it though. Note that label_remapper[0] is
        // guaranteed to be 0.
        std::vector<uint32_t> label_remapper(cmap.maxLabel() + 1);

        // Initialize the identity mapping.
        for (uint32_t label = 0; label <= cmap.maxLabel(); ++label) {
            label_remapper[label] = label;
        }

        // Follows the mappings recursively till it finds an identity mapping.
        auto get_representative = [&label_remapper](uint32_t const label) mutable {
            uint32_t next_label, prev_label = label;
            while ((next_label = label_remapper[prev_label]) != prev_label) {
                prev_label = next_label;
            }
            label_remapper[label] = next_label; // Accelerate future searches.
            return next_label;
        };

        class RegionMerger {
        public:
            RegionMerger(std::function<uint32_t(
                    uint32_t)> get_representative, std::vector<uint32_t>& label_remapper,
                         std::vector<Region>& regions, int max_overlap)
                    : m_getRepresentative(std::move(get_representative)),
                      m_labelRemapper(label_remapper.data()),
                      m_regions(regions.data()),
                      m_labelInConsideration(regions.size() + 1),
                      m_maxOverlap(max_overlap) {
            }

            void addRegionForConsideration(uint32_t const label) {
                assert(label);
                if (!m_labelInConsideration.test(label)) {
                    m_regionsToConsider.push(label);
                    m_labelInConsideration.set(label);
                }
            }

            bool continueMerging() {
                if (m_regionsToConsider.empty()) {
                    return false;
                }

                uint32_t label = m_regionsToConsider.top();
                m_regionsToConsider.pop();
                m_labelInConsideration.reset(label);
                label = m_getRepresentative(label);

                Region& region = m_regions[label - 1];
                for (int conn_idx = 0; conn_idx < 2; ++conn_idx) {
                    uint32_t const connected_label = m_getRepresentative(region.connections[conn_idx]);
                    if (connected_label) {
                        if (connected_label == label) {
                            // Already merged.
                            continue;
                        }

                        Region const& connected_region = m_regions[connected_label - 1];
                        if (m_getRepresentative(connected_region.connections[1 - conn_idx]) == label) {
                            // Mutual connection detected.
                            int const overlap = region.overlapLength(connected_region);
                            if (overlap <= m_maxOverlap) {
                                // Merge connected_region into region.
                                m_labelRemapper[connected_label] = label;
                                region.extendWith(connected_region, (Region::LeftRight) conn_idx);
                                addRegionForConsideration(label);
                            }
                        }
                    }
                }

                return true;
            } // continueMerging

        private:
            std::function<uint32_t(uint32_t)> m_getRepresentative;
            uint32_t* m_labelRemapper;
            Region* m_regions;
            std::stack<uint32_t> m_regionsToConsider;
            boost::dynamic_bitset<> m_labelInConsideration;  // Indexed by label
            int m_maxOverlap;
        };


        RegionMerger merger(get_representative, label_remapper, regions, cmap.size().width() / 10);

        while (!queue.empty()) {
            Pos const pos(queue.top());
            queue.pop();

            uint32_t& label = cmap(pos.x, pos.y);
            if ((label != 0) && (label != pos.label)) {
                // We (src) run over another region (dst).
                uint32_t const src_label = pos.label;
                uint32_t const dst_label = label;
                Region& src = regions[src_label - 1];
                Region const& dst = regions[dst_label - 1];

                // We want: pos.dx == 1  => conn_idx == 1
                // pos.dx == -1 => conn_idx == 0
                int const conn_idx = (pos.dx + 1) >> 1;
                src.connections[conn_idx] = dst_label;
                if (dst.connections[1 - conn_idx] == src_label) {
                    // Mutual connection established.
                    merger.addRegionForConsideration(src_label);
                }
                continue;
            }

            label = pos.label;
            Region& region = regions[pos.label - 1];
            region.extendWith(pos.x, pos.y);

            int const new_x = pos.x + pos.dx;
            if ((new_x < 0) || (new_x >= width)) {
                continue;
            }

            float best_quality = -std::numeric_limits<float>::max();
            int best_y = -1;
            for (int y = pos.y - 1; y <= pos.y + 1; ++y) {
                if ((y < 0) || (y >= height)) {
                    continue;
                }

                float const quality = blurred(new_x, y);
                if (quality >= best_quality) {
                    best_quality = quality;
                    best_y = y;
                }
            }

            if (best_y != -1) {
                queue.push(Pos { new_x, best_y, pos.dx, pos.label, best_quality });
            }
        }

        status.throwIfCancelled();

        while (merger.continueMerging()) {
            // Just continue.
        }

        status.throwIfCancelled();

        if (dbg) {
            QImage canvas(visualizeGrid(blurred).convertToFormat(QImage::Format_ARGB32_Premultiplied));
            {
                QPainter painter(&canvas);
                painter.drawImage(0, 0, cmap.visualized(Qt::transparent));
            }
            dbg->add(canvas, "initial_segmentation");
        }

        status.throwIfCancelled();

        // Re-label the connectivity map.
        rasterOpGeneric(
                [&label_remapper](uint32_t& label) {
                    label = label_remapper[label];
                },
                cmap
        );

        status.throwIfCancelled();

        if (dbg) {
            QImage canvas(visualizeGrid(blurred).convertToFormat(QImage::Format_ARGB32_Premultiplied));
            {
                QPainter painter(&canvas);
                painter.drawImage(0, 0, cmap.visualized(Qt::transparent));
            }
            dbg->add(canvas, "merged");
        }

        makePathsUnique(cmap, blurred);

        if (dbg) {
            QImage canvas(visualizeGrid(blurred).convertToFormat(QImage::Format_ARGB32_Premultiplied));
            {
                QPainter painter(&canvas);
                painter.drawImage(0, 0, cmap.visualized(Qt::transparent));
            }
            dbg->add(canvas, "paths_made_unique");
        }

        return cmap;
    } // TextLineSegmenter::initialSegmentation

    void TextLineSegmenter::makePathsUnique(imageproc::ConnectivityMap& cmap, Grid<float> const& blurred) {
        // cmap may contain regions shaped like this:
        // xxxxxxxxxx
        // x     x
        // xxxxxxxxx
        // This function interprets each region as a graph and finds the best path on it.

        int const width = cmap.size().width();
        int const height = cmap.size().height();

        struct Region {
            QPoint head;
            QPoint tail;
        };

        std::vector<Region> regions(cmap.maxLabel());  // Indexed by label - 1.

        // Populate regions.
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                uint32_t const label = cmap(x, y);
                if (label) {
                    Region& region = regions[label - 1];
                    region.tail = QPoint(x, y);
                    if (region.head.isNull()) {
                        region.head = region.tail;
                    }
                }
            }
        }

        Grid<float> quality_grid(width, height,  /*padding=*/ 0);
        quality_grid.initInterior(-std::numeric_limits<float>::max());

        auto better_quality = [&quality_grid](QPoint lhs, QPoint rhs) {
            return quality_grid(lhs.x(), lhs.y()) > quality_grid(rhs.x(), rhs.y());
        };

        using PQueue = std::priority_queue<QPoint, std::vector<QPoint>, decltype(better_quality)>;
        PQueue pqueue(better_quality);

        // Populate pqueue with head points of regions.
        for (Region const& region : regions) {
            QPoint const pt(region.head);
            if (!pt.isNull()) {
                quality_grid(pt.x(), pt.y()) = blurred(pt.x(), pt.y());
                pqueue.push(pt);
            }
        }

        // Trace the path from tail to head, marking pixels to be preserved.
        auto backtrace = [&cmap, &quality_grid, height](uint32_t const label, QPoint pt) {
            for (;;) {
                // That's our way to mark a pixel to be preserved while keeping its label recoverble.
                cmap(pt.x(), pt.y()) = ~label;

                int const new_x = pt.x() - 1;
                if (new_x < 0) {
                    // Out of bounds.
                    break;
                }

                int new_y = -1;
                float best_quality = -std::numeric_limits<float>::max();
                for (int dy = -1; dy <= 1; ++dy) {
                    int const y = pt.y() + dy;
                    if ((y < 0) || (y >= height)) {
                        // Out of bounds.
                        continue;
                    }

                    if (cmap(new_x, y) != label) {
                        // Different region.
                        continue;
                    }

                    float const quality = quality_grid(new_x, y);
                    if (quality >= best_quality) {
                        best_quality = quality;
                        new_y = y;
                    }
                }

                if (new_y != -1) {
                    pt = QPoint(new_x, new_y);
                } else {
                    break;
                }
            }
        };

        // Below is a simplified version of Dijkstra's algorithm. The simplification is that
        // we never update the priority of a node once it's been added to the priority queue.
        // This simplification was made possible by the fact that our graph is vertex- rather
        // then edge-valued.
        while (!pqueue.empty()) {
            QPoint const pt(pqueue.top());
            pqueue.pop();

            uint32_t const label = cmap(pt.x(), pt.y());
            Region const& region = regions[label - 1];
            if (pt == region.tail) {
                backtrace(label, pt);
                continue;
            }

            int const new_x = pt.x() + 1;
            if (new_x >= width) {
                // Out of bounds.
                continue;
            }

            int new_y = -1;
            float best_quality = -std::numeric_limits<float>::max();
            float const base_quality = quality_grid(pt.x(), pt.y());
            for (int dy = -1; dy <= 1; ++dy) {
                int const y = pt.y() + dy;
                if ((y < 0) || (y >= height)) {
                    // Out of bounds.
                    continue;
                }

                if (cmap(new_x, y) != label) {
                    // Different region.
                    continue;
                }

                if (quality_grid(new_x, y) != -std::numeric_limits<float>::max()) {
                    // This pixel was already processed.
                    continue;
                }

                float const quality = base_quality + blurred(new_x, y);
                if (quality >= best_quality) {
                    best_quality = quality;
                    new_y = y;
                }
            }

            if (new_y != -1) {
                quality_grid(new_x, new_y) = best_quality;
                pqueue.push(QPoint(new_x, new_y));
            }
        }

        rasterOpGeneric(
                [](uint32_t& label) {
                    static uint32_t const msb = uint32_t(1) << 31;
                    if (label & msb) {
                        label = ~label;  // Part of shortest path.
                    } else {
                        label = 0;
                    }
                },
                cmap
        );
    } // TextLineSegmenter::makePathsUnique

    std::list<std::vector<QPointF>>
    TextLineSegmenter::refineSegmentation(GrayImage const& image,
                                          QPolygonF const& crop_area,
                                          ConnectivityMap& cmap,
                                          TaskStatus const& status,
                                          DebugImages* dbg) {
        int const width = image.width();
        int const height = image.height();

        struct Point {
            int x, y;
            float cos;
        };

        std::vector<std::vector<Point>> region_paths(cmap.maxLabel());
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                uint32_t const label = cmap(x, y);
                if (label) {
                    region_paths[label - 1].push_back(Point { x, y, 0.f });
                }
            }
        }

        for (auto& path : region_paths) {
            size_t const path_len = path.size();
            size_t const rib_len = 15;
            for (size_t i = rib_len; i + rib_len < path_len; ++i) {
                size_t const prev_i = i - rib_len;
                size_t const next_i = i + rib_len;
                Vec2f const v1(path[i].x - path[prev_i].x, path[i].y - path[prev_i].y);
                Vec2f const v2(path[next_i].x - path[i].x, path[next_i].y - path[i].y);
                path[i].cos = v1.dot(v2) / std::sqrt(v1.squaredNorm() * v2.squaredNorm());
            }
        }

        status.throwIfCancelled();

        GrayImage seed(openGray(image, QSize(1, 20), 0x00));
        PolygonRasterizer::fillExcept(seed, 0x00, crop_area, Qt::WindingFill);
        memset(seed.data(), 0, seed.stride());
        memset(seed.data() + (seed.height() - 1) * seed.stride(), 0, seed.stride());

        status.throwIfCancelled();

        if (dbg) {
            dbg->add(seed, "seed");
        }

        seedFillGrayInPlace(seed, image, CONN4);

        status.throwIfCancelled();

        if (dbg) {
            dbg->add(seed, "content_removed");
        }

        seed = openGray(seed, QSize(9, 9), 0x00);

        status.throwIfCancelled();

        if (dbg) {
            dbg->add(seed, "content_removed2");
        }

        BinaryImage page_mask(calcPageMask(seed, status, dbg));

        status.throwIfCancelled();

        if (dbg) {
            QImage canvas(image.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));
            {
                QPainter painter(&canvas);
                painter.setOpacity(0.3);
                painter.drawImage(0, 0, page_mask.toAlphaMask(Qt::blue));
            }
            dbg->add(canvas, "page_mask");
        }

        rasterOpGeneric(
                [](uint8_t orig, uint8_t& dst) {
                    if (dst - orig < 1) {
                        dst = 0xff;
                    } else {
                        unsigned const bg = dst;
                        dst = static_cast<uint8_t>(orig * 255u / bg);
                    }
                },
                image, seed
        );

        status.throwIfCancelled();

        if (dbg) {
            dbg->add(seed, "content_only");
        }

        GrayImage vert_comps(openGray(seed, QSize(1, 20), 0x00));

        status.throwIfCancelled();

        if (dbg) {
            dbg->add(vert_comps, "vert_comps");
        }

        rasterOpGeneric(
                [](uint8_t& content, uint8_t vcomps) {
                    content += uint8_t(255) - vcomps;
                },
                seed, vert_comps
        );

        vert_comps = GrayImage();
        status.throwIfCancelled();

        if (dbg) {
            dbg->add(seed, "vert_comps_removed");
        }

        BinaryImage binarized(binarizeGatos(seed, QSize(9, 9), 3.0));
        seed = GrayImage();

        status.throwIfCancelled();

        if (dbg) {
            dbg->add(binarized, "binarized");
        }

        status.throwIfCancelled();

        PolygonRasterizer::fillExcept(binarized, WHITE, crop_area, Qt::WindingFill);
        if (dbg) {
            dbg->add(binarized, "cropped");
        }

        status.throwIfCancelled();

        binarized = dilateBrick(binarized, QSize(1, 3));
        if (dbg) {
            dbg->add(binarized, "dilated");
        }

        status.throwIfCancelled();

        uint32_t const* binarized_data = binarized.data();
        int const binarized_stride = binarized.wordsPerLine();
        auto binarized_get_pixel = [binarized_data, binarized_stride](int x, int y) {
            // Returns uint32_t(1) for black and uint32_t(0) for white.
            return (binarized_data[binarized_stride * y + (x >> 5)] >> (31 - (x & 31)))
                   & uint32_t(1);
        };

        float const PENALTY_COST = 1e6;

        // [label][pixel]
        static float pixel_cost_matrix[3][2] = {
                { 0.f, 4.f },  // label 0
                { 1.f, 0.f },  // label 1
                { 0.f, 4.f },  // label 2
        };

        // [from_label][to_label]
        static float transition_cost_matrix[3][3] = {
                { 0.f,          0.f,          0.f },  // from label 0
                { PENALTY_COST, 0.f,          0.f },  // from label 1
                { PENALTY_COST, PENALTY_COST, 0.f },  // from label 2
        };

        std::list<std::vector<QPointF>> trimmed_lines;

        if (dbg) {
            auto const accessor = cmap.accessor();
            memset(accessor.data, 0, sizeof(*accessor.data) * accessor.stride * accessor.height);
        }

        for (size_t path_idx = 0; path_idx < region_paths.size(); ++path_idx) {
            std::vector<Point> const& path = region_paths[path_idx];
            if (path.empty()) {
                continue;
            }

            struct Node {
                Node* prev;
                int label;
                float cost;
            };

            std::deque<Node> node_storage;

            std::vector<Point>::const_iterator path_it(path.begin());
            std::vector<Point>::const_iterator const path_end(path.end());
            Node* prev_nodes_by_label[3];

            uint32_t pixel = binarized_get_pixel(path_it->x, path_it->y);

            for (int label = 0; label < 3; ++label) {
                float const cost = pixel_cost_matrix[label][pixel] + transition_cost_matrix[0][label];
                node_storage.push_back(Node { nullptr, label, cost });
                prev_nodes_by_label[label] = &node_storage.back();
            }

            while (++path_it != path_end) {
                pixel = binarized_get_pixel(path_it->x, path_it->y);
                Node* nodes_by_label[3];
                for (int label = 0; label < 3; ++label) {
                    float const pixel_cost = pixel_cost_matrix[label][pixel];
                    float bending_cost = 0.f;
                    if ((label == 1) && (path_it->cos < 0.866f)) {
                        bending_cost = PENALTY_COST;
                    }

                    int best_prev_label = 0;
                    float best_cost = std::numeric_limits<float>::max();
                    for (int prev_label = 0; prev_label < 3; ++prev_label) {
                        float const cost = prev_nodes_by_label[prev_label]->cost + pixel_cost
                                           + bending_cost + transition_cost_matrix[prev_label][label];
                        if (cost < best_cost) {
                            best_prev_label = prev_label;
                            best_cost = cost;
                        }
                    }
                    node_storage.push_back(Node { prev_nodes_by_label[best_prev_label], label, best_cost });
                    nodes_by_label[label] = &node_storage.back();
                }
                memcpy(prev_nodes_by_label, nodes_by_label, sizeof(nodes_by_label));
            }

            int best_prev_label = 0;
            float best_cost = std::numeric_limits<float>::max();
            for (int prev_label = 0; prev_label < 3; ++prev_label) {
                float const cost = prev_nodes_by_label[prev_label]->cost
                                   + transition_cost_matrix[prev_label][2];
                if (cost < best_cost) {
                    best_prev_label = prev_label;
                    best_cost = cost;
                }
            }

            // Backtrace.
            std::vector<QPointF> trimmed_path;
            for (Node* node = prev_nodes_by_label[best_prev_label]; node; node = node->prev) {
                --path_it;
                if (node->label == 1) {
                    trimmed_path.emplace_back(qreal(path_it->x), qreal(path_it->y));
                    if (dbg) {
                        cmap(path_it->x, path_it->y) = path_idx + 1;
                    }
                }
            }

            if (trimmed_path.size() > 1) {
                std::reverse(trimmed_path.begin(), trimmed_path.end());
                trimmed_lines.push_back(std::move(trimmed_path));
            }

            status.throwIfCancelled();
        }

        if (dbg) {
            BinaryImage dilated_mask(width, height, WHITE);
            rasterOpGeneric(
                    [](BWPixelProxy bw_pixel, uint32_t cmap_label) {
                        if (cmap_label) {
                            bw_pixel = 1;
                        }
                    },
                    dilated_mask, cmap
            );
            dilated_mask = dilateBrick(dilated_mask, QSize(3, 3));
            InfluenceMap imap(cmap, dilated_mask);
            QImage canvas(binarized.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));
            {
                QPainter painter(&canvas);
                painter.setOpacity(0.7);
                painter.drawImage(0, 0, imap.visualized());
            }
            dbg->add(canvas, "trimmed_lines");
        }

        status.throwIfCancelled();

        maskTextLines(trimmed_lines, image, page_mask, status, dbg);

        return trimmed_lines;
    } // TextLineSegmenter::refineSegmentation

    BinaryImage
    TextLineSegmenter::calcPageMask(GrayImage const& no_content, TaskStatus const& status, DebugImages* dbg) {
        GrayImage grad = morphGradientDetectLightSide(no_content, QSize(7, 7));
        if (dbg) {
            dbg->add(stretchGrayRange(grad), "grad");
        }

        status.throwIfCancelled();

        // We want to find such a threshold of grayscale value that
        // 90% of pixels are valued <= threshold.
        GrayscaleHistogram hist(grad);
        unsigned const total_pixels = grad.width() * grad.height();
        unsigned const want_zero_pixels = total_pixels * 90 / 100;
        unsigned zero_threshold = 0;
        unsigned sum = 0;
        for (unsigned i = 0; i < 256; ++i) {
            sum += hist[i];
            if (sum >= want_zero_pixels) {
                zero_threshold = i;
                break;
            }
        }

        status.throwIfCancelled();

        // Set pixels <= threshold to zero. This will prevent oversegmentation.
        rasterOpGeneric([zero_threshold](uint8_t& px) {
            if (px <= zero_threshold) {
                px = 0;
            }
            // px -= std::min<uint8_t>(px, zero_threshold);
        }, grad);

        status.throwIfCancelled();

        WatershedSegmentation watershed(grad, CONN8);
        if (dbg) {
            dbg->add(watershed.visualized(), "watershed");
        }

        status.throwIfCancelled();

        for (int i = 0; i < 4; ++i) {
            plugHoles(watershed.accessor(), CONN8);
            status.throwIfCancelled();
        }

        if (dbg) {
            dbg->add(watershed.visualized(), "holes_filled");
        }

        struct Region {
            uint64_t sumX;
            uint64_t sumY;
            unsigned count;

            Region()
                    : sumX(),
                      sumY(),
                      count() {
            }
        };

        std::vector<Region> regions(watershed.maxLabel() + 1);
        Region* regs = &regions[0];
        rasterOpGenericXY([regs](uint32_t label, int x, int y) {
            Region& reg = regs[label];
            reg.sumX += x;
            reg.sumY += y;
            reg.count += 1;
        }, watershed);

        status.throwIfCancelled();

        Vec2d const image_center(QRectF(no_content.toQImage().rect()).center());
        double const max_sqdist_from_center = image_center.squaredNorm();

        uint32_t const max_label = watershed.maxLabel();
        uint32_t best_label = 0;
        double best_quality = 0;
        for (uint32_t label = 1; label <= max_label; ++label) {
            Region const& reg = regs[label];
            if (reg.count == 0) {
                continue;
            }

            Vec2d const centroid(double(reg.sumX) / reg.count, double(reg.sumY) / reg.count);
            double const sqdist_from_center = (centroid - image_center).squaredNorm();

            double const center_proximity_factor = std::exp(-sqdist_from_center / max_sqdist_from_center);
            double const size_factor = reg.count;
            double const quality = center_proximity_factor * size_factor;

            if (quality > best_quality) {
                best_label = label;
                best_quality = quality;
            }
        }

        BinaryImage mask(no_content.size(), WHITE);

        if (best_label != 0) {
            rasterOpGeneric(
                    [best_label](BWPixelProxy bit, uint32_t label) {
                        if (label == best_label) {
                            bit = BLACK;
                        }
                    },
                    mask, watershed
            );
        }

        status.throwIfCancelled();

        return mask;
    } // TextLineSegmenter::calcPageMask

    void TextLineSegmenter::maskTextLines(std::list<std::vector<QPointF>>& lines,
                                          imageproc::GrayImage const& image,
                                          imageproc::BinaryImage const& mask,
                                          TaskStatus const&,
                                          DebugImages* dbg) {
        struct MaskChecker {
            uint32_t const* data;
            int stride;

            uint32_t operator()(QPoint const pt) const {
                static uint32_t const msb = uint32_t(1) << 31;
                return data[stride * pt.y() + (pt.x() >> 5)] & (msb >> (pt.x() & 31));
            }
        };

        MaskChecker const checker{ mask.data(), mask.wordsPerLine() };
        std::list<std::vector<QPointF>> rejected_lines;

        auto it = lines.begin();
        auto const end = lines.end();
        while (it != end) {
            size_t ok_points = std::count_if(it->begin(), it->end(), [checker](QPointF const& pt) {
                return checker(pt.toPoint());
            });
            if (ok_points * 2 >= it->size()) {
                // If at least half of line points are masked, we keep the line.
                ++it;
            } else {
                rejected_lines.splice(rejected_lines.end(), lines, it++);
            }
        }

        if (dbg) {
            QImage canvas(image.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));
            {
                QPainter painter(&canvas);
                painter.setOpacity(0.3);
                painter.drawImage(0, 0, mask.toAlphaMask(Qt::blue));
                painter.setOpacity(0.7);

                painter.setRenderHint(QPainter::Antialiasing);

                QPen pen(Qt::green);
                pen.setWidthF(4.0);
                painter.setPen(pen);
                for (auto const& line : lines) {
                    painter.drawPolyline(line.data(), line.size());
                }

                pen.setColor(Qt::red);
                painter.setPen(pen);
                for (auto const& line : rejected_lines) {
                    painter.drawPolyline(line.data(), line.size());
                }
            }
            dbg->add(canvas, "masked_lines");
        }
    } // TextLineSegmenter::maskTextLines
}  // namespace dewarping
