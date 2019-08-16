// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef THUMBNAILLOADRESULT_H_
#define THUMBNAILLOADRESULT_H_

#include <QPixmap>

class ThumbnailLoadResult {
 public:
  enum Status {
    /**
     * \brief Thumbnail loaded successfully.  Pixmap is not null.
     */
    LOADED,

    /**
     * \brief Thumbnail failed to load.  Pixmap is null.
     */
    LOAD_FAILED,

    /**
     * \brief Request has expired.  Pixmap is null.
     *
     * Consider the following situation: we scroll our thumbnail
     * list from the beginning all the way to the end.  This will
     * result in every thumbnail being requested.  If we just
     * load them in request order, that would be quite slow and
     * inefficient.  It would be nice if we could cancel the load
     * requests for items that went out of view.  Unfortunately,
     * QGraphicsView doesn't provide "went out of view"
     * notifications.  Instead, we load thumbnails starting from
     * most recently requested, and expire requests after a certain
     * number of newer requests are processed.  If the client is
     * still interested in the thumbnail, it may request it again.
     */
    REQUEST_EXPIRED
  };

  ThumbnailLoadResult(Status status, const QPixmap& pixmap) : m_pixmap(pixmap), m_status(status) {}

  Status status() const { return m_status; }

  const QPixmap& pixmap() const { return m_pixmap; }

 private:
  QPixmap m_pixmap;
  Status m_status;
};


#endif  // ifndef THUMBNAILLOADRESULT_H_
