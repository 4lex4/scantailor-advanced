
#include "PageLayoutAdapter.h"

namespace page_split {

QLineF PageLayoutAdapter::adaptCutter(const QLineF& cutterLine, const QRectF& newRect) {
  if (!newRect.isValid() || cutterLine.isNull()) {
    return cutterLine;
  }

  QLineF upperBorder(newRect.topLeft(), newRect.topRight());
  QPointF upperIntersection;
  if (upperBorder.intersect(cutterLine, &upperIntersection) == QLineF::NoIntersection) {
    return cutterLine;
  }
  // if intersection is outside the rect
  if (upperIntersection.x() < newRect.topLeft().x()) {
    upperIntersection.setX(newRect.topLeft().x());
  } else if (upperIntersection.x() > newRect.topRight().x()) {
    upperIntersection.setX(newRect.topRight().x());
  }

  QLineF lowerBorder(newRect.bottomLeft(), newRect.bottomRight());
  QPointF lowerIntersection;
  if (lowerBorder.intersect(cutterLine, &lowerIntersection) == QLineF::NoIntersection) {
    return cutterLine;
  }
  // if intersection is outside the rect
  if (lowerIntersection.x() < newRect.bottomLeft().x()) {
    lowerIntersection.setX(newRect.bottomLeft().x());
  } else if (lowerIntersection.x() > newRect.bottomRight().x()) {
    lowerIntersection.setX(newRect.bottomRight().x());
  }

  return QLineF(upperIntersection, lowerIntersection);
}

std::unique_ptr<QVector<QLineF>> PageLayoutAdapter::adaptCutters(const QVector<QLineF>& cuttersList,
                                                                 const QRectF& newRect) {
  auto adaptedCutters = std::make_unique<QVector<QLineF>>();

  for (QLineF cutter : cuttersList) {
    QLineF adaptedCutter = adaptCutter(cutter, newRect);
    adaptedCutters->append(adaptedCutter);
  }

  std::sort(adaptedCutters->begin(), adaptedCutters->end(),
            [](const QLineF& line1, const QLineF& line2) -> bool { return line1.x1() < line2.x1(); });

  // checking whether the cutters intersect each other inside the new rect, and if so fixing that
  const qreal upperBound = newRect.top();
  const qreal lowerBound = newRect.bottom();
  for (int i = 1; i < adaptedCutters->size(); i++) {
    QPointF intersection;
    QLineF cutterLeft = adaptedCutters->at(i - 1);
    QLineF cutterRight = adaptedCutters->at(i);

    if (cutterLeft.intersect(cutterRight, &intersection) == QLineF::NoIntersection) {
      continue;
    }

    if ((intersection.y() < lowerBound) && (intersection.y() > upperBound)) {
      if ((lowerBound - intersection.y()) <= (lowerBound - upperBound) / 2) {
        qreal newY = lowerBound;
        cutterLeft.setP2(QPointF(intersection.x(), newY));
        cutterRight.setP2(QPointF(intersection.x(), newY));
      } else {
        qreal newY = upperBound;
        cutterLeft.setP1(QPointF(intersection.x(), newY));
        cutterRight.setP1(QPointF(intersection.x(), newY));
      }
      adaptedCutters->replace(i - 1, cutterLeft);
      adaptedCutters->replace(i, cutterRight);
    }
  }

  return adaptedCutters;
}

void PageLayoutAdapter::correctPageLayoutType(PageLayout* layout) {
  const QRectF outline = layout->uncutOutline().boundingRect().toRect();

  if (layout->type() == PageLayout::SINGLE_PAGE_CUT) {
    QLineF cutterLine1 = layout->cutterLine(0).toLine();
    QLineF cutterLine2 = layout->cutterLine(1).toLine();

    // if both cutter lines match left or right bound
    if (((cutterLine1.x1() == cutterLine1.x2())
         && ((cutterLine1.x1() == outline.left()) || (cutterLine1.x1() == outline.right())))
        && ((cutterLine2.x1() == cutterLine2.x2())
            && ((cutterLine2.x1() == outline.left()) || (cutterLine2.x1() == outline.right())))) {
      layout->setType(PageLayout::SINGLE_PAGE_UNCUT);
    }

    // if cutter lines match or intersect inside outline (not valid)
    QPointF intersection;
    QLineF::IntersectType intersectType = cutterLine1.intersect(cutterLine2, &intersection);
    if (((intersectType != QLineF::NoIntersection)
         && (((intersection.y() > outline.top()) && (intersection.y() < outline.bottom()))))
        || ((intersectType == QLineF::NoIntersection) && (cutterLine1.pointAt(0) == cutterLine2.pointAt(0)))) {
      layout->setType(PageLayout::SINGLE_PAGE_UNCUT);
    }
  }

  if (layout->type() == PageLayout::TWO_PAGES) {
    QLineF cutterLine1 = layout->cutterLine(0).toLine();

    // if the cutter line matches left or right bound
    if ((cutterLine1.x1() == cutterLine1.x2())
        && ((cutterLine1.x1() == outline.left()) || (cutterLine1.x1() == outline.right()))) {
      layout->setType(PageLayout::SINGLE_PAGE_UNCUT);
    }
  }
}

std::unique_ptr<PageLayout> PageLayoutAdapter::adaptPageLayout(const PageLayout& pageLayout, const QRectF& outline) {
  if (pageLayout.uncutOutline().boundingRect() == outline) {
    return std::make_unique<PageLayout>(pageLayout);
  }

  std::unique_ptr<PageLayout> newPageLayout;

  if (pageLayout.type() == PageLayout::SINGLE_PAGE_CUT) {
    std::unique_ptr<QVector<QLineF>> adaptedCutters
        = PageLayoutAdapter::adaptCutters(QVector<QLineF>{pageLayout.cutterLine(0), pageLayout.cutterLine(1)}, outline);
    newPageLayout = std::make_unique<PageLayout>(outline, adaptedCutters->at(0), adaptedCutters->at(1));
    correctPageLayoutType(newPageLayout.get());
  } else if (pageLayout.type() == PageLayout::TWO_PAGES) {
    QLineF adaptedCutter = PageLayoutAdapter::adaptCutter(pageLayout.cutterLine(0), outline);
    newPageLayout = std::make_unique<PageLayout>(outline, adaptedCutter);
    correctPageLayoutType(newPageLayout.get());
  } else {
    newPageLayout = std::make_unique<PageLayout>(outline);
  }

  return newPageLayout;
}

}  // namespace page_split