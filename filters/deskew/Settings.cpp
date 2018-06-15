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
#include "AbstractRelinker.h"
#include "RelinkablePath.h"
#include "Utils.h"

namespace deskew {
Settings::Settings() {
  m_deviationProvider.setComputeValueByKey([this](const PageId& pageId) -> double {
    auto it(m_perPageParams.find(pageId));
    if (it != m_perPageParams.end()) {
      const Params& params = it->second;

      return params.deskewAngle();
    } else {
      return .0;
    };
  });
}

Settings::~Settings() = default;

void Settings::clear() {
  QMutexLocker locker(&m_mutex);
  m_perPageParams.clear();
  m_deviationProvider.clear();
}

void Settings::performRelinking(const AbstractRelinker& relinker) {
  QMutexLocker locker(&m_mutex);
  PerPageParams new_params;

  for (const PerPageParams::value_type& kv : m_perPageParams) {
    const RelinkablePath old_path(kv.first.imageId().filePath(), RelinkablePath::File);
    PageId new_page_id(kv.first);
    new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
    new_params.insert(PerPageParams::value_type(new_page_id, kv.second));
  }

  m_perPageParams.swap(new_params);

  m_deviationProvider.clear();
  for (const PerPageParams::value_type& kv : m_perPageParams) {
    m_deviationProvider.addOrUpdate(kv.first);
  }
}

void Settings::setPageParams(const PageId& page_id, const Params& params) {
  QMutexLocker locker(&m_mutex);
  Utils::mapSetValue(m_perPageParams, page_id, params);
  m_deviationProvider.addOrUpdate(page_id);
}

void Settings::clearPageParams(const PageId& page_id) {
  QMutexLocker locker(&m_mutex);
  m_perPageParams.erase(page_id);
  m_deviationProvider.remove(page_id);
}

std::unique_ptr<Params> Settings::getPageParams(const PageId& page_id) const {
  QMutexLocker locker(&m_mutex);

  auto it(m_perPageParams.find(page_id));
  if (it != m_perPageParams.end()) {
    return std::make_unique<Params>(it->second);
  } else {
    return nullptr;
  }
}

void Settings::setDegrees(const std::set<PageId>& pages, const Params& params) {
  const QMutexLocker locker(&m_mutex);
  for (const PageId& page : pages) {
    Utils::mapSetValue(m_perPageParams, page, params);
    m_deviationProvider.addOrUpdate(page);
  }
}

bool Settings::isParamsNull(const PageId& page_id) const {
  QMutexLocker locker(&m_mutex);

  return (m_perPageParams.find(page_id) == m_perPageParams.end());
}

const DeviationProvider<PageId>& Settings::deviationProvider() const {
  return m_deviationProvider;
}
}  // namespace deskew