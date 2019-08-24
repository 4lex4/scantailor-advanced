// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_SPLIT_PAGELAYOUTADAPTER_H_
#define SCANTAILOR_PAGE_SPLIT_PAGELAYOUTADAPTER_H_

#include <QtCore/QLineF>
#include <QtCore/QRectF>
#include <QtCore/QVector>
#include <memory>
#include "PageLayout.h"

namespace page_split {
class PageLayoutAdapter {
 public:
  /**
   * Creates the new page layout from the given with another outline adapting the cutters
   */
  static PageLayout adaptPageLayout(const PageLayout& pageLayout, const QRectF& outline);

  /**
   * Corrects page layout type in place, depending on cutters.
   */
  static void correctPageLayoutType(PageLayout* layout);

 private:
  /**
   * Adapts the cutter line for the new outline.
   *
   * @param cutterLine the cutter line to be adapted.
   * @param newRect the new outline.
   * @return the adapted cutter line, where QLineF::p1() belongs the upper bound of the new outline,
   *         and QLineF::p2() does the lower one.
   */
  static QLineF adaptCutter(const QLineF& cutterLine, const QRectF& newRect);


  /**
   * Adapts the cutter lines for the new outline.
   *
   * @param cutterLine the list of the cutter lines to be adapted.
   * @param newRect the new outline.
   * @return the list of the adapted cutter lines.
   */
  static QVector<QLineF> adaptCutters(const QVector<QLineF>& cuttersList, const QRectF& newRect);
};
}  // namespace page_split


#endif  // SCANTAILOR_PAGE_SPLIT_PAGELAYOUTADAPTER_H_
