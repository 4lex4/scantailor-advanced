// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_LAYOUT_ALIGNMENT_H_
#define SCANTAILOR_PAGE_LAYOUT_ALIGNMENT_H_

class QDomDocument;
class QDomElement;
class QString;

#include <iostream>

class CommandLine;

namespace page_layout {
class Alignment {
 public:
  enum Vertical { TOP, VCENTER, BOTTOM, VAUTO, VORIGINAL };

  enum Horizontal { LEFT, HCENTER, RIGHT, HAUTO, HORIGINAL };

  /**
   * \brief Constructs a null alignment.
   */
  Alignment();

  Alignment(Vertical vertical, Horizontal horizontal);

  explicit Alignment(const QDomElement& el);

  Vertical vertical() const;

  void setVertical(Vertical vertical);

  Horizontal horizontal() const;

  void setHorizontal(Horizontal horizontal);

  bool isNull() const;

  void setNull(bool isNull);

  bool isAutoVertical() const;

  bool isAutoHorizontal() const;

  bool operator==(const Alignment& other) const;

  bool operator!=(const Alignment& other) const;

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

 private:
  Vertical m_vertical;
  Horizontal m_horizontal;
  bool m_isNull;
};
}  // namespace page_layout
#endif  // ifndef SCANTAILOR_PAGE_LAYOUT_ALIGNMENT_H_
