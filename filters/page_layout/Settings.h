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

#ifndef PAGE_LAYOUT_SETTINGS_H_
#define PAGE_LAYOUT_SETTINGS_H_

#include <DeviationProvider.h>
#include <memory>
#include "Guide.h"
#include "Margins.h"
#include "NonCopyable.h"
#include "ref_countable.h"

class PageId;
class Margins;
class PageSequence;
class AbstractRelinker;
class QSizeF;
class QRectF;

namespace page_layout {
class Params;
class Alignment;

class Settings : public ref_countable {
  DECLARE_NON_COPYABLE(Settings)

 public:
  enum AggregateSizeChanged { AGGREGATE_SIZE_UNCHANGED, AGGREGATE_SIZE_CHANGED };

  Settings();

  ~Settings() override;

  /**
   * \brief Removes all stored data.
   */
  void clear();

  void performRelinking(const AbstractRelinker& relinker);

  /**
   * \brief Removes all stored data for pages that are not in the provided list.
   */
  void removePagesMissingFrom(const PageSequence& pages);

  /**
   * \brief Check that we have all the essential parameters for every
   *        page in the list.
   *
   * This check is used to allow of forbid going to the output stage.
   * \param pages The list of pages to check.
   * \param ignore The page to be ignored by the check.  Optional.
   */
  bool checkEverythingDefined(const PageSequence& pages, const PageId* ignore = nullptr) const;

  /**
   * \brief Get all page parameters at once.
   *
   * May return a null unique_ptr if the specified page is unknown to us.
   */
  std::unique_ptr<Params> getPageParams(const PageId& page_id) const;

  bool isParamsNull(const PageId& page_id) const;

  /**
   * \brief Set all page parameters at once.
   */
  void setPageParams(const PageId& page_id, const Params& params);

  /**
   * \brief Updates content size and returns all parameters at once.
   */
  Params updateContentSizeAndGetParams(const PageId& page_id,
                                       const QRectF& page_rect,
                                       const QRectF& content_rect,
                                       const QSizeF& content_size_mm,
                                       QSizeF* agg_hard_size_before = nullptr,
                                       QSizeF* agg_hard_size_after = nullptr);

  const QRectF& updateAggregateContentRect();

  const QRectF& getAggregateContentRect();

  void setAggregateContentRect(const QRectF& contentRect);

  /**
   * \brief Returns the hard margins for the specified page.
   *
   * Hard margins are margins that will be there no matter what.
   * Soft margins are those added to extend the page to match its
   * size with other pages.
   * \par
   * If no margins were assigned to the specified page, the default
   * margins are returned.
   */
  Margins getHardMarginsMM(const PageId& page_id) const;

  /**
   * \brief Sets hard margins for the specified page.
   *
   * Hard margins are margins that will be there no matter what.
   * Soft margins are those added to extend the page to match its
   * size with other pages.
   */
  void setHardMarginsMM(const PageId& page_id, const Margins& margins_mm);

  /**
   * \brief Returns the alignment for the specified page.
   *
   * Alignments affect the distribution of soft margins.
   * \par
   * If no alignment was specified, the default alignment is returned,
   * which is "center vertically and horizontally".
   */
  Alignment getPageAlignment(const PageId& page_id) const;

  /**
   * \brief Sets alignment for the specified page.
   *
   * Alignments affect the distribution of soft margins and whether this
   * page's size affects others and vice versa.
   */
  AggregateSizeChanged setPageAlignment(const PageId& page_id, const Alignment& alignment);

  /**
   * \brief Sets content size in millimeters for the specified page.
   *
   * The content size comes from the "Select Content" filter.
   */
  AggregateSizeChanged setContentSizeMM(const PageId& page_id, const QSizeF& content_size_mm);

  void invalidateContentSize(const PageId& page_id);

  /**
   * \brief Returns the aggregate (max width + max height) hard page size.
   */
  QSizeF getAggregateHardSizeMM() const;

  /**
   * \brief Same as getAggregateHardSizeMM(), but assumes a specified
   *        size and alignment for a specified page.
   *
   * This function doesn't modify anything, it just pretends that
   * the size and alignment of a specified page have changed.
   */
  QSizeF getAggregateHardSizeMM(const PageId& page_id, const QSizeF& hard_size_mm, const Alignment& alignment) const;

  bool isPageAutoMarginsEnabled(const PageId& page_id);

  void setPageAutoMarginsEnabled(const PageId& page_id, bool state);

  const DeviationProvider<PageId>& deviationProvider() const;

  std::vector<Guide>& guides();

  bool isShowingMiddleRectEnabled() const;

  void enableShowingMiddleRect(bool state);

 private:
  class Impl;
  class Item;
  class ModifyMargins;
  class ModifyAlignment;

  class ModifyContentSize;

  std::unique_ptr<Impl> m_ptrImpl;
};
}  // namespace page_layout
#endif  // ifndef PAGE_LAYOUT_SETTINGS_H_
