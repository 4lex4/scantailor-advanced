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
QRectF Utils::adaptContentRect(const ImageTransformation& xform, const QRectF& contentRect) {
  if (!contentRect.isEmpty()) {
    return contentRect;
  }

  const QPointF center(xform.resultingRect().center());
  const QPointF delta(0.01, 0.01);
  return QRectF(center - delta, center + delta);
}

QSizeF Utils::calcRectSizeMM(const ImageTransformation& xform, const QRectF& rect) {
  const QTransform virtToMm(xform.transformBack() * UnitsConverter(xform.origDpi()).transform(PIXELS, MILLIMETRES));

  const QLineF horLine(rect.topLeft(), rect.topRight());
  const QLineF verLine(rect.topLeft(), rect.bottomLeft());

  const double width = virtToMm.map(horLine).length();
  const double height = virtToMm.map(verLine).length();
  return QSizeF(width, height);
}

void Utils::extendPolyRectWithMargins(QPolygonF& polyRect, const Margins& margins) {
  const QPointF downUv(getDownUnitVector(polyRect));
  const QPointF rightUv(getRightUnitVector(polyRect));

  // top-left
  polyRect[0] -= downUv * margins.top();
  polyRect[0] -= rightUv * margins.left();

  // top-right
  polyRect[1] -= downUv * margins.top();
  polyRect[1] += rightUv * margins.right();

  // bottom-right
  polyRect[2] += downUv * margins.bottom();
  polyRect[2] += rightUv * margins.right();

  // bottom-left
  polyRect[3] += downUv * margins.bottom();
  polyRect[3] -= rightUv * margins.left();

  if (polyRect.size() > 4) {
    assert(polyRect.size() == 5);
    // This polygon is closed.
    polyRect[4] = polyRect[3];
  }
}

Margins Utils::calcMarginsMM(const ImageTransformation& xform, const QRectF& pageRect, const QRectF& contentRect) {
  const QSizeF contentSizeMm(Utils::calcRectSizeMM(xform, contentRect));

  const QSizeF pageSizeMm(Utils::calcRectSizeMM(xform, pageRect));

  double widthMM = pageSizeMm.width() - contentSizeMm.width();
  double heightMM = pageSizeMm.height() - contentSizeMm.height();

  auto left = double(contentRect.left() - pageRect.left());
  auto right = double(pageRect.right() - contentRect.right());
  auto top = double(contentRect.top() - pageRect.top());
  auto bottom = double(pageRect.bottom() - contentRect.bottom());
  double hSpace = left + right;
  double vSpace = top + bottom;

  double lMM = (hSpace < 1.0) ? 0.0 : (left * widthMM / hSpace);
  double rMM = (hSpace < 1.0) ? 0.0 : (right * widthMM / hSpace);
  double tMM = (vSpace < 1.0) ? 0.0 : (top * heightMM / vSpace);
  double bMM = (vSpace < 1.0) ? 0.0 : (bottom * heightMM / vSpace);
  return Margins(lMM, tMM, rMM, bMM);
}

