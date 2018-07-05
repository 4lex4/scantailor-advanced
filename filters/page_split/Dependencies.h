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

#ifndef PAGE_SPLIT_DEPENDENCIES_H_
#define PAGE_SPLIT_DEPENDENCIES_H_

#include <QSize>
#include "LayoutType.h"
#include "OrthogonalRotation.h"

class QString;
class QDomDocument;
class QDomElement;

namespace page_split {
class Params;

/**
 * \brief Dependencies of a page parameters.
 *
 * Once dependencies change, the stored page parameters are no longer valid.
 */
class Dependencies {
  // Member-wise copying is OK.
 public:
  Dependencies();

  explicit Dependencies(const QDomElement& el);

  Dependencies(const QSize& image_size, OrthogonalRotation rotation, LayoutType layout_type);

  void setLayoutType(LayoutType type);

  const OrthogonalRotation& orientation() const;

  bool compatibleWith(const Params& params) const;

  bool isNull() const;

  QDomElement toXml(QDomDocument& doc, const QString& tag_name) const;

 private:
  QSize m_imageSize;
  OrthogonalRotation m_rotation;
  LayoutType m_layoutType;
};
}  // namespace page_split
#endif  // ifndef PAGE_SPLIT_DEPENDENCIES_H_
