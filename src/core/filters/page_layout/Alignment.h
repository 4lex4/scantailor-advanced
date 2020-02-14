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


inline Alignment::Vertical Alignment::vertical() const {
  return m_vertical;
}

inline void Alignment::setVertical(Alignment::Vertical vertical) {
  m_vertical = vertical;
}

inline Alignment::Horizontal Alignment::horizontal() const {
  return m_horizontal;
}

inline void Alignment::setHorizontal(Alignment::Horizontal horizontal) {
  m_horizontal = horizontal;
}

inline bool Alignment::isNull() const {
  return m_isNull;
}

inline void Alignment::setNull(bool isNull) {
  m_isNull = isNull;
}

inline bool Alignment::isAutoVertical() const {
  return (m_vertical == VAUTO) || (m_vertical == VORIGINAL);
}

inline bool Alignment::isAutoHorizontal() const {
  return (m_horizontal == HAUTO) || (m_horizontal == HORIGINAL);
}
}  // namespace page_layout
#endif  // ifndef SCANTAILOR_PAGE_LAYOUT_ALIGNMENT_H_
