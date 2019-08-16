// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
