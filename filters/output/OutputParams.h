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

#ifndef OUTPUT_OUTPUT_PARAMS_H_
#define OUTPUT_OUTPUT_PARAMS_H_

#include "OutputFileParams.h"
#include "OutputImageParams.h"
#include "ZoneSet.h"

class QDomDocument;
class QDomElement;
class QString;

namespace output {
class OutputParams {
 public:
  OutputParams(const OutputImageParams& output_image_params,
               const OutputFileParams& output_file_params,
               const OutputFileParams& foreground_file_params,
               const OutputFileParams& background_file_params,
               const OutputFileParams& original_background_file_params,
               const OutputFileParams& automask_file_params,
               const OutputFileParams& speckles_file_params,
               const ZoneSet& picture_zones,
               const ZoneSet& fill_zones);

  explicit OutputParams(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  const OutputImageParams& outputImageParams() const;

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
#endif  // ifndef OUTPUT_OUTPUT_PARAMS_H_