Margins Utils::calcSoftMarginsMM(const QSizeF& hardSizeMm,
                                 const QSizeF& aggregateHardSizeMm,
                                 const Alignment& alignment,
                                 const QRectF& contentRect,
                                 const QSizeF& contentSizeMm,
                                 const QRectF& pageRect) {
  if (alignment.isNull()) {
    // This means we are not aligning this page with others.
    return Margins();
  }

  double top = 0.0;
  double bottom = 0.0;
  double left = 0.0;
  double right = 0.0;

  const double deltaWidth = aggregateHardSizeMm.width() - hardSizeMm.width();
  const double deltaHeight = aggregateHardSizeMm.height() - hardSizeMm.height();

  double aggLeftBorder = 0.0;
  double aggRightBorder = deltaWidth;
  double aggTopBorder = 0.0;
  double aggBottomBorder = deltaHeight;

  Alignment correctedAlignment = alignment;

  if (!contentSizeMm.isEmpty() && !contentRect.isEmpty() && !pageRect.isEmpty()) {
    const double pixelsPerMmHorizontal = contentRect.width() / contentSizeMm.width();
    const double pixelsPerMmVertical = contentRect.height() / contentSizeMm.height();

    const QSizeF pageRectSizeMm(pageRect.width() / pixelsPerMmHorizontal, pageRect.height() / pixelsPerMmVertical);

    const double contentRectCenterXInMm
        = ((contentRect.center().x() - pageRect.left()) / pageRect.width()) * pageRectSizeMm.width();
    const double contentRectCenterYInMm
        = ((contentRect.center().y() - pageRect.top()) / pageRect.height()) * pageRectSizeMm.height();

    if (deltaWidth > .0) {
      aggLeftBorder = ((contentRectCenterXInMm / pageRectSizeMm.width()) * aggregateHardSizeMm.width())
                      - (hardSizeMm.width() / 2);
      aggLeftBorder = qBound(.0, aggLeftBorder, deltaWidth);
      aggRightBorder = deltaWidth - aggLeftBorder;
    }
    if (deltaHeight > .0) {
      aggTopBorder = ((contentRectCenterYInMm / pageRectSizeMm.height()) * aggregateHardSizeMm.height())
                     - (hardSizeMm.height() / 2);
      aggTopBorder = qBound(.0, aggTopBorder, deltaHeight);
      aggBottomBorder = deltaHeight - aggTopBorder;
    }

    if ((correctedAlignment.horizontal() == Alignment::HAUTO) || (correctedAlignment.vertical() == Alignment::VAUTO)) {
      const double goldenRatio = (1 + std::sqrt(5)) / 2;

      if (correctedAlignment.horizontal() == Alignment::HAUTO) {
        const double rightGridLine = pageRectSizeMm.width() / goldenRatio;
        const double leftGridLine = pageRectSizeMm.width() - rightGridLine;

        if (contentRectCenterXInMm < leftGridLine) {
          correctedAlignment.setHorizontal(Alignment::LEFT);
        } else if (contentRectCenterXInMm > rightGridLine) {
          correctedAlignment.setHorizontal(Alignment::RIGHT);
        } else {
          correctedAlignment.setHorizontal(Alignment::HCENTER);
        }
      }

      if (correctedAlignment.vertical() == Alignment::VAUTO) {
        const double bottomGridLine = pageRectSizeMm.height() / goldenRatio;
        const double topGridLine = pageRectSizeMm.height() - bottomGridLine;

        if (contentRectCenterYInMm < topGridLine) {
          correctedAlignment.setVertical(Alignment::TOP);
        } else if (contentRectCenterYInMm > bottomGridLine) {
          correctedAlignment.setVertical(Alignment::BOTTOM);
        } else {
          correctedAlignment.setVertical(Alignment::VCENTER);
        }
      }
    }
  }

  if (deltaWidth > .0) {
    switch (correctedAlignment.horizontal()) {
      case Alignment::LEFT:
        right = deltaWidth;
        break;
      case Alignment::HCENTER:
        left = right = 0.5 * deltaWidth;
        break;
      case Alignment::RIGHT:
        left = deltaWidth;
        break;
      default:
        left = aggLeftBorder;
        right = aggRightBorder;
        break;
    }
  }

  if (deltaHeight > .0) {
    switch (correctedAlignment.vertical()) {
      case Alignment::TOP:
        bottom = deltaHeight;
        break;
      case Alignment::VCENTER:
        top = bottom = 0.5 * deltaHeight;
        break;
      case Alignment::BOTTOM:
        top = deltaHeight;
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
                                  const QPolygonF& contentRectPhys,
                                  const Params& params,
                                  const QSizeF& aggregateHardSizeMm) {
  const QTransform pixelsToMmTransform(UnitsConverter(xform.origDpi()).transform(PIXELS, MILLIMETRES));

  QPolygonF polyMm(pixelsToMmTransform.map(contentRectPhys));
  extendPolyRectWithMargins(polyMm, params.hardMarginsMM());

  const QSizeF hardSizeMm(QLineF(polyMm[0], polyMm[1]).length(), QLineF(polyMm[0], polyMm[3]).length());
  Margins softMarginsMm(calcSoftMarginsMM(hardSizeMm, aggregateHardSizeMm, params.alignment(), params.contentRect(),
                                          params.contentSizeMM(), params.pageRect()));

  extendPolyRectWithMargins(polyMm, softMarginsMm);
  return pixelsToMmTransform.inverted().map(polyMm);
}

QPointF Utils::getRightUnitVector(const QPolygonF& polyRect) {
  const QPointF topLeft(polyRect[0]);
  const QPointF topRight(polyRect[1]);
  return QLineF(topLeft, topRight).unitVector().p2() - topLeft;
}

QPointF Utils::getDownUnitVector(const QPolygonF& polyRect) {
  const QPointF topLeft(polyRect[0]);
  const QPointF bottomLeft(polyRect[3]);
  return QLineF(topLeft, bottomLeft).unitVector().p2() - topLeft;
}

QPolygonF Utils::shiftToRoundedOrigin(const QPolygonF& poly) {
  const double x = poly.boundingRect().left();
  const double y = poly.boundingRect().top();
  const double shiftValueX = -(x - std::round(x));
  const double shiftValueY = -(y - std::round(y));
  return poly.translated(shiftValueX, shiftValueY);
}
}  // namespace page_layout