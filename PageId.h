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

#ifndef PAGEID_H_
#define PAGEID_H_

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
  explicit PageId(const ImageId& image_id, SubPage subpage = SINGLE_PAGE);

  bool isNull() const { return m_imageId.isNull(); }

  ImageId& imageId() { return m_imageId; }

  const ImageId& imageId() const { return m_imageId; }

  SubPage subPage() const { return m_subPage; }

  QString subPageAsString() const { return subPageToString(m_subPage); }

  static QString subPageToString(SubPage sub_page);

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

#endif  // ifndef PAGEID_H_
