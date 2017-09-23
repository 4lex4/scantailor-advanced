/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "HoughLineDetector.h"
#include "BinaryImage.h"
#include "ConnCompEraser.h"
#include "Constants.h"
#include "Morphology.h"
#include "RasterOp.h"
#include "SeedFill.h"
#include "Grayscale.h"
#include <QPainter>
#include <QDebug>
#include <assert.h>
#include <cmath>

namespace imageproc {
    class HoughLineDetector::GreaterQualityFirst {
    public:
        bool operator()(HoughLine const& lhs, HoughLine const& rhs) const {
            return lhs.quality() > rhs.quality();
        }
    };


    HoughLineDetector::HoughLineDetector(QSize const& input_dimensions,
                                         double const distance_resolution,
                                         double const start_angle,
                                         double const angle_delta,
                                         int const num_angles)
            : m_distanceResolution(distance_resolution),
              m_recipDistanceResolution(1.0 / distance_resolution) {
        int const max_x = input_dimensions.width() - 1;
        int const max_y = input_dimensions.height() - 1;

        QPoint checkpoints[3];
        checkpoints[0] = QPoint(max_x, max_y);
        checkpoints[1] = QPoint(max_x, 0);
        checkpoints[2] = QPoint(0, max_y);

        double max_distance = 0.0;
        double min_distance = 0.0;

        m_angleUnitVectors.reserve(num_angles);
        for (int i = 0; i < num_angles; ++i) {
            double angle = start_angle + angle_delta * i;
            angle *= constants::DEG2RAD;

            QPointF const uv(cos(angle), sin(angle));
            for (QPoint const& p : checkpoints) {
                double const distance = uv.x() * p.x() + uv.y() * p.y();
                max_distance = std::max(max_distance, distance);
                min_distance = std::min(min_distance, distance);
            }

            m_angleUnitVectors.push_back(uv);
        }

        m_distanceBias = -min_distance;

        double const max_biased_distance = max_distance + m_distanceBias;
        int const max_bin = int(
                max_biased_distance * m_recipDistanceResolution + 0.5
        );

        m_histWidth = max_bin + 1;
        m_histHeight = num_angles;
        m_histogram.resize(m_histWidth * m_histHeight, 0);
    }

    void HoughLineDetector::process(int x, int y, unsigned weight) {
        unsigned* hist_line = &m_histogram[0];

        for (QPointF const& uv : m_angleUnitVectors) {
            double const distance = uv.x() * x + uv.y() * y;
            double const biased_distance = distance + m_distanceBias;

            int const bin = (int) (biased_distance * m_recipDistanceResolution + 0.5);
            assert(bin >= 0 && bin < m_histWidth);
            hist_line[bin] += weight;

            hist_line += m_histWidth;
        }
    }

    QImage HoughLineDetector::visualizeHoughSpace(unsigned const lower_bound) const {
        QImage intensity(m_histWidth, m_histHeight, QImage::Format_Indexed8);
        intensity.setColorTable(createGrayscalePalette());
        if ((m_histWidth > 0) && (m_histHeight > 0) && intensity.isNull()) {
            throw std::bad_alloc();
        }

        unsigned max_value = 0;
        unsigned const* hist_line = &m_histogram[0];
        for (int y = 0; y < m_histHeight; ++y) {
            for (int x = 0; x < m_histWidth; ++x) {
                max_value = std::max(max_value, hist_line[x]);
            }
            hist_line += m_histWidth;
        }

        if (max_value == 0) {
            intensity.fill(0);

            return intensity;
        }

        unsigned char* intensity_line = intensity.bits();
        int const intensity_bpl = intensity.bytesPerLine();
        hist_line = &m_histogram[0];
        for (int y = 0; y < m_histHeight; ++y) {
            for (int x = 0; x < m_histWidth; ++x) {
                unsigned const intensity = (unsigned) floor(
                        hist_line[x] * 255.0 / max_value + 0.5
                );
                intensity_line[x] = (unsigned char) intensity;
            }
            intensity_line += intensity_bpl;
            hist_line += m_histWidth;
        }

        BinaryImage const peaks(
                findHistogramPeaks(
                        m_histogram, m_histWidth, m_histHeight, lower_bound
                )
        );

        QImage peaks_visual(intensity.size(), QImage::Format_ARGB32_Premultiplied);
        peaks_visual.fill(qRgb(0xff, 0x00, 0x00));
        QImage alpha_channel(peaks.toQImage());
        if (qGray(alpha_channel.color(0)) < qGray(alpha_channel.color(1))) {
            alpha_channel.setColor(0, qRgb(0x80, 0x80, 0x80));
            alpha_channel.setColor(1, 0);
        } else {
            alpha_channel.setColor(0, 0);
            alpha_channel.setColor(1, qRgb(0x80, 0x80, 0x80));
        }
        peaks_visual.setAlphaChannel(alpha_channel);

        QImage visual(intensity.convertToFormat(QImage::Format_ARGB32_Premultiplied));

        {
            QPainter painter(&visual);
            painter.drawImage(QPoint(0, 0), peaks_visual);
        }

        return visual;
    }      // HoughLineDetector::visualizeHoughSpace

