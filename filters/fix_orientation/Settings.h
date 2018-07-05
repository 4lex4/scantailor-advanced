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

#ifndef FIX_ORIENTATION_SETTINGS_H_
#define FIX_ORIENTATION_SETTINGS_H_

#include <QMutex>
#include <set>
#include <unordered_map>
#include "ImageId.h"
#include "NonCopyable.h"
#include "OrthogonalRotation.h"
#include "PageId.h"
#include "ref_countable.h"

class AbstractRelinker;

namespace fix_orientation {
class Settings : public ref_countable {
  DECLARE_NON_COPYABLE(Settings)

 public:
  Settings();

  ~Settings() override;

  void clear();

  void performRelinking(const AbstractRelinker& relinker);

  void applyRotation(const ImageId& image_id, OrthogonalRotation rotation);

  void applyRotation(const std::set<PageId>& pages, OrthogonalRotation rotation);

  OrthogonalRotation getRotationFor(const ImageId& image_id) const;

  bool isRotationNull(const ImageId& image_id) const;

 private:
  typedef std::unordered_map<ImageId, OrthogonalRotation> PerImageRotation;

  void setImageRotationLocked(const ImageId& image_id, const OrthogonalRotation& rotation);

  mutable QMutex m_mutex;
  PerImageRotation m_perImageRotation;
};
}  // namespace fix_orientation
#endif  // ifndef FIX_ORIENTATION_SETTINGS_H_
