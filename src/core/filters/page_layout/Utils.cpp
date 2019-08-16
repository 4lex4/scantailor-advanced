// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Utils.h"
#include <UnitsConverter.h>
#include <cassert>
#include <cmath>
#include "Alignment.h"
#include "ImageTransformation.h"
#include "Margins.h"
#include "Params.h"

namespace page_layout {
QRectF Utils::adaptContentRect(const ImageTransformation& xform, const QRectF& content_rect) {
  if (!content_rect.isEmpty()) {
    return content_rect;
  }

  const QPointF center(xform.resultingRect().center());
  const QPointF delta(0.01, 0.01);

  return QRectF(center - delta, center + delta);
}

QSizeF Utils::calcRectSizeMM(const ImageTransformation& xform, const QRectF& rect) {
  const QTransform virt_to_mm(xform.transformBack() * UnitsConverter(xform.origDpi()).transform(PIXELS, MILLIMETRES));

  const QLineF hor_line(rect.topLeft(), rect.topRight());
  const QLineF ver_line(rect.topLeft(), rect.bottomLeft());

  const double width = virt_to_mm.map(hor_line).length();
  const double height = virt_to_mm.map(ver_line).length();

  return QSizeF(width, height);
}

void Utils::extendPolyRectWithMargins(QPolygonF& poly_rect, const Margins& margins) {
  const QPointF down_uv(getDownUnitVector(poly_rect));
  const QPointF right_uv(getRightUnitVector(poly_rect));

  // top-left
  poly_rect[0] -= down_uv * margins.top();
  poly_rect[0] -= right_uv * margins.left();

  // top-right
  poly_rect[1] -= down_uv * margins.top();
  poly_rect[1] += right_uv * margins.right();

  // bottom-right
  poly_rect[2] += down_uv * margins.bottom();
  poly_rect[2] += right_uv * margins.right();

  // bottom-left
  poly_rect[3] += down_uv * margins.bottom();
  poly_rect[3] -= right_uv * margins.left();

  if (poly_rect.size() > 4) {
    assert(poly_rect.size() == 5);
    // This polygon is closed.
    poly_rect[4] = poly_rect[3];
  }
}

Margins Utils::calcMarginsMM(const ImageTransformation& xform, const QRectF& page_rect, const QRectF& content_rect) {
  const QSizeF content_size_mm(Utils::calcRectSizeMM(xform, content_rect));

  const QSizeF page_size_mm(Utils::calcRectSizeMM(xform, page_rect));

  double widthMM = page_size_mm.width() - content_size_mm.width();
  double heightMM = page_size_mm.height() - content_size_mm.height();

  auto left = double(content_rect.left() - page_rect.left());
  auto right = double(page_rect.right() - content_rect.right());
  auto top = double(content_rect.top() - page_rect.top());
  auto bottom = double(page_rect.bottom() - content_rect.bottom());
  double h_space = left + right;
  double v_space = top + bottom;

  double lMM = (h_space < 1.0) ? 0.0 : (left * widthMM / h_space);
  double rMM = (h_space < 1.0) ? 0.0 : (right * widthMM / h_space);
  double tMM = (v_space < 1.0) ? 0.0 : (top * heightMM / v_space);
  double bMM = (v_space < 1.0) ? 0.0 : (bottom * heightMM / v_space);

  return Margins(lMM, tMM, rMM, bMM);
}

Margins Utils::calcSoftMarginsMM(const QSizeF& hard_size_mm,
                                 const QSizeF& aggregate_hard_size_mm,
                                 const Alignment& alignment,
                                 const QRectF& content_rect,
                                 const QSizeF& content_size_mm,
                                 const QRectF& page_rect) {
  if (alignment.isNull()) {
    // This means we are not aligning this page with others.
    return Margins();
  }

  double top = 0.0;
  double bottom = 0.0;
  double left = 0.0;
  double right = 0.0;

  const double delta_width = aggregate_hard_size_mm.width() - hard_size_mm.width();
  const double delta_height = aggregate_hard_size_mm.height() - hard_size_mm.height();

  double agg_left_border = 0.0;
  double agg_right_border = delta_width;
  double agg_top_border = 0.0;
  double agg_bottom_border = delta_height;

  Alignment corrected_alignment = alignment;

  if (!content_size_mm.isEmpty() && !content_rect.isEmpty() && !page_rect.isEmpty()) {
    const double pixels_per_mm_horizontal = content_rect.width() / content_size_mm.width();
    const double pixels_per_mm_vertical = content_rect.height() / content_size_mm.height();

    const QSizeF page_rect_size_mm(page_rect.width() / pixels_per_mm_horizontal,
                                   page_rect.height() / pixels_per_mm_vertical);

    const double content_rect_center_x_in_mm
        = ((content_rect.center().x() - page_rect.left()) / page_rect.width()) * page_rect_size_mm.width();
    const double content_rect_center_y_in_mm
        = ((content_rect.center().y() - page_rect.top()) / page_rect.height()) * page_rect_size_mm.height();

    if (delta_width > .0) {
      agg_left_border = ((content_rect_center_x_in_mm / page_rect_size_mm.width()) * aggregate_hard_size_mm.width())
                        - (hard_size_mm.width() / 2);
      agg_left_border = qBound(.0, agg_left_border, delta_width);
      agg_right_border = delta_width - agg_left_border;
    }
    if (delta_height > .0) {
      agg_top_border = ((content_rect_center_y_in_mm / page_rect_size_mm.height()) * aggregate_hard_size_mm.height())
                       - (hard_size_mm.height() / 2);
      agg_top_border = qBound(.0, agg_top_border, delta_height);
      agg_bottom_border = delta_height - agg_top_border;
    }

    if ((corrected_alignment.horizontal() == Alignment::HAUTO)
        || (corrected_alignment.vertical() == Alignment::VAUTO)) {
      const double goldenRatio = (1 + std::sqrt(5)) / 2;

      if (corrected_alignment.horizontal() == Alignment::HAUTO) {
        const double right_grid_line = page_rect_size_mm.width() / goldenRatio;
        const double left_grid_line = page_rect_size_mm.width() - right_grid_line;

        if (content_rect_center_x_in_mm < left_grid_line) {
          corrected_alignment.setHorizontal(Alignment::LEFT);
        } else if (content_rect_center_x_in_mm > right_grid_line) {
          corrected_alignment.setHorizontal(Alignment::RIGHT);
        } else {
          corrected_alignment.setHorizontal(Alignment::HCENTER);
        }
      }

      if (corrected_alignment.vertical() == Alignment::VAUTO) {
        const double bottom_grid_line = page_rect_size_mm.height() / goldenRatio;
        const double top_grid_line = page_rect_size_mm.height() - bottom_grid_line;

        if (content_rect_center_y_in_mm < top_grid_line) {
          corrected_alignment.setVertical(Alignment::TOP);
        } else if (content_rect_center_y_in_mm > bottom_grid_line) {
          corrected_alignment.setVertical(Alignment::BOTTOM);
        } else {
          corrected_alignment.setVertical(Alignment::VCENTER);
        }
      }
    }
  }

  if (delta_width > .0) {
    switch (corrected_alignment.horizontal()) {
      case Alignment::LEFT:
        right = delta_width;
        break;
      case Alignment::HCENTER:
        left = right = 0.5 * delta_width;
        break;
      case Alignment::RIGHT:
        left = delta_width;
        break;
      default:
        left = agg_left_border;
        right = agg_right_border;
        break;
    }
  }

  if (delta_height > .0) {
    switch (corrected_alignment.vertical()) {
      case Alignment::TOP:
        bottom = delta_height;
        break;
      case Alignment::VCENTER:
        top = bottom = 0.5 * delta_height;
        break;
      case Alignment::BOTTOM:
        top = delta_height;
        break;
      default:
        top = agg_top_border;
        bottom = agg_bottom_border;
        break;
    }
  }

  return Margins(left, top, right, bottom);
}  // Utils::calcSoftMarginsMM

QPolygonF Utils::calcPageRectPhys(const ImageTransformation& xform,
                                  const QPolygonF& content_rect_phys,
                                  const Params& params,
                                  const QSizeF& aggregate_hard_size_mm) {
  const QTransform pixelsToMmTransform(UnitsConverter(xform.origDpi()).transform(PIXELS, MILLIMETRES));

  QPolygonF poly_mm(pixelsToMmTransform.map(content_rect_phys));
  extendPolyRectWithMargins(poly_mm, params.hardMarginsMM());

  const QSizeF hard_size_mm(QLineF(poly_mm[0], poly_mm[1]).length(), QLineF(poly_mm[0], poly_mm[3]).length());
  Margins soft_margins_mm(calcSoftMarginsMM(hard_size_mm, aggregate_hard_size_mm, params.alignment(),
                                            params.contentRect(), params.contentSizeMM(), params.pageRect()));

  extendPolyRectWithMargins(poly_mm, soft_margins_mm);

  return pixelsToMmTransform.inverted().map(poly_mm);
}

QPointF Utils::getRightUnitVector(const QPolygonF& poly_rect) {
  const QPointF top_left(poly_rect[0]);
  const QPointF top_right(poly_rect[1]);

  return QLineF(top_left, top_right).unitVector().p2() - top_left;
}

QPointF Utils::getDownUnitVector(const QPolygonF& poly_rect) {
  const QPointF top_left(poly_rect[0]);
  const QPointF bottom_left(poly_rect[3]);

  return QLineF(top_left, bottom_left).unitVector().p2() - top_left;
}

QPolygonF Utils::shiftToRoundedOrigin(const QPolygonF& poly) {
  const double x = poly.boundingRect().left();
  const double y = poly.boundingRect().top();
  const double shift_value_x = -(x - std::round(x));
  const double shift_value_y = -(y - std::round(y));

  return poly.translated(shift_value_x, shift_value_y);
}
}  // namespace page_layout