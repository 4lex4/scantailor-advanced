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

#include "Settings.h"
#include "../../Utils.h"
#include "AbstractRelinker.h"
#include "FillColorProperty.h"
#include "RelinkablePath.h"

namespace output {
Settings::Settings()
    : m_defaultPictureZoneProps(initialPictureZoneProps()), m_defaultFillZoneProps(initialFillZoneProps()) {}

Settings::~Settings() = default;

void Settings::clear() {
  const QMutexLocker locker(&m_mutex);

  initialPictureZoneProps().swap(m_defaultPictureZoneProps);
  initialFillZoneProps().swap(m_defaultFillZoneProps);
  m_perPageParams.clear();
  m_perPageOutputParams.clear();
  m_perPagePictureZones.clear();
  m_perPageFillZones.clear();
  m_perPageOutputProcessingParams.clear();
}

void Settings::performRelinking(const AbstractRelinker& relinker) {
  const QMutexLocker locker(&m_mutex);

  PerPageParams new_params;
  PerPageOutputParams new_output_params;
  PerPageZones new_picture_zones;
  PerPageZones new_fill_zones;
  PerPageOutputProcessingParams new_output_processing_params;

  for (const PerPageParams::value_type& kv : m_perPageParams) {
    const RelinkablePath old_path(kv.first.imageId().filePath(), RelinkablePath::File);
    PageId new_page_id(kv.first);
    new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
    new_params.insert(PerPageParams::value_type(new_page_id, kv.second));
  }

  for (const PerPageOutputParams::value_type& kv : m_perPageOutputParams) {
    const RelinkablePath old_path(kv.first.imageId().filePath(), RelinkablePath::File);
    PageId new_page_id(kv.first);
    new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
    new_output_params.insert(PerPageOutputParams::value_type(new_page_id, kv.second));
  }

  for (const PerPageZones::value_type& kv : m_perPagePictureZones) {
    const RelinkablePath old_path(kv.first.imageId().filePath(), RelinkablePath::File);
    PageId new_page_id(kv.first);
    new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
    new_picture_zones.insert(PerPageZones::value_type(new_page_id, kv.second));
  }

  for (const PerPageZones::value_type& kv : m_perPageFillZones) {
    const RelinkablePath old_path(kv.first.imageId().filePath(), RelinkablePath::File);
    PageId new_page_id(kv.first);
    new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
    new_fill_zones.insert(PerPageZones::value_type(new_page_id, kv.second));
  }

  for (const PerPageOutputProcessingParams::value_type& kv : m_perPageOutputProcessingParams) {
    const RelinkablePath old_path(kv.first.imageId().filePath(), RelinkablePath::File);
    PageId new_page_id(kv.first);
    new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
    new_output_processing_params.insert(PerPageOutputProcessingParams::value_type(new_page_id, kv.second));
  }

  m_perPageParams.swap(new_params);
  m_perPageOutputParams.swap(new_output_params);
  m_perPagePictureZones.swap(new_picture_zones);
  m_perPageFillZones.swap(new_fill_zones);
  m_perPageOutputProcessingParams.swap(new_output_processing_params);
}  // Settings::performRelinking

Params Settings::getParams(const PageId& page_id) const {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageParams.find(page_id));
  if (it != m_perPageParams.end()) {
    return it->second;
  } else {
    return Params();
  }
}

void Settings::setParams(const PageId& page_id, const Params& params) {
  const QMutexLocker locker(&m_mutex);
  Utils::mapSetValue(m_perPageParams, page_id, params);
}

void Settings::setColorParams(const PageId& page_id, const ColorParams& prms) {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageParams.find(page_id));
  if (it == m_perPageParams.end()) {
    Params params;
    params.setColorParams(prms);
    m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
  } else {
    it->second.setColorParams(prms);
  }
}

void Settings::setPictureShapeOptions(const PageId& page_id, PictureShapeOptions picture_shape_options) {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageParams.find(page_id));
  if (it == m_perPageParams.end()) {
    Params params;
    params.setPictureShapeOptions(picture_shape_options);
    m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
  } else {
    it->second.setPictureShapeOptions(picture_shape_options);
  }
}

void Settings::setDpi(const PageId& page_id, const Dpi& dpi) {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageParams.find(page_id));
  if (it == m_perPageParams.end()) {
    Params params;
    params.setOutputDpi(dpi);
    m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
  } else {
    it->second.setOutputDpi(dpi);
  }
}

void Settings::setDewarpingOptions(const PageId& page_id, const DewarpingOptions& opt) {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageParams.find(page_id));
  if (it == m_perPageParams.end()) {
    Params params;
    params.setDewarpingOptions(opt);
    m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
  } else {
    it->second.setDewarpingOptions(opt);
  }
}

