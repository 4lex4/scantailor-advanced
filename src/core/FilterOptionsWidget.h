// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_FILTEROPTIONSWIDGET_H_
#define SCANTAILOR_CORE_FILTEROPTIONSWIDGET_H_

#include <QWidget>

#include "PageId.h"
#include "PageInfo.h"

class FilterOptionsWidget : public QWidget {
  Q_OBJECT
 signals:

  /**
   * \brief To be emitted by subclasses when they want to reload the page.
   */
  void reloadRequested();

  void invalidateThumbnail(const PageId& pageId);

  /**
   * This signature differs from invalidateThumbnail(PageId) in that
   * it will cause PageInfo stored by ThumbnailSequence to be updated.
   */
  void invalidateThumbnail(const PageInfo& pageInfo);

  void invalidateAllThumbnails();

  /**
   * After we've got rid of "Widest Page" / "Tallest Page" links,
   * there is no one using this signal.  It's a candidate for removal.
   */
  void goToPage(const PageId& pageId);
};


#endif
