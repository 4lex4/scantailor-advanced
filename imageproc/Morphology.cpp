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

#include "Morphology.h"
#include <QDebug>
#include <cassert>
#include <cmath>
#include "BinaryImage.h"
#include "GrayImage.h"
#include "Grayscale.h"
#include "RasterOp.h"

namespace imageproc {
Brick::Brick(const QSize& size) {
  const int x_origin = size.width() >> 1;
  const int y_origin = size.height() >> 1;
  m_minX = -x_origin;
  m_minY = -y_origin;
  m_maxX = (size.width() - 1) - x_origin;
  m_maxY = (size.height() - 1) - y_origin;
}

Brick::Brick(const QSize& size, const QPoint& origin) {
  const int x_origin = origin.x();
  const int y_origin = origin.y();
  m_minX = -x_origin;
  m_minY = -y_origin;
  m_maxX = (size.width() - 1) - x_origin;
  m_maxY = (size.height() - 1) - y_origin;
}

Brick::Brick(int min_x, int min_y, int max_x, int max_y) : m_minX(min_x), m_maxX(max_x), m_minY(min_y), m_maxY(max_y) {}

void Brick::flip() {
  const int new_min_x = -m_maxX;
  m_maxX = -m_minX;
  m_minX = new_min_x;
  const int new_min_y = -m_maxY;
  m_maxY = -m_minY;
  m_minY = new_min_y;
}

Brick Brick::flipped() const {
  Brick brick(*this);
  brick.flip();

  return brick;
}

namespace {
class ReusableImages {
 public:
  void store(BinaryImage& img);

  BinaryImage retrieveOrCreate(const QSize& size);

 private:
  std::vector<BinaryImage> m_images;
};


void ReusableImages::store(BinaryImage& img) {
  assert(!img.isNull());

  // Using push_back(null_image) then swap() avoids atomic operations
  // inside BinaryImage.
  m_images.emplace_back();
  m_images.back().swap(img);
}

BinaryImage ReusableImages::retrieveOrCreate(const QSize& size) {
  if (m_images.empty()) {
    return BinaryImage(size);
  } else {
    BinaryImage img;
    m_images.back().swap(img);
    m_images.pop_back();

    return img;
  }
}

class CoordinateSystem {
 public:
  /**
   * \brief Constructs a global coordinate system.
   */
  CoordinateSystem() : m_origin(0, 0) {}

  /**
   * \brief Constructs a coordinate system relative to the global system.
   */
  explicit CoordinateSystem(const QPoint& origin) : m_origin(origin) {}

  const QPoint& origin() const { return m_origin; }

  QRect fromGlobal(const QRect& rect) const { return rect.translated(-m_origin); }

  QRect toGlobal(const QRect& rect) const { return rect.translated(m_origin); }

  QRect mapTo(const QRect& rect, const CoordinateSystem& target_cs) const {
    return rect.translated(m_origin).translated(-target_cs.origin());
  }

  QPoint offsetTo(const CoordinateSystem& target_cs) const { return m_origin - target_cs.origin(); }

