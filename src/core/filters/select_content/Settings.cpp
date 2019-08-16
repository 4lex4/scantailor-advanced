// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Settings.h"
#include <iostream>
#include "AbstractRelinker.h"
#include "RelinkablePath.h"
#include "../../Utils.h"

using namespace core;

namespace select_content {
Settings::Settings() : m_pageDetectionBox(0.0, 0.0), m_pageDetectionTolerance(0.1) {
  m_deviationProvider.setComputeValueByKey([this](const PageId& pageId) -> double {
    auto it(m_pageParams.find(pageId));
    if (it != m_pageParams.end()) {
      const Params& params = it->second;
      const QSizeF& contentSizeMM = params.contentSizeMM();

      return std::sqrt(contentSizeMM.width() * contentSizeMM.height() / 4 / 25.4);
    } else {
      return .0;
    };
  });
}

Settings::~Settings() = default;

void Settings::clear() {
  QMutexLocker locker(&m_mutex);
  m_pageParams.clear();
  m_deviationProvider.clear();
}

void Settings::performRelinking(const AbstractRelinker& relinker) {
  QMutexLocker locker(&m_mutex);
  PageParams new_params;

  for (const PageParams::value_type& kv : m_pageParams) {
    const RelinkablePath old_path(kv.first.imageId().filePath(), RelinkablePath::File);
    PageId new_page_id(kv.first);
    new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
    new_params.insert(PageParams::value_type(new_page_id, kv.second));
  }

  m_pageParams.swap(new_params);

  m_deviationProvider.clear();
  for (const PageParams::value_type& kv : m_pageParams) {
    m_deviationProvider.addOrUpdate(kv.first);
  }
}

void Settings::setPageParams(const PageId& page_id, const Params& params) {
  QMutexLocker locker(&m_mutex);
  Utils::mapSetValue(m_pageParams, page_id, params);
  m_deviationProvider.addOrUpdate(page_id);
}

void Settings::clearPageParams(const PageId& page_id) {
  QMutexLocker locker(&m_mutex);
  m_pageParams.erase(page_id);
  m_deviationProvider.remove(page_id);
}

std::unique_ptr<Params> Settings::getPageParams(const PageId& page_id) const {
  QMutexLocker locker(&m_mutex);

  const auto it(m_pageParams.find(page_id));
  if (it != m_pageParams.end()) {
    return std::make_unique<Params>(it->second);
  } else {
    return nullptr;
  }
}

bool Settings::isParamsNull(const PageId& page_id) const {
  QMutexLocker locker(&m_mutex);

  return (m_pageParams.find(page_id) == m_pageParams.end());
}

QSizeF Settings::pageDetectionBox() const {
  return m_pageDetectionBox;
}

void Settings::setPageDetectionBox(QSizeF size) {
  m_pageDetectionBox = size;
}

double Settings::pageDetectionTolerance() const {
  return m_pageDetectionTolerance;
}

void Settings::setPageDetectionTolerance(double tolerance) {
  m_pageDetectionTolerance = tolerance;
}

const DeviationProvider<PageId>& Settings::deviationProvider() const {
  return m_deviationProvider;
}

}  // namespace select_content