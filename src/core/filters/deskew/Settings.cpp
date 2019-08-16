// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Settings.h"
#include "AbstractRelinker.h"
#include "RelinkablePath.h"
#include "../../Utils.h"

using namespace core;

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