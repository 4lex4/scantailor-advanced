// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DESKEW_DEPENDENCIES_H_
#define SCANTAILOR_DESKEW_DEPENDENCIES_H_

#include <QPolygonF>
#include "OrthogonalRotation.h"

class QDomDocument;
class QDomElement;
class QString;

namespace deskew {
/**
 * \brief Dependencies of deskew parameters.
 *
 * Once dependencies change, deskew parameters are no longer valid.
 */
class Dependencies {
 public:
  // Member-wise copying is OK.

  Dependencies();

  Dependencies(const QPolygonF& pageOutline, OrthogonalRotation rotation);

  explicit Dependencies(const QDomElement& depsEl);

  ~Dependencies();

  bool matches(const Dependencies& other) const;

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

 private:
  QPolygonF m_pageOutline;
  OrthogonalRotation m_rotation;
};
}  // namespace deskew
#endif  // ifndef SCANTAILOR_DESKEW_DEPENDENCIES_H_
