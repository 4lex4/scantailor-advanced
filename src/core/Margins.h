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

#ifndef MARGINS_H_
#define MARGINS_H_

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


#endif  // ifndef MARGINS_H_
