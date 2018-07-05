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