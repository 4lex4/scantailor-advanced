/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

  double width = page_rect.width() - content_rect.width();
  double height = page_rect.height() - content_rect.height();

  auto left = double(content_rect.left() - page_rect.left());
  auto right = double(page_rect.right() - content_rect.right());
  auto top = double(content_rect.top() - page_rect.top());
  auto bottom = double(page_rect.bottom() - content_rect.bottom());
  double hspace = left + right;
  double vspace = top + bottom;

  double lMM = (hspace < 1.0) ? 0.0 : (left * widthMM / hspace);
  double rMM = (hspace < 1.0) ? 0.0 : (right * widthMM / hspace);
  double tMM = (vspace < 1.0) ? 0.0 : (top * heightMM / vspace);
  double bMM = (vspace < 1.0) ? 0.0 : (bottom * heightMM / vspace);

  return Margins(lMM, tMM, rMM, bMM);
}

Margins Utils::calcSoftMarginsMM(const QSizeF& hard_size_mm,
                                 const QSizeF& aggregate_hard_size_mm,
                                 const Alignment& alignment,
                                 const QRectF& contentRect,
                                 const QSizeF& contentSizeMM,
                                 const QRectF& agg_content_rect,
                                 const QRectF& pageRect) {
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

  double aggLeftBorder = 0.0;
  double aggRightBorder = delta_width;
  double aggTopBorder = 0.0;
  double aggBottomBorder = delta_height;

  Alignment correctedAlignment = alignment;

  if (!contentSizeMM.isEmpty() && !contentRect.isEmpty() && !pageRect.isEmpty() && !agg_content_rect.isEmpty()) {
    const double pixelsPerMmHorizontal = contentRect.width() / contentSizeMM.width();
    const double pixelsPerMmVertical = contentRect.height() / contentSizeMM.height();

    const QSizeF agg_content_size_mm(agg_content_rect.width() / pixelsPerMmHorizontal,
                                     agg_content_rect.height() / pixelsPerMmVertical);

    QRectF correctedContentRect = contentRect.translated(-pageRect.x(), -pageRect.y());
    double content_rect_x_center_in_mm
        = ((correctedContentRect.center().x() - agg_content_rect.left()) / agg_content_rect.width())
          * agg_content_size_mm.width();
    double content_rect_y_center_in_mm
        = ((correctedContentRect.center().y() - agg_content_rect.top()) / agg_content_rect.height())
          * agg_content_size_mm.height();

    if (delta_width > 0.1) {
      aggLeftBorder = (content_rect_x_center_in_mm - (hard_size_mm.width() / 2))
                      - ((agg_content_size_mm.width() - aggregate_hard_size_mm.width()) / 2);
      if (aggLeftBorder < 0) {
        aggLeftBorder = 0;
      } else if (aggLeftBorder > delta_width) {
        aggLeftBorder = delta_width;
      }
      aggRightBorder = delta_width - aggLeftBorder;
    }
    if (delta_height > 0.1) {
      aggTopBorder = (content_rect_y_center_in_mm - (hard_size_mm.height() / 2))
                     - ((agg_content_size_mm.height() - aggregate_hard_size_mm.height()) / 2);
      if (aggTopBorder < 0) {
        aggTopBorder = 0;
      } else if (aggTopBorder > delta_height) {
        aggTopBorder = delta_height;
      }
      aggBottomBorder = delta_height - aggTopBorder;
    }

    if ((correctedAlignment.horizontal() == Alignment::HAUTO) || (correctedAlignment.vertical() == Alignment::VAUTO)) {
      const double goldenRatio = (1 + std::sqrt(5)) / 2;
      const double rightGridLine = agg_content_size_mm.width() / goldenRatio;
      const double leftGridLine = agg_content_size_mm.width() - rightGridLine;
      const double bottomGridLine = agg_content_size_mm.height() / goldenRatio;
      const double topGridLine = agg_content_size_mm.height() - bottomGridLine;

      if (correctedAlignment.horizontal() == Alignment::HAUTO) {
        if (content_rect_x_center_in_mm < leftGridLine) {
          correctedAlignment.setHorizontal(Alignment::LEFT);
        } else if (content_rect_x_center_in_mm > rightGridLine) {
          correctedAlignment.setHorizontal(Alignment::RIGHT);
        } else {
          correctedAlignment.setHorizontal(Alignment::HCENTER);
        }
      }

      if (correctedAlignment.vertical() == Alignment::VAUTO) {
        if (content_rect_y_center_in_mm < topGridLine) {
          correctedAlignment.setVertical(Alignment::TOP);
        } else if (content_rect_y_center_in_mm > bottomGridLine) {
          correctedAlignment.setVertical(Alignment::BOTTOM);
        } else {
          correctedAlignment.setVertical(Alignment::VCENTER);
        }
      }
    }
  }

  if (delta_width > 0.0) {
    switch (correctedAlignment.horizontal()) {
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
        left = aggLeftBorder;
        right = aggRightBorder;
        break;
    }
  }

  if (delta_height > 0.0) {
    switch (correctedAlignment.vertical()) {
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
        top = aggTopBorder;
        bottom = aggBottomBorder;
        break;
    }
  }

  return Margins(left, top, right, bottom);
}  // Utils::calcSoftMarginsMM

QPolygonF Utils::calcPageRectPhys(const ImageTransformation& xform,
                                  const QPolygonF& content_rect_phys,
                                  const Params& params,
                                  const QSizeF& aggregate_hard_size_mm,
                                  const QRectF& agg_content_rect) {
  const QTransform pixelsToMmTransform(UnitsConverter(xform.origDpi()).transform(PIXELS, MILLIMETRES));

  QPolygonF poly_mm(pixelsToMmTransform.map(content_rect_phys));
  extendPolyRectWithMargins(poly_mm, params.hardMarginsMM());

  const QSizeF hard_size_mm(QLineF(poly_mm[0], poly_mm[1]).length(), QLineF(poly_mm[0], poly_mm[3]).length());
  Margins soft_margins_mm(calcSoftMarginsMM(hard_size_mm, aggregate_hard_size_mm, params.alignment(),
                                            params.contentRect(), params.contentSizeMM(), agg_content_rect,
                                            params.pageRect()));

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
}  // namespace page_layout