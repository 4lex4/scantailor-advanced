// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_OUTPUTPARAMS_H_
#define SCANTAILOR_OUTPUT_OUTPUTPARAMS_H_

#include "OutputFileParams.h"
#include "OutputImageParams.h"
#include "ZoneSet.h"

class QDomDocument;
class QDomElement;
class QString;

namespace output {
class OutputParams {
 public:
  OutputParams(const OutputImageParams& outputImageParams,
               const OutputFileParams& sourceFileParams,
               const OutputFileParams& outputFileParams,
               const OutputFileParams& foregroundFileParams,
               const OutputFileParams& backgroundFileParams,
               const OutputFileParams& originalBackgroundFileParams,
               const OutputFileParams& automaskFileParams,
               const OutputFileParams& specklesFileParams,
               const ZoneSet& pictureZones,
               const ZoneSet& fillZones);

  explicit OutputParams(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  const OutputImageParams& outputImageParams() const;

  const OutputFileParams& sourceFileParams() const;

  const OutputFileParams& outputFileParams() const;

  const OutputFileParams& foregroundFileParams() const;

  const OutputFileParams& backgroundFileParams() const;

  const OutputFileParams& originalBackgroundFileParams() const;

  const OutputFileParams& automaskFileParams() const;

  const OutputFileParams& specklesFileParams() const;

  const ZoneSet& pictureZones() const;

  const ZoneSet& fillZones() const;

 private:
  OutputImageParams m_outputImageParams;
  OutputFileParams m_sourceFileParams;
  OutputFileParams m_outputFileParams;
  OutputFileParams m_foregroundFileParams;
  OutputFileParams m_backgroundFileParams;
  OutputFileParams m_originalBackgroundFileParams;
  OutputFileParams m_automaskFileParams;
  OutputFileParams m_specklesFileParams;
  ZoneSet m_pictureZones;
  ZoneSet m_fillZones;
};


inline const OutputImageParams& OutputParams::outputImageParams() const {
  return m_outputImageParams;
}

inline const OutputFileParams& OutputParams::outputFileParams() const {
  return m_outputFileParams;
}

inline const OutputFileParams& OutputParams::foregroundFileParams() const {
  return m_foregroundFileParams;
}

inline const OutputFileParams& OutputParams::backgroundFileParams() const {
  return m_backgroundFileParams;
}

inline const OutputFileParams& OutputParams::originalBackgroundFileParams() const {
  return m_originalBackgroundFileParams;
}

inline const OutputFileParams& OutputParams::automaskFileParams() const {
  return m_automaskFileParams;
}

inline const OutputFileParams& OutputParams::specklesFileParams() const {
  return m_specklesFileParams;
}

inline const ZoneSet& OutputParams::pictureZones() const {
  return m_pictureZones;
}

inline const ZoneSet& OutputParams::fillZones() const {
  return m_fillZones;
}

inline const OutputFileParams& OutputParams::sourceFileParams() const {
  return m_sourceFileParams;
}
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_OUTPUTPARAMS_H_
