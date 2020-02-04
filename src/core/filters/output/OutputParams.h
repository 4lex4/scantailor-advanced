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
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_OUTPUTPARAMS_H_
