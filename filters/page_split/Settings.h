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

#ifndef PAGE_SPLIT_SETTINGS_H_
#define PAGE_SPLIT_SETTINGS_H_

#include <QMutex>
#include <memory>
#include <set>
#include <unordered_map>
#include "ImageId.h"
#include "LayoutType.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "PageLayout.h"
#include "Params.h"
#include "ref_countable.h"

class AbstractRelinker;

namespace page_split {
class Settings : public ref_countable {
  DECLARE_NON_COPYABLE(Settings)

 private:
  class BaseRecord {
    // Member-wise copying is OK.
    friend class Settings;

   public:
    BaseRecord();

    const LayoutType* layoutType() const;

    const Params* params() const;

    /**
     * \brief A record is considered null of it doesn't carry any
     *        information.
     */
    bool isNull() const;

   protected:
    void setParams(const Params& params);

    void setLayoutType(LayoutType layout_type);

    void clearParams();

    void clearLayoutType();

    /**
     * \brief Checks if a particular layout type conflicts with PageLayout
     *        that is part of Params.
     */
    bool hasLayoutTypeConflict(LayoutType layout_type) const;

    Params m_params;
    LayoutType m_layoutType;
    bool m_paramsValid;
    bool m_layoutTypeValid;
  };


 public:
  class UpdateAction;

  class Record : public BaseRecord {
    // Member-wise copying is OK.
   public:
    explicit Record(LayoutType default_layout_type);

    Record(const BaseRecord& base_record, LayoutType default_layout_type);

    LayoutType combinedLayoutType() const;

    void update(const UpdateAction& action);

    bool hasLayoutTypeConflict() const;

   private:
    LayoutType m_defaultLayoutType;
  };


  class UpdateAction {
    friend class Settings::Record;

   public:
    UpdateAction();

    void setLayoutType(LayoutType layout_type);

    void clearLayoutType();

    void setParams(const Params& params);

    void clearParams();

   private:
    enum Action { DONT_TOUCH, SET, CLEAR };

    Params m_params;
    LayoutType m_layoutType;
    Action m_paramsAction;
    Action m_layoutTypeAction;
  };


  Settings();

  ~Settings() override;

  /**
   * \brief Reset all settings to their initial state.
   */
  void clear();

  void performRelinking(const AbstractRelinker& relinker);

  LayoutType defaultLayoutType() const;

  /**
   * Sets layout type for all pages, removing other page
   * parameters where they conflict with the new layout type.
   */
  void setLayoutTypeForAllPages(LayoutType layout_type);

  /**
   * Sets layout type for specified pages, removing other page
   * parameters where they conflict with the new layout type.
   */
  void setLayoutTypeFor(LayoutType layout_type, const std::set<PageId>& pages);

  /**
   * \brief Returns all data related to a page as a single object.
   */
  Record getPageRecord(const ImageId& image_id) const;

  /**
   * \brief Performs the requested update on the page.
   *
   * If the update would lead to a conflict between the layout
   * type and page parameters, the page parameters will be
   * cleared.
   */
  void updatePage(const ImageId& image_id, const UpdateAction& action);

  /**
   * \brief Performs a conditional update on the page.
   *
   * If the update would lead to a conflict between the layout
   * type and page parameters, the update won't take place.
   * Whether the update took place or not, the current page record
   * (updated or not) will be returned.
   */
  Record conditionalUpdate(const ImageId& image_id, const UpdateAction& action, bool* conflict = nullptr);

 private:
  typedef std::unordered_map<ImageId, BaseRecord> PerPageRecords;

  Record getPageRecordLocked(const ImageId& image_id) const;

  void updatePageLocked(const ImageId& image_id, const UpdateAction& action);

  void updatePageLocked(PerPageRecords::iterator it, const UpdateAction& action);

  mutable QMutex m_mutex;
  PerPageRecords m_perPageRecords;
  LayoutType m_defaultLayoutType;
};
}  // namespace page_split
#endif  // ifndef PAGE_SPLIT_SETTINGS_H_
