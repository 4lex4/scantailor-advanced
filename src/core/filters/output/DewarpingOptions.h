// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_DEWARPINGOPTIONS_H_
#define SCANTAILOR_OUTPUT_DEWARPINGOPTIONS_H_

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

  double getPostDeskewAngle() const;

  void setPostDeskewAngle(double postDeskewAngle);

  static DewarpingMode parseDewarpingMode(const QString& str);

  static QString formatDewarpingMode(DewarpingMode mode);

 private:
  DewarpingMode m_mode;
  bool m_needPostDeskew;
  double m_postDeskewAngle;
};
}  // namespace output
#endif
