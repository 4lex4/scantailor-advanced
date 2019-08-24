// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutputParams.h"
#include <QDomDocument>
#include "FillZonePropFactory.h"
#include "PictureZonePropFactory.h"

namespace output {
OutputParams::OutputParams(const OutputImageParams& outputImageParams,
                           const OutputFileParams& outputFileParams,
                           const OutputFileParams& foregroundFileParams,
                           const OutputFileParams& backgroundFileParams,
                           const OutputFileParams& originalBackgroundFileParams,
                           const OutputFileParams& automaskFileParams,
                           const OutputFileParams& specklesFileParams,
                           const ZoneSet& pictureZones,
                           const ZoneSet& fillZones)
    : m_outputImageParams(outputImageParams),
      m_outputFileParams(outputFileParams),
      m_foregroundFileParams(foregroundFileParams),
      m_backgroundFileParams(backgroundFileParams),
      m_originalBackgroundFileParams(originalBackgroundFileParams),
      m_automaskFileParams(automaskFileParams),
      m_specklesFileParams(specklesFileParams),
      m_pictureZones(pictureZones),
      m_fillZones(fillZones) {}

OutputParams::OutputParams(const QDomElement& el)
    : m_outputImageParams(el.namedItem("image").toElement()),
      m_outputFileParams(el.namedItem("file").toElement()),
      m_foregroundFileParams(el.namedItem("foreground_file").toElement()),
      m_backgroundFileParams(el.namedItem("background_file").toElement()),
      m_originalBackgroundFileParams(el.namedItem("original_background_file").toElement()),
      m_automaskFileParams(el.namedItem("automask").toElement()),
      m_specklesFileParams(el.namedItem("speckles").toElement()),
      m_pictureZones(el.namedItem("zones").toElement(), PictureZonePropFactory()),
      m_fillZones(el.namedItem("fill-zones").toElement(), FillZonePropFactory()) {}

QDomElement OutputParams::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.appendChild(m_outputImageParams.toXml(doc, "image"));
  el.appendChild(m_outputFileParams.toXml(doc, "file"));
  el.appendChild(m_foregroundFileParams.toXml(doc, "foreground_file"));
  el.appendChild(m_backgroundFileParams.toXml(doc, "background_file"));
  el.appendChild(m_originalBackgroundFileParams.toXml(doc, "original_background_file"));
  el.appendChild(m_automaskFileParams.toXml(doc, "automask"));
  el.appendChild(m_specklesFileParams.toXml(doc, "speckles"));
  el.appendChild(m_pictureZones.toXml(doc, "zones"));
  el.appendChild(m_fillZones.toXml(doc, "fill-zones"));

  return el;
}

const OutputImageParams& OutputParams::outputImageParams() const {
  return m_outputImageParams;
}

const OutputFileParams& OutputParams::outputFileParams() const {
  return m_outputFileParams;
}

const OutputFileParams& OutputParams::foregroundFileParams() const {
  return m_foregroundFileParams;
}

const OutputFileParams& OutputParams::backgroundFileParams() const {
  return m_backgroundFileParams;
}

const OutputFileParams& OutputParams::originalBackgroundFileParams() const {
  return m_originalBackgroundFileParams;
}

const OutputFileParams& OutputParams::automaskFileParams() const {
  return m_automaskFileParams;
}

const OutputFileParams& OutputParams::specklesFileParams() const {
  return m_specklesFileParams;
}

const ZoneSet& OutputParams::pictureZones() const {
  return m_pictureZones;
}

const ZoneSet& OutputParams::fillZones() const {
  return m_fillZones;
}
}  // namespace output