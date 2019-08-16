// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Settings.h"
#include "AbstractRelinker.h"
#include "RelinkablePath.h"
#include "../../Utils.h"

using namespace core;

namespace fix_orientation {
Settings::Settings() = default;

Settings::~Settings() = default;

void Settings::clear() {
  QMutexLocker locker(&m_mutex);
  m_perImageRotation.clear();
}

void Settings::performRelinking(const AbstractRelinker& relinker) {
  QMutexLocker locker(&m_mutex);
  PerImageRotation new_rotations;

  for (const PerImageRotation::value_type& kv : m_perImageRotation) {
    const RelinkablePath old_path(kv.first.filePath(), RelinkablePath::File);
    ImageId new_image_id(kv.first);
    new_image_id.setFilePath(relinker.substitutionPathFor(old_path));
    new_rotations.insert(PerImageRotation::value_type(new_image_id, kv.second));
  }

  m_perImageRotation.swap(new_rotations);
}

void Settings::applyRotation(const ImageId& image_id, const OrthogonalRotation rotation) {
  QMutexLocker locker(&m_mutex);
  setImageRotationLocked(image_id, rotation);
}

void Settings::applyRotation(const std::set<PageId>& pages, const OrthogonalRotation rotation) {
  QMutexLocker locker(&m_mutex);

  for (const PageId& page : pages) {
    setImageRotationLocked(page.imageId(), rotation);
  }
}

OrthogonalRotation Settings::getRotationFor(const ImageId& image_id) const {
  QMutexLocker locker(&m_mutex);

  auto it(m_perImageRotation.find(image_id));
  if (it != m_perImageRotation.end()) {
    return it->second;
  } else {
    return OrthogonalRotation();
  }
}

void Settings::setImageRotationLocked(const ImageId& image_id, const OrthogonalRotation& rotation) {
  Utils::mapSetValue(m_perImageRotation, image_id, rotation);
}

bool Settings::isRotationNull(const ImageId& image_id) const {
  QMutexLocker locker(&m_mutex);

  return (m_perImageRotation.find(image_id) == m_perImageRotation.end());
}
}  // namespace fix_orientation