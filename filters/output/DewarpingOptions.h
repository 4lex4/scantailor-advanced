/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef OUTPUT_DEWARPING_MODE_H_
#define OUTPUT_DEWARPING_MODE_H_

#include <QString>
#include <QtXml/QDomElement>

namespace output {
enum DewarpingMode { OFF, AUTO, MANUAL, MARGINAL };

class DewarpingOptions {
 public:
  explicit DewarpingOptions(DewarpingMode mode = OFF, bool needPostDeskew = true);

  explicit DewarpingOptions(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool operator==(const DewarpingOptions& other) const;

  bool operator!=(const DewarpingOptions& other) const;

  DewarpingMode dewarpingMode() const;

  void setDewarpingMode(DewarpingMode m_mode);

  bool needPostDeskew() const;

  void setPostDeskew(bool postDeskew);

  static DewarpingMode parseDewarpingMode(const QString& str);

  static QString formatDewarpingMode(DewarpingMode mode);

 private:
  DewarpingMode m_mode;
  bool postDeskew;
};
}  // namespace output
#endif
