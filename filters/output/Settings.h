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

#ifndef OUTPUT_SETTINGS_H_
#define OUTPUT_SETTINGS_H_

#include <QMutex>
#include <memory>
#include <unordered_map>
#include "ColorParams.h"
#include "DespeckleLevel.h"
#include "DewarpingOptions.h"
#include "Dpi.h"
#include "NonCopyable.h"
#include "OutputParams.h"
#include "OutputProcessingParams.h"
#include "PageId.h"
#include "Params.h"
#include "PropertySet.h"
#include "ZoneSet.h"
#include "dewarping/DistortionModel.h"
#include "ref_countable.h"

class AbstractRelinker;

namespace output {
class Params;

class Settings : public ref_countable {
  DECLARE_NON_COPYABLE(Settings)

 public:
  Settings();

  ~Settings() override;

  void clear();

  void performRelinking(const AbstractRelinker& relinker);

  Params getParams(const PageId& page_id) const;

  void setParams(const PageId& page_id, const Params& params);

  bool isParamsNull(const PageId& page_id) const;

  void setColorParams(const PageId& page_id, const ColorParams& prms);

  void setPictureShapeOptions(const PageId& page_id, PictureShapeOptions picture_shape_options);

  void setDpi(const PageId& page_id, const Dpi& dpi);

  void setDewarpingOptions(const PageId& page_id, const DewarpingOptions& opt);

  void setSplittingOptions(const PageId& page_id, const SplittingOptions& opt);

  void setDistortionModel(const PageId& page_id, const dewarping::DistortionModel& model);

  void setDepthPerception(const PageId& page_id, const DepthPerception& depth_perception);

  void setDespeckleLevel(const PageId& page_id, double level);

  std::unique_ptr<OutputParams> getOutputParams(const PageId& page_id) const;

  void removeOutputParams(const PageId& page_id);

  void setOutputParams(const PageId& page_id, const OutputParams& params);

  ZoneSet pictureZonesForPage(const PageId& page_id) const;

  ZoneSet fillZonesForPage(const PageId& page_id) const;

  void setPictureZones(const PageId& page_id, const ZoneSet& zones);

  void setFillZones(const PageId& page_id, const ZoneSet& zones);

  /**
   * For now, default zone properties are not persistent.
   * They may become persistent later though.
   */
  PropertySet defaultPictureZoneProperties() const;

  PropertySet defaultFillZoneProperties() const;

  void setDefaultPictureZoneProperties(const PropertySet& props);

  void setDefaultFillZoneProperties(const PropertySet& props);

  OutputProcessingParams getOutputProcessingParams(const PageId& page_id) const;

  void setOutputProcessingParams(const PageId& page_id, const OutputProcessingParams& output_processing_params);

  void setBlackOnWhite(const PageId& page_id, bool black_on_white);

 private:
  typedef std::unordered_map<PageId, Params> PerPageParams;
  typedef std::unordered_map<PageId, OutputParams> PerPageOutputParams;
  typedef std::unordered_map<PageId, ZoneSet> PerPageZones;
  typedef std::unordered_map<PageId, OutputProcessingParams> PerPageOutputProcessingParams;

  static PropertySet initialPictureZoneProps();

  static PropertySet initialFillZoneProps();

  mutable QMutex m_mutex;
  PerPageParams m_perPageParams;
  PerPageOutputParams m_perPageOutputParams;
  PerPageZones m_perPagePictureZones;
  PerPageZones m_perPageFillZones;
  PropertySet m_defaultPictureZoneProps;
  PropertySet m_defaultFillZoneProps;
  PerPageOutputProcessingParams m_perPageOutputProcessingParams;
};
}  // namespace output
#endif  // ifndef OUTPUT_SETTINGS_H_