 private:
  QPoint m_origin;
};


void adjustToFit(const QRect& fit_into, QRect& fit_and_adjust, QRect& adjust_only) {
  int adj_left = fit_into.left() - fit_and_adjust.left();
  if (adj_left < 0) {
    adj_left = 0;
  }

  int adj_right = fit_into.right() - fit_and_adjust.right();
  if (adj_right > 0) {
    adj_right = 0;
  }

  int adj_top = fit_into.top() - fit_and_adjust.top();
  if (adj_top < 0) {
    adj_top = 0;
  }

  int adj_bottom = fit_into.bottom() - fit_and_adjust.bottom();
  if (adj_bottom > 0) {
    adj_bottom = 0;
  }

  fit_and_adjust.adjust(adj_left, adj_top, adj_right, adj_bottom);
  adjust_only.adjust(adj_left, adj_top, adj_right, adj_bottom);
}

inline QRect extendByBrick(const QRect& rect, const Brick& brick) {
  return rect.adjusted(brick.minX(), brick.minY(), brick.maxX(), brick.maxY());
}

inline QRect shrinkByBrick(const QRect& rect, const Brick& brick) {
  return rect.adjusted(brick.maxX(), brick.maxY(), brick.minX(), brick.minY());
}

const int COMPOSITE_THRESHOLD = 8;

void doInitialCopy(BinaryImage& dst,
                   const CoordinateSystem& dst_cs,
                   const QRect& dst_relevant_rect,
                   const BinaryImage& src,
                   const CoordinateSystem& src_cs,
                   const BWColor initial_color,
                   const int dx,
                   const int dy) {
  QRect src_rect(src.rect());
  QRect dst_rect(src_cs.mapTo(src_rect, dst_cs));
  dst_rect.translate(dx, dy);
  adjustToFit(dst_relevant_rect, dst_rect, src_rect);

  rasterOp<RopSrc>(dst, dst_rect, src, src_rect.topLeft());

  dst.fillFrame(dst_relevant_rect, dst_rect, initial_color);
}

void spreadInto(BinaryImage& dst,
                const CoordinateSystem& dst_cs,
                const QRect& dst_relevant_rect,
                const BinaryImage& src,
                const CoordinateSystem& src_cs,
                const int dx_min,
                const int dx_step,
                const int dy_min,
                const int dy_step,
                const int num_steps,
                const AbstractRasterOp& rop) {
  assert(dx_step == 0 || dy_step == 0);

  int dx = dx_min;
  int dy = dy_min;
  for (int i = 0; i < num_steps; ++i, dx += dx_step, dy += dy_step) {
    QRect src_rect(src.rect());
    QRect dst_rect(src_cs.mapTo(src_rect, dst_cs));
    dst_rect.translate(dx, dy);

    adjustToFit(dst_relevant_rect, dst_rect, src_rect);

    rop(dst, dst_rect, src, src_rect.topLeft());
  }
}

void spreadInDirectionLow(BinaryImage& dst,
                          const CoordinateSystem& dst_cs,
                          const QRect& dst_relevant_rect,
                          const BinaryImage& src,
                          const CoordinateSystem& src_cs,
                          const int dx_min,
                          const int dx_step,
                          const int dy_min,
                          const int dy_step,
                          const int num_steps,
                          const AbstractRasterOp& rop,
                          const BWColor initial_color,
                          const bool dst_composition_allowed) {
  assert(dx_step == 0 || dy_step == 0);

  if (num_steps == 0) {
    return;
  }

  doInitialCopy(dst, dst_cs, dst_relevant_rect, src, src_cs, initial_color, dx_min, dy_min);

  if (num_steps == 1) {
    return;
  }

  int remaining_dx_min = dx_min + dx_step;
  int remaining_dy_min = dy_min + dy_step;
  int remaining_steps = num_steps - 1;

  if (dst_composition_allowed) {
    int dx = dx_step;
    int dy = dy_step;
    int i = 1;
    for (; (i << 1) <= num_steps; i <<= 1, dx <<= 1, dy <<= 1) {
      QRect dst_rect(dst.rect());
      QRect src_rect(dst_rect);
      dst_rect.translate(dx, dy);

      adjustToFit(dst_relevant_rect, dst_rect, src_rect);

      rop(dst, dst_rect, dst, src_rect.topLeft());
    }

    remaining_dx_min = dx_min + dx;
    remaining_dy_min = dy_min + dy;
    remaining_steps = num_steps - i;
  }

  if (remaining_steps > 0) {
    spreadInto(dst, dst_cs, dst_relevant_rect, src, src_cs, remaining_dx_min, dx_step, remaining_dy_min, dy_step,
               remaining_steps, rop);
  }
}  // spreadInDirectionLow

void spreadInDirection(BinaryImage& dst,
                       const CoordinateSystem& dst_cs,
                       const QRect& dst_relevant_rect,
                       const BinaryImage& src,
                       const CoordinateSystem& src_cs,
                       ReusableImages& tmp_images,
                       const CoordinateSystem& tmp_cs,
                       const QSize& tmp_image_size,
                       const int dx_min,
                       const int dx_step,
                       const int dy_min,
                       const int dy_step,
                       const int num_steps,
                       const AbstractRasterOp& rop,
                       const BWColor initial_color,
                       const bool dst_composition_allowed) {
  assert(dx_step == 0 || dy_step == 0);

  if (num_steps < COMPOSITE_THRESHOLD) {
    spreadInDirectionLow(dst, dst_cs, dst_relevant_rect, src, src_cs, dx_min, dx_step, dy_min, dy_step, num_steps, rop,
                         initial_color, dst_composition_allowed);

    return;
  }

  const auto first_phase_steps = (int) std::sqrt((double) num_steps);

  BinaryImage tmp(tmp_images.retrieveOrCreate(tmp_image_size));

  spreadInDirection(tmp, tmp_cs, tmp.rect(), src, src_cs, tmp_images, tmp_cs, tmp_image_size, dx_min, dx_step, dy_min,
                    dy_step, first_phase_steps, rop, initial_color, true);

  const int second_phase_steps = num_steps / first_phase_steps;

  spreadInDirection(dst, dst_cs, dst_relevant_rect, tmp, tmp_cs, tmp_images, tmp_cs, tmp_image_size, 0,
                    dx_step * first_phase_steps, 0, dy_step * first_phase_steps, second_phase_steps, rop, initial_color,
                    dst_composition_allowed);

  const int steps_done = first_phase_steps * second_phase_steps;
  const int steps_remaining = num_steps - steps_done;

  if (steps_remaining <= 0) {
    assert(steps_remaining == 0);
  } else if (steps_remaining < COMPOSITE_THRESHOLD) {
    spreadInto(dst, dst_cs, dst_relevant_rect, src, src_cs, dx_min + dx_step * steps_done, dx_step,
               dy_min + dy_step * steps_done, dy_step, steps_remaining, rop);
  } else {
    spreadInDirection(tmp, tmp_cs, tmp.rect(), src, src_cs, tmp_images, tmp_cs, tmp_image_size,
                      dx_min + dx_step * steps_done, dx_step, dy_min + dy_step * steps_done, dy_step, steps_remaining,
                      rop, initial_color, true);

    spreadInto(dst, dst_cs, dst_relevant_rect, tmp, tmp_cs, 0, 0, 0, 0, 1, rop);
  }

  tmp_images.store(tmp);
}  // spreadInDirection

void dilateOrErodeBrick(BinaryImage& dst,
                        const BinaryImage& src,
                        const Brick& brick,
                        const QRect& dst_area,
                        const BWColor src_surroundings,
                        const AbstractRasterOp& rop,
                        const BWColor spreading_color) {
  assert(!src.isNull());
  assert(!brick.isEmpty());
  assert(!dst_area.isEmpty());

  if (!extendByBrick(src.rect(), brick).intersects(dst_area)) {
    dst.fill(src_surroundings);

    return;
  }

  const CoordinateSystem src_cs;  // global coordinate system
  const CoordinateSystem dst_cs(dst_area.topLeft());
  const QRect dst_image_rect(QPoint(0, 0), dst_area.size());

  // Area in dst coordinates that matters.
  // Everything outside of it will be overwritten.
  QRect dst_relevant_rect(dst_image_rect);

  if (src_surroundings == spreading_color) {
    dst_relevant_rect = dst_cs.fromGlobal(src.rect());
    dst_relevant_rect = shrinkByBrick(dst_relevant_rect, brick);
    dst_relevant_rect = dst_relevant_rect.intersected(dst_image_rect);
    if (dst_relevant_rect.isEmpty()) {
      dst.fill(src_surroundings);

      return;
    }
  }

  const QRect tmp_area(dst_cs.toGlobal(dst_relevant_rect)
                           .adjusted(-(brick.maxX() - brick.minX()), -(brick.maxY() - brick.minY()), 0, 0));

  CoordinateSystem tmp_cs(tmp_area.topLeft());
  const QRect tmp_image_rect(QPoint(0, 0), tmp_area.size());

  // Because all temporary images share the same size, it's easy
  // to reuse them.  Reusing an image not only saves us the
  // cost of memory allocation, but also improves chances that
  // image data is already in CPU cache.
  ReusableImages tmp_images;

  if (brick.minY() == brick.maxY()) {
    spreadInDirection(  // horizontal
        dst, dst_cs, dst_relevant_rect, src, src_cs, tmp_images, tmp_cs, tmp_image_rect.size(), brick.minX(), 1,
        brick.minY(), 0, brick.width(), rop, !spreading_color, false);
  } else if (brick.minX() == brick.maxX()) {
    spreadInDirection(  // vertical
        dst, dst_cs, dst_relevant_rect, src, src_cs, tmp_images, tmp_cs, tmp_image_rect.size(), brick.minX(), 0,
        brick.minY(), 1, brick.height(), rop, !spreading_color, false);
  } else {
    BinaryImage tmp(tmp_area.size());
    spreadInDirection(  // horizontal
        tmp, tmp_cs, tmp_image_rect, src, src_cs, tmp_images, tmp_cs, tmp_image_rect.size(), brick.minX(), 1,
        brick.minY(), 0, brick.width(), rop, !spreading_color, true);

    spreadInDirection(  // vertical
        dst, dst_cs, dst_relevant_rect, tmp, tmp_cs, tmp_images, tmp_cs, tmp_image_rect.size(), 0, 0, 0, 1,
        brick.height(), rop, !spreading_color, false);
  }

  if (src_surroundings == spreading_color) {
    dst.fillExcept(dst_relevant_rect, src_surroundings);
  }
}  // dilateOrErodeBrick

class Darker {
 public:
  static uint8_t select(uint8_t v1, uint8_t v2) { return std::min(v1, v2); }
};


class Lighter {
 public:
  static uint8_t select(uint8_t v1, uint8_t v2) { return std::max(v1, v2); }
};


template <typename MinOrMax>
void fillExtremumArrayLeftHalf(uint8_t* dst,
                               const uint8_t* const src_center,
                               const int src_delta,
                               const int src_first_offset,
                               const int src_center_offset) {
  const uint8_t* src = src_center;
  uint8_t extremum = *src;
  *dst = extremum;

  for (int i = src_center_offset - 1; i >= src_first_offset; --i) {
    src -= src_delta;
    --dst;
    extremum = MinOrMax::select(extremum, *src);
    *dst = extremum;
  }
}

template <typename MinOrMax>
void fillExtremumArrayRightHalf(uint8_t* dst,
                                const uint8_t* const src_center,
                                const int src_delta,
                                const int src_center_offset,
                                const int src_last_offset) {
  const uint8_t* src = src_center;
  uint8_t extremum = *src;
  *dst = extremum;

  for (int i = src_center_offset + 1; i <= src_last_offset; ++i) {
    src += src_delta;
    ++dst;
    extremum = MinOrMax::select(extremum, *src);
    *dst = extremum;
  }
}

template <typename MinOrMax>
void spreadGrayHorizontal(GrayImage& dst, const GrayImage& src, const int dy, const int dx1, const int dx2) {
  const int src_stride = src.stride();
  const int dst_stride = dst.stride();
  const uint8_t* src_line = src.data() + dy * src_stride;
  uint8_t* dst_line = dst.data();

  const int dst_width = dst.width();
  const int dst_height = dst.height();

  const int se_len = dx2 - dx1 + 1;

  std::vector<uint8_t> min_max_array(se_len * 2 - 1, 0);
  uint8_t* const array_center = &min_max_array[se_len - 1];

  for (int y = 0; y < dst_height; ++y) {
    for (int dst_segment_first = 0; dst_segment_first < dst_width; dst_segment_first += se_len) {
      const int dst_segment_last = std::min(dst_segment_first + se_len, dst_width) - 1;  // inclusive
      const int src_segment_first = dst_segment_first + dx1;
      const int src_segment_last = dst_segment_last + dx2;
      const int src_segment_center = (src_segment_first + src_segment_last) >> 1;

      fillExtremumArrayLeftHalf<MinOrMax>(array_center, src_line + src_segment_center, 1, src_segment_first,
                                          src_segment_center);

      fillExtremumArrayRightHalf<MinOrMax>(array_center, src_line + src_segment_center, 1, src_segment_center,
                                           src_segment_last);

      for (int x = dst_segment_first; x <= dst_segment_last; ++x) {
        const int src_first = x + dx1;
        const int src_last = x + dx2;  // inclusive
        assert(src_segment_center >= src_first);
        assert(src_segment_center <= src_last);
        uint8_t v1 = array_center[src_first - src_segment_center];
        uint8_t v2 = array_center[src_last - src_segment_center];
        dst_line[x] = MinOrMax::select(v1, v2);
      }
    }

    src_line += src_stride;
    dst_line += dst_stride;
  }
}  // spreadGrayHorizontal

template <typename MinOrMax>
void spreadGrayHorizontal(GrayImage& dst,
                          const CoordinateSystem& dst_cs,
                          const GrayImage& src,
                          const CoordinateSystem& src_cs,
                          const int dy,
                          const int dx1,
                          const int dx2) {
  // src_point = dst_point + dst_to_src;
  const QPoint dst_to_src(dst_cs.offsetTo(src_cs));

  spreadGrayHorizontal<MinOrMax>(dst, src, dy + dst_to_src.y(), dx1 + dst_to_src.x(), dx2 + dst_to_src.x());
}

template <typename MinOrMax>
void spreadGrayVertical(GrayImage& dst, const GrayImage& src, const int dx, const int dy1, const int dy2) {
  const int src_stride = src.stride();
  const int dst_stride = dst.stride();
  const uint8_t* const src_data = src.data() + dx;
  uint8_t* const dst_data = dst.data();

  const int dst_width = dst.width();
  const int dst_height = dst.height();

  const int se_len = dy2 - dy1 + 1;

  std::vector<uint8_t> min_max_array(se_len * 2 - 1, 0);
  uint8_t* const array_center = &min_max_array[se_len - 1];

  for (int x = 0; x < dst_width; ++x) {
    for (int dst_segment_first = 0; dst_segment_first < dst_height; dst_segment_first += se_len) {
      const int dst_segment_last = std::min(dst_segment_first + se_len, dst_height) - 1;  // inclusive
      const int src_segment_first = dst_segment_first + dy1;
      const int src_segment_last = dst_segment_last + dy2;
      const int src_segment_center = (src_segment_first + src_segment_last) >> 1;

      fillExtremumArrayLeftHalf<MinOrMax>(array_center, src_data + x + src_segment_center * src_stride, src_stride,
                                          src_segment_first, src_segment_center);

      fillExtremumArrayRightHalf<MinOrMax>(array_center, src_data + x + src_segment_center * src_stride, src_stride,
                                           src_segment_center, src_segment_last);

      uint8_t* dst = dst_data + x + dst_segment_first * dst_stride;
      for (int y = dst_segment_first; y <= dst_segment_last; ++y) {
        const int src_first = y + dy1;
        const int src_last = y + dy2;  // inclusive
        assert(src_segment_center >= src_first);
        assert(src_segment_center <= src_last);
        uint8_t v1 = array_center[src_first - src_segment_center];
        uint8_t v2 = array_center[src_last - src_segment_center];
        *dst = MinOrMax::select(v1, v2);
        dst += dst_stride;
      }
    }
  }
}  // spreadGrayVertical

template <typename MinOrMax>
void spreadGrayVertical(GrayImage& dst,
                        const CoordinateSystem& dst_cs,
                        const GrayImage& src,
                        const CoordinateSystem& src_cs,
                        const int dx,
                        const int dy1,
                        const int dy2) {
  // src_point = dst_point + dst_to_src;
  const QPoint dst_to_src(dst_cs.offsetTo(src_cs));

  spreadGrayVertical<MinOrMax>(dst, src, dx + dst_to_src.x(), dy1 + dst_to_src.y(), dy2 + dst_to_src.y());
}

GrayImage extendGrayImage(const GrayImage& src, const QRect& dst_area, const uint8_t background) {
  GrayImage dst(dst_area.size());

  const CoordinateSystem dst_cs(dst_area.topLeft());
  const QRect src_rect_in_dst_cs(dst_cs.fromGlobal(src.rect()));
  const QRect bound_src_rect_in_dst_cs(src_rect_in_dst_cs.intersected(dst.rect()));

  if (bound_src_rect_in_dst_cs.isEmpty()) {
    dst.fill(background);

    return dst;
  }

  const uint8_t* src_line = src.data();
  uint8_t* dst_line = dst.data();
  const int src_stride = src.stride();
  const int dst_stride = dst.stride();

  int y = 0;
  for (; y < bound_src_rect_in_dst_cs.top(); ++y, dst_line += dst_stride) {
    memset(dst_line, background, dst_stride);
  }

  const int front_span_len = bound_src_rect_in_dst_cs.left();
  const int data_span_len = bound_src_rect_in_dst_cs.width();
  const int back_span_offset = front_span_len + data_span_len;
  const int back_span_len = dst_area.width() - back_span_offset;

  const QPoint src_offset(bound_src_rect_in_dst_cs.topLeft() - src_rect_in_dst_cs.topLeft());

  src_line += src_offset.x() + src_offset.y() * src_stride;
  for (; y <= bound_src_rect_in_dst_cs.bottom(); ++y) {
    memset(dst_line, background, front_span_len);
    memcpy(dst_line + front_span_len, src_line, data_span_len);
    memset(dst_line + back_span_offset, background, back_span_len);

    src_line += src_stride;
    dst_line += dst_stride;
  }

  const int height = dst_area.height();
  for (; y < height; ++y, dst_line += dst_stride) {
    memset(dst_line, background, dst_stride);
  }

  return dst;
}  // extendGrayImage

template <typename MinOrMax>
GrayImage dilateOrErodeGray(const GrayImage& src,
                            const Brick& brick,
                            const QRect& dst_area,
                            const unsigned char src_surroundings) {
  assert(!src.isNull());
  assert(!brick.isEmpty());
  assert(!dst_area.isEmpty());

  GrayImage dst(dst_area.size());

  if (!extendByBrick(src.rect(), brick).intersects(dst_area)) {
    dst.fill(src_surroundings);

    return dst;
  }
  const CoordinateSystem dst_cs(dst_area.topLeft());

  // Each pixel will be a minumum or maximum of a group of pixels
  // in its neighborhood.  The neighborhood is defined by collect_area.
  const Brick collect_area(brick.flipped());

  if ((collect_area.minY() != collect_area.maxY()) && (collect_area.minX() != collect_area.maxX())) {
    // We are going to make two operations:
    // src -> tmp, then tmp -> dst
    // Those operations will use the following collect areas:
    const Brick collect_area1(collect_area.minX(), collect_area.minY(), collect_area.maxX(), collect_area.minY());
    const Brick collect_area2(0, 0, 0, collect_area.maxY() - collect_area.minY());

    const QRect tmp_rect(extendByBrick(dst_area, collect_area2));
    CoordinateSystem tmp_cs(tmp_rect.topLeft());

    GrayImage tmp(tmp_rect.size());
    // First operation.  The scope is there to destroy the
    // effective_src image when it's no longer necessary.
    {
      const QRect effective_src_rect(extendByBrick(tmp_rect, collect_area1));
      GrayImage effective_src;
      CoordinateSystem effective_src_cs;
      if (src.rect().contains(effective_src_rect)) {
        effective_src = src;
      } else {
        effective_src = extendGrayImage(src, effective_src_rect, src_surroundings);
        effective_src_cs = CoordinateSystem(effective_src_rect.topLeft());
      }

      spreadGrayHorizontal<MinOrMax>(tmp, tmp_cs, effective_src, effective_src_cs, collect_area1.minY(),
                                     collect_area1.minX(), collect_area1.maxX());
    }
    // Second operation.
    spreadGrayVertical<MinOrMax>(dst, dst_cs, tmp, tmp_cs, collect_area2.minX(), collect_area2.minY(),
                                 collect_area2.maxY());
  } else {
    const QRect effective_src_rect(extendByBrick(dst_area, collect_area));
    GrayImage effective_src;
    CoordinateSystem effective_src_cs;
    if (src.rect().contains(effective_src_rect)) {
      effective_src = src;
    } else {
      effective_src = extendGrayImage(src, effective_src_rect, src_surroundings);
      effective_src_cs = CoordinateSystem(effective_src_rect.topLeft());
    }

    if (collect_area.minY() == collect_area.maxY()) {
      spreadGrayHorizontal<MinOrMax>(dst, dst_cs, effective_src, effective_src_cs, collect_area.minY(),
                                     collect_area.minX(), collect_area.maxX());
    } else {
      assert(collect_area.minX() == collect_area.maxX());
      spreadGrayVertical<MinOrMax>(dst, dst_cs, effective_src, effective_src_cs, collect_area.minX(),
                                   collect_area.minY(), collect_area.maxY());
    }
  }

  return dst;
}  // dilateOrErodeGray
}  // anonymous namespace

BinaryImage dilateBrick(const BinaryImage& src,
                        const Brick& brick,
                        const QRect& dst_area,
                        const BWColor src_surroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("dilateBrick: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("dilateBrick: brick is empty");
  }
  if (dst_area.isEmpty()) {
    throw std::invalid_argument("dilateBrick: dst_area is empty");
  }

  TemplateRasterOp<RopOr<RopSrc, RopDst>> rop;
  BinaryImage dst(dst_area.size());
  dilateOrErodeBrick(dst, src, brick, dst_area, src_surroundings, rop, BLACK);

  return dst;
}

GrayImage dilateGray(const GrayImage& src,
                     const Brick& brick,
                     const QRect& dst_area,
                     const unsigned char src_surroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("dilateGray: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("dilateGray: brick is empty");
  }
  if (dst_area.isEmpty()) {
    throw std::invalid_argument("dilateGray: dst_area is empty");
  }

  return dilateOrErodeGray<Darker>(src, brick, dst_area, src_surroundings);
}

BinaryImage dilateBrick(const BinaryImage& src, const Brick& brick, const BWColor src_surroundings) {
  return dilateBrick(src, brick, src.rect(), src_surroundings);
}

GrayImage dilateGray(const GrayImage& src, const Brick& brick, const unsigned char src_surroundings) {
  return dilateGray(src, brick, src.rect(), src_surroundings);
}

BinaryImage erodeBrick(const BinaryImage& src,
                       const Brick& brick,
                       const QRect& dst_area,
                       const BWColor src_surroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("erodeBrick: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("erodeBrick: brick is empty");
  }
  if (dst_area.isEmpty()) {
    throw std::invalid_argument("erodeBrick: dst_area is empty");
  }

  typedef RopAnd<RopSrc, RopDst> Rop;

  TemplateRasterOp<RopAnd<RopSrc, RopDst>> rop;
  BinaryImage dst(dst_area.size());
  dilateOrErodeBrick(dst, src, brick, dst_area, src_surroundings, rop, WHITE);

  return dst;
}

GrayImage erodeGray(const GrayImage& src,
                    const Brick& brick,
                    const QRect& dst_area,
                    const unsigned char src_surroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("erodeGray: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("erodeGray: brick is empty");
  }
  if (dst_area.isEmpty()) {
    throw std::invalid_argument("erodeGray: dst_area is empty");
  }

  return dilateOrErodeGray<Lighter>(src, brick, dst_area, src_surroundings);
}

BinaryImage erodeBrick(const BinaryImage& src, const Brick& brick, const BWColor src_surroundings) {
  return erodeBrick(src, brick, src.rect(), src_surroundings);
}

GrayImage erodeGray(const GrayImage& src, const Brick& brick, const unsigned char src_surroundings) {
  return erodeGray(src, brick, src.rect(), src_surroundings);
}

BinaryImage openBrick(const BinaryImage& src,
                      const QSize& brick,
                      const QRect& dst_area,
                      const BWColor src_surroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("openBrick: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("openBrick: brick is empty");
  }

  Brick actual_brick(brick);

  QRect tmp_area;

  if (src_surroundings == WHITE) {
    tmp_area = shrinkByBrick(src.rect(), actual_brick);
    if (tmp_area.isEmpty()) {
      return BinaryImage(dst_area.size(), WHITE);
    }
  } else {
    tmp_area = extendByBrick(src.rect(), actual_brick);
  }

  // At this point we could leave tmp_area as is, but a large
  // tmp_area would be a waste if dst_area is small.

  tmp_area = extendByBrick(dst_area, actual_brick).intersected(tmp_area);

  CoordinateSystem tmp_cs(tmp_area.topLeft());

  const BinaryImage tmp(erodeBrick(src, actual_brick, tmp_area, src_surroundings));
  actual_brick.flip();

  return dilateBrick(tmp, actual_brick, tmp_cs.fromGlobal(dst_area), src_surroundings);
}  // openBrick

BinaryImage openBrick(const BinaryImage& src, const QSize& brick, const BWColor src_surroundings) {
  return openBrick(src, brick, src.rect(), src_surroundings);
}

GrayImage openGray(const GrayImage& src,
                   const QSize& brick,
                   const QRect& dst_area,
                   const unsigned char src_surroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("openGray: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("openGray: brick is empty");
  }
  if (dst_area.isEmpty()) {
    throw std::invalid_argument("openGray: dst_area is empty");
  }

  const Brick brick1(brick);
  const Brick brick2(brick1.flipped());

  // We are going to make two operations:
  // tmp = erodeGray(src, brick1), then dst = dilateGray(tmp, brick2)
  const QRect tmp_rect(extendByBrick(dst_area, brick1));
  CoordinateSystem tmp_cs(tmp_rect.topLeft());

  const GrayImage tmp(dilateOrErodeGray<Lighter>(src, brick1, tmp_rect, src_surroundings));

  return dilateOrErodeGray<Darker>(tmp, brick2, tmp_cs.fromGlobal(dst_area), src_surroundings);
}

GrayImage openGray(const GrayImage& src, const QSize& brick, const unsigned char src_surroundings) {
  return openGray(src, brick, src.rect(), src_surroundings);
}

BinaryImage closeBrick(const BinaryImage& src,
                       const QSize& brick,
                       const QRect& dst_area,
                       const BWColor src_surroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("closeBrick: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("closeBrick: brick is empty");
  }

  Brick actual_brick(brick);

  QRect tmp_area;

  if (src_surroundings == BLACK) {
    tmp_area = shrinkByBrick(src.rect(), actual_brick);
    if (tmp_area.isEmpty()) {
      return BinaryImage(dst_area.size(), BLACK);
    }
  } else {
    tmp_area = extendByBrick(src.rect(), actual_brick);
  }

  // At this point we could leave tmp_area as is, but a large
  // tmp_area would be a waste if dst_area is small.

  tmp_area = extendByBrick(dst_area, actual_brick).intersected(tmp_area);

  CoordinateSystem tmp_cs(tmp_area.topLeft());

  const BinaryImage tmp(dilateBrick(src, actual_brick, tmp_area, src_surroundings));
  actual_brick.flip();

  return erodeBrick(tmp, actual_brick, tmp_cs.fromGlobal(dst_area), src_surroundings);
}  // closeBrick

BinaryImage closeBrick(const BinaryImage& src, const QSize& brick, const BWColor src_surroundings) {
  return closeBrick(src, brick, src.rect(), src_surroundings);
}

GrayImage closeGray(const GrayImage& src,
                    const QSize& brick,
                    const QRect& dst_area,
                    const unsigned char src_surroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("closeGray: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("closeGray: brick is empty");
  }
  if (dst_area.isEmpty()) {
    throw std::invalid_argument("closeGray: dst_area is empty");
  }

  const Brick brick1(brick);
  const Brick brick2(brick1.flipped());

  // We are going to make two operations:
  // tmp = dilateGray(src, brick1), then dst = erodeGray(tmp, brick2)
  const QRect tmp_rect(extendByBrick(dst_area, brick2));
  CoordinateSystem tmp_cs(tmp_rect.topLeft());

  const GrayImage tmp(dilateOrErodeGray<Darker>(src, brick1, tmp_rect, src_surroundings));

  return dilateOrErodeGray<Lighter>(tmp, brick2, tmp_cs.fromGlobal(dst_area), src_surroundings);
}

GrayImage closeGray(const GrayImage& src, const QSize& brick, const unsigned char src_surroundings) {
  return closeGray(src, brick, src.rect(), src_surroundings);
}

BinaryImage hitMissMatch(const BinaryImage& src,
                         const BWColor src_surroundings,
                         const std::vector<QPoint>& hits,
                         const std::vector<QPoint>& misses) {
  if (src.isNull()) {
    return BinaryImage();
  }

  const QRect rect(src.rect());  // same as dst.rect()
  BinaryImage dst(src.size());

  bool first = true;

  for (const QPoint& hit : hits) {
    QRect src_rect(rect);
    QRect dst_rect(rect.translated(-hit));
    adjustToFit(rect, dst_rect, src_rect);

    if (first) {
      first = false;
      rasterOp<RopSrc>(dst, dst_rect, src, src_rect.topLeft());
      if (src_surroundings == BLACK) {
        dst.fillExcept(dst_rect, BLACK);
      }
    } else {
      rasterOp<RopAnd<RopSrc, RopDst>>(dst, dst_rect, src, src_rect.topLeft());
    }

    if (src_surroundings == WHITE) {
      // No hits on white surroundings.
      dst.fillExcept(dst_rect, WHITE);
    }
  }

  for (const QPoint& miss : misses) {
    QRect src_rect(rect);
    QRect dst_rect(rect.translated(-miss));
    adjustToFit(rect, dst_rect, src_rect);

    if (first) {
      first = false;
      rasterOp<RopNot<RopSrc>>(dst, dst_rect, src, src_rect.topLeft());
      if (src_surroundings == WHITE) {
        dst.fillExcept(dst_rect, BLACK);
      }
    } else {
      rasterOp<RopAnd<RopNot<RopSrc>, RopDst>>(dst, dst_rect, src, src_rect.topLeft());
    }

    if (src_surroundings == BLACK) {
      // No misses on black surroundings.
      dst.fillExcept(dst_rect, WHITE);
    }
  }

  if (first) {
    dst.fill(WHITE);  // No matches.
  }

  return dst;
}  // hitMissMatch

BinaryImage hitMissMatch(const BinaryImage& src,
                         const BWColor src_surroundings,
                         const char* const pattern,
                         const int pattern_width,
                         const int pattern_height,
                         const QPoint& pattern_origin) {
  std::vector<QPoint> hits;
  std::vector<QPoint> misses;

  const char* p = pattern;
  for (int y = 0; y < pattern_height; ++y) {
    for (int x = 0; x < pattern_width; ++x, ++p) {
      switch (*p) {
        case 'X':
          hits.push_back(QPoint(x, y) - pattern_origin);
          break;
        case ' ':
          misses.push_back(QPoint(x, y) - pattern_origin);
          break;
        case '?':
          break;
        default:
          throw std::invalid_argument("hitMissMatch: invalid character in pattern");
      }
    }
  }

  return hitMissMatch(src, src_surroundings, hits, misses);
}

BinaryImage hitMissReplace(const BinaryImage& src,
                           const BWColor src_surroundings,
                           const char* const pattern,
                           const int pattern_width,
                           const int pattern_height) {
  BinaryImage dst(src);

  hitMissReplaceInPlace(dst, src_surroundings, pattern, pattern_width, pattern_height);

  return dst;
}

void hitMissReplaceInPlace(BinaryImage& img,
                           const BWColor src_surroundings,
                           const char* const pattern,
                           const int pattern_width,
                           const int pattern_height) {
  // It's better to have the origin at one of the replacement positions.
  // Otherwise we may miss a partially outside-of-image match because
  // the origin point was outside of the image as well.
  const int pattern_len = pattern_width * pattern_height;
  const auto* const minus_pos = (const char*) memchr(pattern, '-', pattern_len);
  const auto* const plus_pos = (const char*) memchr(pattern, '+', pattern_len);
  const char* origin_pos;
  if (minus_pos && plus_pos) {
    origin_pos = std::min(minus_pos, plus_pos);
  } else if (minus_pos) {
    origin_pos = minus_pos;
  } else if (plus_pos) {
    origin_pos = plus_pos;
  } else {
    // No replacements requested - nothing to do.
    return;
  }

  const QPoint origin(static_cast<int>((origin_pos - pattern) % pattern_width),
                      static_cast<int>((origin_pos - pattern) / pattern_width));

  std::vector<QPoint> hits;
  std::vector<QPoint> misses;
  std::vector<QPoint> white_to_black;
  std::vector<QPoint> black_to_white;

  const char* p = pattern;
  for (int y = 0; y < pattern_height; ++y) {
    for (int x = 0; x < pattern_width; ++x, ++p) {
      switch (*p) {
        case '-':
          black_to_white.push_back(QPoint(x, y) - origin);
          // fall through
        case 'X':
          hits.push_back(QPoint(x, y) - origin);
          break;
        case '+':
          white_to_black.push_back(QPoint(x, y) - origin);
          // fall through
        case ' ':
          misses.push_back(QPoint(x, y) - origin);
          break;
        case '?':
          break;
        default:
          throw std::invalid_argument("hitMissReplace: invalid character in pattern");
      }
    }
  }

  const BinaryImage matches(hitMissMatch(img, src_surroundings, hits, misses));
  const QRect rect(img.rect());

  for (const QPoint& offset : white_to_black) {
    QRect src_rect(rect);
    QRect dst_rect(rect.translated(offset));
    adjustToFit(rect, dst_rect, src_rect);

    rasterOp<RopOr<RopSrc, RopDst>>(img, dst_rect, matches, src_rect.topLeft());
  }

  for (const QPoint& offset : black_to_white) {
    QRect src_rect(rect);
    QRect dst_rect(rect.translated(offset));
    adjustToFit(rect, dst_rect, src_rect);

    rasterOp<RopSubtract<RopDst, RopSrc>>(img, dst_rect, matches, src_rect.topLeft());
  }
}

BinaryImage whiteTopHatTransform(const BinaryImage& src,
                                 const QSize& brick,
                                 const QRect& dst_area,
                                 BWColor src_surroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("whiteTopHatTransform: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("whiteTopHatTransform: brick is empty");
  }
  if (dst_area.isEmpty()) {
    throw std::invalid_argument("whiteTopHatTransform: dst_area is empty");
  }

  BinaryImage dst(dst_area.size());
  if (dst_area != src.rect()) {
    rasterOp<RopSrc>(dst, dst.rect(), src, dst_area.topLeft());
  } else {
    dst = src;
  }

  rasterOp<RopSubtract<RopDst, RopSrc>>(dst, openBrick(src, brick, dst_area, src_surroundings));

  return dst;
}

BinaryImage whiteTopHatTransform(const BinaryImage& src, const QSize& brick, BWColor src_surroundings) {
  return whiteTopHatTransform(src, brick, src.rect(), src_surroundings);
}

BinaryImage blackTopHatTransform(const BinaryImage& src,
                                 const QSize& brick,
                                 const QRect& dst_area,
                                 BWColor src_surroundings) {
  if (src.isNull()) {
    throw std::invalid_argument("blackTopHatTransform: src image is null");
  }
  if (brick.isEmpty()) {
    throw std::invalid_argument("blackTopHatTransform: brick is empty");
  }
  if (dst_area.isEmpty()) {
    throw std::invalid_argument("blackTopHatTransform: dst_area is empty");
  }

  BinaryImage dst(dst_area.size());
  if (dst_area != src.rect()) {
    rasterOp<RopSrc>(dst, dst.rect(), src, dst_area.topLeft());
  } else {
    dst = src;
  }

  rasterOp<RopSubtract<RopSrc, RopDst>>(dst, closeBrick(src, brick, dst_area, src_surroundings));

  return dst;
}

BinaryImage blackTopHatTransform(const BinaryImage& src, const QSize& brick, BWColor src_surroundings) {
  return blackTopHatTransform(src, brick, src.rect(), src_surroundings);
}
}  // namespace imageproc