    std::vector<HoughLine>
    HoughLineDetector::findLines(unsigned const quality_lower_bound) const {
        BinaryImage peaks(
                findHistogramPeaks(
                        m_histogram, m_histWidth, m_histHeight,
                        quality_lower_bound
                )
        );

        std::vector<HoughLine> lines;

        QRect const peaks_rect(peaks.rect());
        ConnCompEraser eraser(peaks.release(), CONN8);
        ConnComp cc;
        while (!(cc = eraser.nextConnComp()).isNull()) {
            unsigned const level = m_histogram[
                    cc.seed().y() * m_histWidth + cc.seed().x()
            ];

            QPoint const center(cc.rect().center());
            QPointF const norm_uv(m_angleUnitVectors[center.y()]);
            double const distance = (center.x() + 0.5)
                                    * m_distanceResolution - m_distanceBias;
            lines.push_back(HoughLine(norm_uv, distance, level));
        }

        std::sort(lines.begin(), lines.end(), GreaterQualityFirst());

        return lines;
    }

    BinaryImage HoughLineDetector::findHistogramPeaks(std::vector<unsigned> const& hist,
                                                      int const width,
                                                      int const height,
                                                      unsigned const lower_bound) {
        BinaryImage peak_candidates(
                findPeakCandidates(hist, width, height, lower_bound)
        );

        BinaryImage neighborhood_mask(dilateBrick(peak_candidates, QSize(5, 5)));
        rasterOp<RopXor<RopSrc, RopDst>>(neighborhood_mask, peak_candidates);

        std::vector<unsigned> new_hist(hist);
        incrementBinsMasked(new_hist, width, height, neighborhood_mask);
        neighborhood_mask.release();

        BinaryImage diff(findPeakCandidates(new_hist, width, height, lower_bound));
        rasterOp<RopXor<RopSrc, RopDst>>(diff, peak_candidates);

        BinaryImage const not_peaks(seedFill(diff, peak_candidates, CONN8));

        rasterOp<RopXor<RopSrc, RopDst>>(peak_candidates, not_peaks);

        return peak_candidates;
    }

/**
 * Build a binary image where a black pixel indicates that the corresponding
 * histogram bin meets the following conditions:
 * \li It doesn't have a greater neighbor (in a 5x5 window).
 * \li It's value is not below \p lower_bound.
 */
    BinaryImage HoughLineDetector::findPeakCandidates(std::vector<unsigned> const& hist,
                                                      int const width,
                                                      int const height,
                                                      unsigned const lower_bound) {
        std::vector<unsigned> maxed(hist.size(), 0);

        max5x5(hist, maxed, width, height);

        BinaryImage equal_map(
                buildEqualMap(hist, maxed, width, height, lower_bound)
        );

        return equal_map;
    }

/**
 * Increment bins that correspond to black pixels in \p mask.
 */
    void HoughLineDetector::incrementBinsMasked(std::vector<unsigned>& hist,
                                                int const width,
                                                int const height,
                                                BinaryImage const& mask) {
        uint32_t const* mask_line = mask.data();
        int const mask_wpl = mask.wordsPerLine();
        unsigned* hist_line = &hist[0];
        uint32_t const msb = uint32_t(1) << 31;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (mask_line[x >> 5] & (msb >> (x & 31))) {
                    ++hist_line[x];
                }
            }
            mask_line += mask_wpl;
            hist_line += width;
        }
    }

