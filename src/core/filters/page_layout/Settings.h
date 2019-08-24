// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_LAYOUT_SETTINGS_H_
#define SCANTAILOR_PAGE_LAYOUT_SETTINGS_H_

#include <DeviationProvider.h>
#include <memory>
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
class Guide;

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
  std::unique_ptr<Params> getPageParams(const PageId& pageId) const;

  bool isParamsNull(const PageId& pageId) const;

  /**
   * \brief Set all page parameters at once.
   */
  void setPageParams(const PageId& pageId, const Params& params);

  /**
   * \brief Updates content size and returns all parameters at once.
   */
  Params updateContentSizeAndGetParams(const PageId& pageId,
                                       const QRectF& pageRect,
                                       const QRectF& contentRect,
                                       const QSizeF& contentSizeMm,
                                       QSizeF* aggHardSizeBefore = nullptr,
                                       QSizeF* aggHardSizeAfter = nullptr);

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
  Margins getHardMarginsMM(const PageId& pageId) const;

  /**
   * \brief Sets hard margins for the specified page.
   *
   * Hard margins are margins that will be there no matter what.
   * Soft margins are those added to extend the page to match its
   * size with other pages.
   */
  void setHardMarginsMM(const PageId& pageId, const Margins& marginsMm);

  /**
   * \brief Returns the alignment for the specified page.
   *
   * Alignments affect the distribution of soft margins.
   * \par
   * If no alignment was specified, the default alignment is returned,
   * which is "center vertically and horizontally".
   */
  Alignment getPageAlignment(const PageId& pageId) const;

  /**
   * \brief Sets alignment for the specified page.
   *
   * Alignments affect the distribution of soft margins and whether this
   * page's size affects others and vice versa.
   */
  AggregateSizeChanged setPageAlignment(const PageId& pageId, const Alignment& alignment);

  /**
   * \brief Sets content size in millimeters for the specified page.
   *
   * The content size comes from the "Select Content" filter.
   */
  AggregateSizeChanged setContentSizeMM(const PageId& pageId, const QSizeF& contentSizeMm);

  void invalidateContentSize(const PageId& pageId);

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
  QSizeF getAggregateHardSizeMM(const PageId& pageId, const QSizeF& hardSizeMm, const Alignment& alignment) const;

  bool isPageAutoMarginsEnabled(const PageId& pageId);

  void setPageAutoMarginsEnabled(const PageId& pageId, bool state);

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

  std::unique_ptr<Impl> m_impl;
};
}  // namespace page_layout
#endif  // ifndef SCANTAILOR_PAGE_LAYOUT_SETTINGS_H_
