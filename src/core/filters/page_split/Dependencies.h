// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_SPLIT_DEPENDENCIES_H_
#define SCANTAILOR_PAGE_SPLIT_DEPENDENCIES_H_

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

  Dependencies(const QSize& imageSize, OrthogonalRotation rotation, LayoutType layoutType);

  void setLayoutType(LayoutType type);

  const OrthogonalRotation& orientation() const;

  bool compatibleWith(const Params& params) const;

  bool isNull() const;

  QDomElement toXml(QDomDocument& doc, const QString& tagName) const;

 private:
  QSize m_imageSize;
  OrthogonalRotation m_rotation;
  LayoutType m_layoutType;
};
}  // namespace page_split
#endif  // ifndef SCANTAILOR_PAGE_SPLIT_DEPENDENCIES_H_
