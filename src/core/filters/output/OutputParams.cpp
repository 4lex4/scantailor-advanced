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

#include "OutputParams.h"
#include <QDomDocument>
#include "FillZonePropFactory.h"
#include "PictureZonePropFactory.h"

namespace output {
OutputParams::OutputParams(const OutputImageParams& output_image_params,
                           const OutputFileParams& output_file_params,
                           const OutputFileParams& foreground_file_params,
                           const OutputFileParams& background_file_params,
                           const OutputFileParams& original_background_file_params,
                           const OutputFileParams& automask_file_params,
                           const OutputFileParams& speckles_file_params,
                           const ZoneSet& picture_zones,
                           const ZoneSet& fill_zones)
    : m_outputImageParams(output_image_params),
      m_outputFileParams(output_file_params),
      m_foregroundFileParams(foreground_file_params),
      m_backgroundFileParams(background_file_params),
      m_originalBackgroundFileParams(original_background_file_params),
      m_automaskFileParams(automask_file_params),
      m_specklesFileParams(speckles_file_params),
      m_pictureZones(picture_zones),
      m_fillZones(fill_zones) {}

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