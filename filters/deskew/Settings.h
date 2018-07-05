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

#ifndef DESKEW_SETTINGS_H_
#define DESKEW_SETTINGS_H_

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

  void setPageParams(const PageId& page_id, const Params& params);

  void clearPageParams(const PageId& page_id);

  std::unique_ptr<Params> getPageParams(const PageId& page_id) const;

  bool isParamsNull(const PageId& page_id) const;

  void setDegrees(const std::set<PageId>& pages, const Params& params);

  const DeviationProvider<PageId>& deviationProvider() const;

 private:
  typedef std::unordered_map<PageId, Params> PerPageParams;

  mutable QMutex m_mutex;
  PerPageParams m_perPageParams;
  DeviationProvider<PageId> m_deviationProvider;
};
}  // namespace deskew
#endif  // ifndef DESKEW_SETTINGS_H_
