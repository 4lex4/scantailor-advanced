// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PAGEID_H_
#define SCANTAILOR_CORE_PAGEID_H_

#include "ImageId.h"

class QString;

/**
 * \brief A logical page on an image.
 *
 * An image can contain one or two logical pages.
 */
class PageId {
  // Member-wise copying is OK.
 public:
  enum SubPage { SINGLE_PAGE, LEFT_PAGE, RIGHT_PAGE };

  PageId();

  /**
   * \note The default parameter for subpage is not arbitrary.  It has to
   *       preceed other values in terms of operator<().  That's necessary
   *       to be able to use lower_bound() to find the first page with
   *       a matching image id.
   */
  explicit PageId(const ImageId& imageId, SubPage subpage = SINGLE_PAGE);

  bool isNull() const { return m_imageId.isNull(); }

  ImageId& imageId() { return m_imageId; }

  const ImageId& imageId() const { return m_imageId; }

  SubPage subPage() const { return m_subPage; }

  QString subPageAsString() const { return subPageToString(m_subPage); }

  static QString subPageToString(SubPage subPage);

  static SubPage subPageFromString(const QString& string, bool* ok = nullptr);

 private:
  ImageId m_imageId;
  SubPage m_subPage;
};


bool operator==(const PageId& lhs, const PageId& rhs);

bool operator!=(const PageId& lhs, const PageId& rhs);

bool operator<(const PageId& lhs, const PageId& rhs);

namespace std {
template <>
struct hash<PageId> {
  size_t operator()(const PageId& pageId) const noexcept {
    return (hash<ImageId>()(pageId.imageId()) ^ hash<int>()(pageId.subPage()) << 1);
  }
};
}  // namespace std

#endif  // ifndef SCANTAILOR_CORE_PAGEID_H_
