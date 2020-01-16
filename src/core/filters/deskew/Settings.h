// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_DESKEW_SETTINGS_H_
#define SCANTAILOR_DESKEW_SETTINGS_H_

#include <DeviationProvider.h>

#include <QMutex>
#include <memory>
#include <set>
#include <unordered_map>

#include "NonCopyable.h"
#include "PageId.h"
#include "Params.h"
#include "ref_countable.h"

class AbstractRelinker;

namespace deskew {
class Settings : public ref_countable {
  DECLARE_NON_COPYABLE(Settings)

 public:
  Settings();

  ~Settings() override;

  void clear();

  void performRelinking(const AbstractRelinker& relinker);

  void setPageParams(const PageId& pageId, const Params& params);

  void clearPageParams(const PageId& pageId);

  std::unique_ptr<Params> getPageParams(const PageId& pageId) const;

  bool isParamsNull(const PageId& pageId) const;

  void setDegrees(const std::set<PageId>& pages, const Params& params);

  const DeviationProvider<PageId>& deviationProvider() const;

 private:
  using PerPageParams = std::unordered_map<PageId, Params>;

  mutable QMutex m_mutex;
  PerPageParams m_perPageParams;
  DeviationProvider<PageId> m_deviationProvider;
};
}  // namespace deskew
#endif  // ifndef SCANTAILOR_DESKEW_SETTINGS_H_
