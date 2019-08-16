// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SELECTED_PAGE_H_
#define SELECTED_PAGE_H_

#include "PageId.h"
#include "PageView.h"

/**
 * The whole point of this class can be demonstrated with a few lines of code:
 * \code
 * ImageId image_id = ...;
 * SelectedPage page;
 * page.set(PageId(image_id, PageId::RIGHT_PAGE), PAGE_VIEW);
 * page.set(PageId(image_id, PageId::SINGLE_PAGE), IMAGE_VIEW);
 * page.get(PAGE_VIEW);  * \endcode
 * As seen above, this class remembers the sub-page as long as image id
 * stays the same.  Note that set(..., PAGE_VIEW) will always overwrite
 * the sub-page, while get(IMAGE_VIEW) will always return SINGLE_PAGE sub-pages.
 */
class SelectedPage {
 public:
  SelectedPage() = default;

  SelectedPage(const PageId& page_id, PageView view);

  bool isNull() const { return m_pageId.isNull(); }

  void set(const PageId& page_id, PageView view);

  PageId get(PageView view) const;

 private:
  PageId m_pageId;
};


#endif