void Settings::setSplittingOptions(const PageId& page_id, const SplittingOptions& opt) {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageParams.find(page_id));
  if (it == m_perPageParams.end()) {
    Params params;
    params.setSplittingOptions(opt);
    m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
  } else {
    it->second.setSplittingOptions(opt);
  }
}

void Settings::setDistortionModel(const PageId& page_id, const dewarping::DistortionModel& model) {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageParams.find(page_id));
  if (it == m_perPageParams.end()) {
    Params params;
    params.setDistortionModel(model);
    m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
  } else {
    it->second.setDistortionModel(model);
  }
}

void Settings::setDepthPerception(const PageId& page_id, const DepthPerception& depth_perception) {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageParams.find(page_id));
  if (it == m_perPageParams.end()) {
    Params params;
    params.setDepthPerception(depth_perception);
    m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
  } else {
    it->second.setDepthPerception(depth_perception);
  }
}

void Settings::setDespeckleLevel(const PageId& page_id, double level) {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageParams.find(page_id));
  if (it == m_perPageParams.end()) {
    Params params;
    params.setDespeckleLevel(level);
    m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
  } else {
    it->second.setDespeckleLevel(level);
  }
}

std::unique_ptr<OutputParams> Settings::getOutputParams(const PageId& page_id) const {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageOutputParams.find(page_id));
  if (it != m_perPageOutputParams.end()) {
    return std::make_unique<OutputParams>(it->second);
  } else {
    return nullptr;
  }
}

void Settings::removeOutputParams(const PageId& page_id) {
  const QMutexLocker locker(&m_mutex);
  m_perPageOutputParams.erase(page_id);
}

void Settings::setOutputParams(const PageId& page_id, const OutputParams& params) {
  const QMutexLocker locker(&m_mutex);
  Utils::mapSetValue(m_perPageOutputParams, page_id, params);
}

ZoneSet Settings::pictureZonesForPage(const PageId& page_id) const {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPagePictureZones.find(page_id));
  if (it != m_perPagePictureZones.end()) {
    return it->second;
  } else {
    return ZoneSet();
  }
}

ZoneSet Settings::fillZonesForPage(const PageId& page_id) const {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageFillZones.find(page_id));
  if (it != m_perPageFillZones.end()) {
    return it->second;
  } else {
    return ZoneSet();
  }
}

void Settings::setPictureZones(const PageId& page_id, const ZoneSet& zones) {
  const QMutexLocker locker(&m_mutex);
  Utils::mapSetValue(m_perPagePictureZones, page_id, zones);
}

void Settings::setFillZones(const PageId& page_id, const ZoneSet& zones) {
  const QMutexLocker locker(&m_mutex);
  Utils::mapSetValue(m_perPageFillZones, page_id, zones);
}

PropertySet Settings::defaultPictureZoneProperties() const {
  const QMutexLocker locker(&m_mutex);

  return m_defaultPictureZoneProps;
}

PropertySet Settings::defaultFillZoneProperties() const {
  const QMutexLocker locker(&m_mutex);

  return m_defaultFillZoneProps;
}

void Settings::setDefaultPictureZoneProperties(const PropertySet& props) {
  const QMutexLocker locker(&m_mutex);
  m_defaultPictureZoneProps = props;
}

void Settings::setDefaultFillZoneProperties(const PropertySet& props) {
  const QMutexLocker locker(&m_mutex);
  m_defaultFillZoneProps = props;
}

PropertySet Settings::initialPictureZoneProps() {
  PropertySet props;
  props.locateOrCreate<PictureLayerProperty>()->setLayer(PictureLayerProperty::PAINTER2);

  return props;
}

PropertySet Settings::initialFillZoneProps() {
  PropertySet props;
  props.locateOrCreate<FillColorProperty>()->setColor(Qt::white);

  return props;
}

OutputProcessingParams Settings::getOutputProcessingParams(const PageId& page_id) const {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageOutputProcessingParams.find(page_id));
  if (it != m_perPageOutputProcessingParams.end()) {
    return it->second;
  } else {
    return OutputProcessingParams();
  }
}

void Settings::setOutputProcessingParams(const PageId& page_id,
                                         const OutputProcessingParams& output_processing_params) {
  const QMutexLocker locker(&m_mutex);
  Utils::mapSetValue(m_perPageOutputProcessingParams, page_id, output_processing_params);
}

bool Settings::isParamsNull(const PageId& page_id) const {
  const QMutexLocker locker(&m_mutex);

  return m_perPageParams.find(page_id) == m_perPageParams.end();
}

void Settings::setBlackOnWhite(const PageId& page_id, const bool black_on_white) {
  const QMutexLocker locker(&m_mutex);

  const auto it(m_perPageParams.find(page_id));
  if (it == m_perPageParams.end()) {
    Params params;
    params.setBlackOnWhite(black_on_white);
    m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
  } else {
    it->second.setBlackOnWhite(black_on_white);
  }
}
}  // namespace output