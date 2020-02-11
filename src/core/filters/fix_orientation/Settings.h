// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FIX_ORIENTATION_SETTINGS_H_
#define SCANTAILOR_FIX_ORIENTATION_SETTINGS_H_

#include <QMutex>
#include <set>
#include <unordered_map>

#include "ImageId.h"
#include "NonCopyable.h"
#include "OrthogonalRotation.h"
#include "PageId.h"

class AbstractRelinker;

namespace fix_orientation {
class Settings {
  DECLARE_NON_COPYABLE(Settings)

 public:
  Settings();

  virtual ~Settings();

  void clear();

  void performRelinking(const AbstractRelinker& relinker);

  void applyRotation(const ImageId& imageId, OrthogonalRotation rotation);

  void applyRotation(const std::set<PageId>& pages, OrthogonalRotation rotation);

  OrthogonalRotation getRotationFor(const ImageId& imageId) const;

  bool isRotationNull(const ImageId& imageId) const;

 private:
  using PerImageRotation = std::unordered_map<ImageId, OrthogonalRotation>;

  void setImageRotationLocked(const ImageId& imageId, const OrthogonalRotation& rotation);

  mutable QMutex m_mutex;
  PerImageRotation m_perImageRotation;
};
}  // namespace fix_orientation
#endif  // ifndef SCANTAILOR_FIX_ORIENTATION_SETTINGS_H_
