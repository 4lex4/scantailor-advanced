// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUTPROCESSINGPARAMS_H
#define SCANTAILOR_OUTPUTPROCESSINGPARAMS_H

class QString;
class QDomDocument;
class QDomElement;

namespace output {
class OutputProcessingParams {
 public:
  OutputProcessingParams();

  explicit OutputProcessingParams(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool operator==(const OutputProcessingParams& other) const;

  bool operator!=(const OutputProcessingParams& other) const;

  bool isAutoZonesFound() const;

  void setAutoZonesFound(bool autoZonesFound);

  bool isBlackOnWhiteSetManually() const;

  void setBlackOnWhiteSetManually(bool blackOnWhiteSetManually);

 private:
  bool m_autoZonesFound;
  bool m_blackOnWhiteSetManually;
};
}  // namespace output


#endif  // SCANTAILOR_OUTPUTPROCESSINGPARAMS_H
