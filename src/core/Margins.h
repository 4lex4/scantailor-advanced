// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_MARGINS_H_
#define SCANTAILOR_CORE_MARGINS_H_

class QString;
class QDomElement;
class QDomDocument;

class Margins {
 public:
  Margins();

  Margins(double left, double top, double right, double bottom);

  explicit Margins(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  double top() const;

  void setTop(double val);

  double bottom() const;

  void setBottom(double val);

  double left() const;

  void setLeft(double val);

  double right() const;

  void setRight(double val);

 private:
  double m_top;
  double m_bottom;
  double m_left;
  double m_right;
};


inline double Margins::top() const {
  return m_top;
}

inline void Margins::setTop(double val) {
  m_top = val;
}

inline double Margins::bottom() const {
  return m_bottom;
}

inline void Margins::setBottom(double val) {
  m_bottom = val;
}

inline double Margins::left() const {
  return m_left;
}

inline void Margins::setLeft(double val) {
  m_left = val;
}

inline double Margins::right() const {
  return m_right;
}

inline void Margins::setRight(double val) {
  m_right = val;
}

#endif  // ifndef SCANTAILOR_CORE_MARGINS_H_