/**
 * Every bin in \p dst is set to the maximum of the corresponding bin
 * in \p src and its neighbors in a 5x5 window.
 */
    void HoughLineDetector::max5x5(std::vector<unsigned> const& src,
                                   std::vector<unsigned>& dst,
                                   int const width,
                                   int const height) {
        std::vector<unsigned> tmp(src.size(), 0);
        max3x1(src, tmp, width, height);
        max3x1(tmp, dst, width, height);
        max1x3(dst, tmp, width, height);
        max1x3(tmp, dst, width, height);
    }

/**
 * Every bin in \p dst is set to the maximum of the corresponding bin
 * in \p src and its left and right neighbors (if it has them).
 */
    void HoughLineDetector::max3x1(std::vector<unsigned> const& src,
                                   std::vector<unsigned>& dst,
                                   int const width,
                                   int const height) {
        if (width == 1) {
            dst = src;

            return;
        }

        unsigned const* src_line = &src[0];
        unsigned* dst_line = &dst[0];

        for (int y = 0; y < height; ++y) {
            int x = 0;
            dst_line[x] = std::max(src_line[x], src_line[x + 1]);

            for (++x; x < width - 1; ++x) {
                unsigned const prev = src_line[x - 1];
                unsigned const cur = src_line[x];
                unsigned const next = src_line[x + 1];
                dst_line[x] = std::max(prev, std::max(cur, next));
            }

            dst_line[x] = std::max(src_line[x], src_line[x - 1]);

            src_line += width;
            dst_line += width;
        }
    }

/**
 * Every bin in \p dst is set to the maximum of the corresponding bin
 * in \p src and its top and bottom neighbors (if it has them).
 */
    void HoughLineDetector::max1x3(std::vector<unsigned> const& src,
                                   std::vector<unsigned>& dst,
                                   int const width,
                                   int const height) {
        if (height == 1) {
            dst = src;

            return;
        }

        unsigned const* p_src = &src[0];
        unsigned* p_dst = &dst[0];
        for (int x = 0; x < width; ++x) {
            *p_dst = std::max(p_src[0], p_src[width]);
            ++p_src;
            ++p_dst;
        }

        for (int y = 1; y < height - 1; ++y) {
            for (int x = 0; x < width; ++x) {
                unsigned const prev = p_src[x - width];
                unsigned const cur = p_src[x];
                unsigned const next = p_src[x + width];
                p_dst[x] = std::max(prev, std::max(cur, next));
            }

            p_src += width;
            p_dst += width;
        }

        for (int x = 0; x < width; ++x) {
            *p_dst = std::max(p_src[0], p_src[-width]);
            ++p_src;
            ++p_dst;
        }
    }      // HoughLineDetector::max1x3

/**
 * Builds a binary image where a black pixel indicates that the corresponding
 * bins in two histograms have equal values, and those values are not below
 * \p lower_bound.
 */
    BinaryImage HoughLineDetector::buildEqualMap(std::vector<unsigned> const& src1,
                                                 std::vector<unsigned> const& src2,
                                                 int const width,
                                                 int const height,
                                                 unsigned const lower_bound) {
        BinaryImage dst(width, height, WHITE);
        uint32_t* dst_line = dst.data();
        int const dst_wpl = dst.wordsPerLine();
        unsigned const* src1_line = &src1[0];
        unsigned const* src2_line = &src2[0];
        uint32_t const msb = uint32_t(1) << 31;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if ((src1_line[x] >= lower_bound)
                    && (src1_line[x] == src2_line[x])) {
                    dst_line[x >> 5] |= msb >> (x & 31);
                }
            }
            dst_line += dst_wpl;
            src1_line += width;
            src2_line += width;
        }

        return dst;
    }

/*=============================== HoughLine ================================*/

    QPointF HoughLine::pointAtY(double const y) const {
        double x = (m_distance - y * m_normUnitVector.y()) / m_normUnitVector.x();

        return QPointF(x, y);
    }

    QPointF HoughLine::pointAtX(double const x) const {
        double y = (m_distance - x * m_normUnitVector.x()) / m_normUnitVector.y();

        return QPointF(x, y);
    }

    QLineF HoughLine::unitSegment() const {
        QPointF const line_point(m_normUnitVector * m_distance);
        QPointF const line_vector(m_normUnitVector.y(), -m_normUnitVector.x());

        return QLineF(line_point, line_point + line_vector);
    }
}  // namespace imageproc