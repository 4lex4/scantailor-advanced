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
#include <DeviationProvider.h>
#include <QMutex>
#include <boost/foreach.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <utility>
#include "AbstractRelinker.h"
#include "PageId.h"
#include "PageSequence.h"
#include "Params.h"
#include "RelinkablePath.h"


using namespace ::boost;
using namespace ::boost::multi_index;

namespace page_layout {
class Settings::Item {
 public:
  PageId pageId;
  Margins hardMarginsMM;
  QRectF pageRect;
  QRectF contentRect;
  QSizeF contentSizeMM;
  Alignment alignment;
  bool autoMargins;

  Item(const PageId& page_id,
       const Margins& hard_margins_mm,
       const QRectF& page_rect,
       const QRectF& content_rect,
       const QSizeF& content_size_mm,
       const Alignment& alignment,
       bool auto_margins);

  double hardWidthMM() const;

  double hardHeightMM() const;

  double influenceHardWidthMM() const;

  double influenceHardHeightMM() const;

  bool alignedWithOthers() const { return !alignment.isNull(); }
};


class Settings::ModifyMargins {
 public:
  ModifyMargins(const Margins& margins_mm, const bool auto_margins)
      : m_marginsMM(margins_mm), m_autoMargins(auto_margins) {}

  void operator()(Item& item) {
    item.hardMarginsMM = m_marginsMM;
    item.autoMargins = m_autoMargins;
  }

 private:
  Margins m_marginsMM;
  bool m_autoMargins;
};


class Settings::ModifyAlignment {
 public:
  explicit ModifyAlignment(const Alignment& alignment) : m_alignment(alignment) {}

  void operator()(Item& item) { item.alignment = m_alignment; }

 private:
  Alignment m_alignment;
};


class Settings::ModifyContentSize {
 public:
  ModifyContentSize(const QSizeF& content_size_mm, const QRectF& content_rect, const QRectF& page_rect)
      : m_contentSizeMM(content_size_mm), m_contentRect(content_rect), m_pageRect(page_rect) {}

  void operator()(Item& item) {
    item.contentSizeMM = m_contentSizeMM;
    item.contentRect = m_contentRect;
    item.pageRect = m_pageRect;
  }

 private:
  QSizeF m_contentSizeMM;
  QRectF m_contentRect;
  QRectF m_pageRect;
};


class Settings::Impl {
 public:
  Impl();

  ~Impl();

  void clear();

  void performRelinking(const AbstractRelinker& relinker);

  void removePagesMissingFrom(const PageSequence& pages);

  bool checkEverythingDefined(const PageSequence& pages, const PageId* ignore) const;

  std::unique_ptr<Params> getPageParams(const PageId& page_id) const;

  bool isParamsNull(const PageId& page_id) const;

  void setPageParams(const PageId& page_id, const Params& params);

  Params updateContentSizeAndGetParams(const PageId& page_id,
                                       const QRectF& page_rect,
                                       const QRectF& content_rect,
                                       const QSizeF& content_size_mm,
                                       QSizeF* agg_hard_size_before,
                                       QSizeF* agg_hard_size_after);

  const QRectF& updateAggregateContentRect();

  const QRectF& getAggregateContentRect() { return m_aggregateContentRect; }

  void setAggregateContentRect(const QRectF& aggregateContentRect) { m_aggregateContentRect = aggregateContentRect; }

  Margins getHardMarginsMM(const PageId& page_id) const;

  void setHardMarginsMM(const PageId& page_id, const Margins& margins_mm);

  Alignment getPageAlignment(const PageId& page_id) const;

  AggregateSizeChanged setPageAlignment(const PageId& page_id, const Alignment& alignment);

  AggregateSizeChanged setContentSizeMM(const PageId& page_id, const QSizeF& content_size_mm);

  void invalidateContentSize(const PageId& page_id);

  QSizeF getAggregateHardSizeMM() const;

  QSizeF getAggregateHardSizeMMLocked() const;

  QSizeF getAggregateHardSizeMM(const PageId& page_id, const QSizeF& hard_size_mm, const Alignment& alignment) const;

  bool isPageAutoMarginsEnabled(const PageId& page_id);

  void setPageAutoMarginsEnabled(const PageId& page_id, bool state);

  const DeviationProvider<PageId>& deviationProvider() const;

  std::vector<Guide>& guides();

  bool isShowingMiddleRectEnabled() const;

  void enableShowingMiddleRect(bool state);

 private:
  class SequencedTag;
  class DescWidthTag;
  class DescHeightTag;

  typedef multi_index_container<
      Item,
      indexed_by<ordered_unique<member<Item, PageId, &Item::pageId>>,
                 sequenced<tag<SequencedTag>>,
                 ordered_non_unique<tag<DescWidthTag>,
                                    // ORDER BY alignedWithOthers DESC, hardWidthMM DESC
                                    composite_key<Item,
                                                  const_mem_fun<Item, bool, &Item::alignedWithOthers>,
                                                  const_mem_fun<Item, double, &Item::hardWidthMM>>,
                                    composite_key_compare<std::greater<>, std::greater<double>>>,
                 ordered_non_unique<tag<DescHeightTag>,
                                    // ORDER BY alignedWithOthers DESC, hardHeightMM DESC
                                    composite_key<Item,
                                                  const_mem_fun<Item, bool, &Item::alignedWithOthers>,
                                                  const_mem_fun<Item, double, &Item::hardHeightMM>>,
                                    composite_key_compare<std::greater<>, std::greater<double>>>>>
      Container;

  typedef Container::index<SequencedTag>::type UnorderedItems;
  typedef Container::index<DescWidthTag>::type DescWidthOrder;
  typedef Container::index<DescHeightTag>::type DescHeightOrder;

  mutable QMutex m_mutex;
  Container m_items;
  UnorderedItems& m_unorderedItems;
  DescWidthOrder& m_descWidthOrder;
  DescHeightOrder& m_descHeightOrder;
  const QRectF m_invalidRect;
  const QSizeF m_invalidSize;
  const Margins m_defaultHardMarginsMM;
  const Alignment m_defaultAlignment;
  QRectF m_aggregateContentRect;
  const bool m_autoMarginsDefault;
  DeviationProvider<PageId> m_deviationProvider;
  std::vector<Guide> m_guides;
  bool m_showMiddleRect;
};


/*=============================== Settings ==================================*/

Settings::Settings() : m_ptrImpl(new Impl()) {}

Settings::~Settings() = default;

void Settings::clear() {
  return m_ptrImpl->clear();
}

void Settings::performRelinking(const AbstractRelinker& relinker) {
  m_ptrImpl->performRelinking(relinker);
}

void Settings::removePagesMissingFrom(const PageSequence& pages) {
  m_ptrImpl->removePagesMissingFrom(pages);
}

bool Settings::checkEverythingDefined(const PageSequence& pages, const PageId* ignore) const {
  return m_ptrImpl->checkEverythingDefined(pages, ignore);
}

std::unique_ptr<Params> Settings::getPageParams(const PageId& page_id) const {
  return m_ptrImpl->getPageParams(page_id);
}

void Settings::setPageParams(const PageId& page_id, const Params& params) {
  return m_ptrImpl->setPageParams(page_id, params);
}

Params Settings::updateContentSizeAndGetParams(const PageId& page_id,
                                               const QRectF& page_rect,
                                               const QRectF& content_rect,
                                               const QSizeF& content_size_mm,
                                               QSizeF* agg_hard_size_before,
                                               QSizeF* agg_hard_size_after) {
  return m_ptrImpl->updateContentSizeAndGetParams(page_id, page_rect, content_rect, content_size_mm,
                                                  agg_hard_size_before, agg_hard_size_after);
}

const QRectF& Settings::updateAggregateContentRect() {
  return m_ptrImpl->updateAggregateContentRect();
}

const QRectF& Settings::getAggregateContentRect() {
  return m_ptrImpl->getAggregateContentRect();
}

void Settings::setAggregateContentRect(const QRectF& contentRect) {
  m_ptrImpl->setAggregateContentRect(contentRect);
}

Margins Settings::getHardMarginsMM(const PageId& page_id) const {
  return m_ptrImpl->getHardMarginsMM(page_id);
}

void Settings::setHardMarginsMM(const PageId& page_id, const Margins& margins_mm) {
  m_ptrImpl->setHardMarginsMM(page_id, margins_mm);
}

Alignment Settings::getPageAlignment(const PageId& page_id) const {
  return m_ptrImpl->getPageAlignment(page_id);
}

Settings::AggregateSizeChanged Settings::setPageAlignment(const PageId& page_id, const Alignment& alignment) {
  return m_ptrImpl->setPageAlignment(page_id, alignment);
}

Settings::AggregateSizeChanged Settings::setContentSizeMM(const PageId& page_id, const QSizeF& content_size_mm) {
  return m_ptrImpl->setContentSizeMM(page_id, content_size_mm);
}

void Settings::invalidateContentSize(const PageId& page_id) {
  return m_ptrImpl->invalidateContentSize(page_id);
}

QSizeF Settings::getAggregateHardSizeMM() const {
  return m_ptrImpl->getAggregateHardSizeMM();
}

QSizeF Settings::getAggregateHardSizeMM(const PageId& page_id,
                                        const QSizeF& hard_size_mm,
                                        const Alignment& alignment) const {
  return m_ptrImpl->getAggregateHardSizeMM(page_id, hard_size_mm, alignment);
}

bool Settings::isPageAutoMarginsEnabled(const PageId& page_id) {
  return m_ptrImpl->isPageAutoMarginsEnabled(page_id);
}

void Settings::setPageAutoMarginsEnabled(const PageId& page_id, const bool state) {
  return m_ptrImpl->setPageAutoMarginsEnabled(page_id, state);
}

bool Settings::isParamsNull(const PageId& page_id) const {
  return m_ptrImpl->isParamsNull(page_id);
}

const DeviationProvider<PageId>& Settings::deviationProvider() const {
  return m_ptrImpl->deviationProvider();
}

std::vector<Guide>& Settings::guides() {
  return m_ptrImpl->guides();
}

bool Settings::isShowingMiddleRectEnabled() const {
  return m_ptrImpl->isShowingMiddleRectEnabled();
}

void Settings::enableShowingMiddleRect(const bool state) {
  m_ptrImpl->enableShowingMiddleRect(state);
}

/*============================== Settings::Item =============================*/

Settings::Item::Item(const PageId& page_id,
                     const Margins& hard_margins_mm,
                     const QRectF& page_rect,
                     const QRectF& content_rect,
                     const QSizeF& content_size_mm,
                     const Alignment& alignment,
                     const bool auto_margins)
    : pageId(page_id),
      hardMarginsMM(hard_margins_mm),
      pageRect(page_rect),
      contentRect(content_rect),
      contentSizeMM(content_size_mm),
      alignment(alignment),
      autoMargins(auto_margins) {}

double Settings::Item::hardWidthMM() const {
  return contentSizeMM.width() + hardMarginsMM.left() + hardMarginsMM.right();
}

double Settings::Item::hardHeightMM() const {
  return contentSizeMM.height() + hardMarginsMM.top() + hardMarginsMM.bottom();
}

double Settings::Item::influenceHardWidthMM() const {
  return alignment.isNull() ? 0.0 : hardWidthMM();
}

double Settings::Item::influenceHardHeightMM() const {
  return alignment.isNull() ? 0.0 : hardHeightMM();
}

/*============================= Settings::Impl ==============================*/

Settings::Impl::Impl()
    : m_items(),
      m_unorderedItems(m_items.get<SequencedTag>()),
      m_descWidthOrder(m_items.get<DescWidthTag>()),
      m_descHeightOrder(m_items.get<DescHeightTag>()),
      m_invalidRect(),
      m_invalidSize(),
      m_defaultHardMarginsMM(Margins(10.0, 5.0, 10.0, 5.0)),
      m_defaultAlignment(Alignment::TOP, Alignment::HCENTER),
      m_autoMarginsDefault(false),
      m_showMiddleRect(true) {
  m_deviationProvider.setComputeValueByKey([this](const PageId& pageId) -> double {
    auto it(m_items.find(pageId));
    if (it != m_items.end()) {
      if (it->alignment.isNull()) {
        return std::sqrt(it->hardWidthMM() * it->hardHeightMM() / 4 / 25.4);
      } else {
        return .0;
      }
    } else {
      return .0;
    };
  });
}

Settings::Impl::~Impl() = default;

void Settings::Impl::clear() {
  const QMutexLocker locker(&m_mutex);
  m_items.clear();
  m_deviationProvider.clear();
}

void Settings::Impl::performRelinking(const AbstractRelinker& relinker) {
  QMutexLocker locker(&m_mutex);
  Container new_items;

  for (const Item& item : m_unorderedItems) {
    const RelinkablePath old_path(item.pageId.imageId().filePath(), RelinkablePath::File);
    Item new_item(item);
    new_item.pageId.imageId().setFilePath(relinker.substitutionPathFor(old_path));
    new_items.insert(new_item);
  }

  m_items.swap(new_items);

  m_deviationProvider.clear();
  for (const Item& item : m_unorderedItems) {
    m_deviationProvider.addOrUpdate(item.pageId);
  }
}

void Settings::Impl::removePagesMissingFrom(const PageSequence& pages) {
  const QMutexLocker locker(&m_mutex);

  std::vector<PageId> sorted_pages;

  sorted_pages.reserve(pages.numPages());
  for (const PageInfo& page : pages) {
    sorted_pages.push_back(page.id());
  }
  std::sort(sorted_pages.begin(), sorted_pages.end());

  UnorderedItems::const_iterator it(m_unorderedItems.begin());
  const UnorderedItems::const_iterator end(m_unorderedItems.end());
  while (it != end) {
    if (std::binary_search(sorted_pages.begin(), sorted_pages.end(), it->pageId)) {
      ++it;
    } else {
      m_deviationProvider.remove(it->pageId);
      m_unorderedItems.erase(it++);
    }
  }
}

bool Settings::Impl::checkEverythingDefined(const PageSequence& pages, const PageId* ignore) const {
  const QMutexLocker locker(&m_mutex);

  for (const PageInfo& page_info : pages) {
    if (ignore && (*ignore == page_info.id())) {
      continue;
    }
    const Container::iterator it(m_items.find(page_info.id()));
    if ((it == m_items.end()) || !it->contentSizeMM.isValid()) {
      return false;
    }
  }

  return true;
}

std::unique_ptr<Params> Settings::Impl::getPageParams(const PageId& page_id) const {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(page_id));
  if (it == m_items.end()) {
    return nullptr;
  }

  return std::make_unique<Params>(it->hardMarginsMM, it->pageRect, it->contentRect, it->contentSizeMM, it->alignment,
                                  it->autoMargins);
}

void Settings::Impl::setPageParams(const PageId& page_id, const Params& params) {
  const QMutexLocker locker(&m_mutex);

  const Item new_item(page_id, params.hardMarginsMM(), params.pageRect(), params.contentRect(), params.contentSizeMM(),
                      params.alignment(), params.isAutoMarginsEnabled());

  const Container::iterator it(m_items.lower_bound(page_id));
  if ((it == m_items.end()) || (page_id < it->pageId)) {
    m_items.insert(it, new_item);
  } else {
    m_items.replace(it, new_item);
  }

  m_deviationProvider.addOrUpdate(page_id);
}

Params Settings::Impl::updateContentSizeAndGetParams(const PageId& page_id,
                                                     const QRectF& page_rect,
                                                     const QRectF& content_rect,
                                                     const QSizeF& content_size_mm,
                                                     QSizeF* agg_hard_size_before,
                                                     QSizeF* agg_hard_size_after) {
  const QMutexLocker locker(&m_mutex);

  if (agg_hard_size_before) {
    *agg_hard_size_before = getAggregateHardSizeMMLocked();
  }

  const Container::iterator it(m_items.lower_bound(page_id));
  Container::iterator item_it(it);
  if ((it == m_items.end()) || (page_id < it->pageId)) {
    const Item item(page_id, m_defaultHardMarginsMM, page_rect, content_rect, content_size_mm, m_defaultAlignment,
                    m_autoMarginsDefault);
    item_it = m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyContentSize(content_size_mm, content_rect, page_rect));
  }

  if (agg_hard_size_after) {
    *agg_hard_size_after = getAggregateHardSizeMMLocked();
  }

  updateAggregateContentRect();

  m_deviationProvider.addOrUpdate(page_id);

  return Params(item_it->hardMarginsMM, item_it->pageRect, item_it->contentRect, item_it->contentSizeMM,
                item_it->alignment, item_it->autoMargins);
}  // Settings::Impl::updateContentSizeAndGetParams

const QRectF& Settings::Impl::updateAggregateContentRect() {
  Container::iterator it = m_items.begin();
  if (it == m_items.end()) {
    return m_aggregateContentRect;
  }

  m_aggregateContentRect = it->contentRect;
  for (; it != m_items.end(); it++) {
    if (it->contentRect == m_invalidRect) {
      continue;
    }
    if (it->alignment.isNull()) {
      continue;
    }

    const QRectF page_rect(it->pageRect);
    QRectF content_rect(it->contentRect.translated(-page_rect.x(), -page_rect.y()));

    m_aggregateContentRect |= content_rect;
  }

  return m_aggregateContentRect;
}  // Settings::Impl::updateContentRect

Margins Settings::Impl::getHardMarginsMM(const PageId& page_id) const {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(page_id));
  if (it == m_items.end()) {
    return m_defaultHardMarginsMM;
  } else {
    return it->hardMarginsMM;
  }
}

void Settings::Impl::setHardMarginsMM(const PageId& page_id, const Margins& margins_mm) {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.lower_bound(page_id));
  if ((it == m_items.end()) || (page_id < it->pageId)) {
    const Item item(page_id, margins_mm, m_invalidRect, m_invalidRect, m_invalidSize, m_defaultAlignment,
                    m_autoMarginsDefault);
    m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyMargins(margins_mm, it->autoMargins));
  }

  m_deviationProvider.addOrUpdate(page_id);
}

Alignment Settings::Impl::getPageAlignment(const PageId& page_id) const {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(page_id));
  if (it == m_items.end()) {
    return m_defaultAlignment;
  } else {
    return it->alignment;
  }
}

Settings::AggregateSizeChanged Settings::Impl::setPageAlignment(const PageId& page_id, const Alignment& alignment) {
  const QMutexLocker locker(&m_mutex);

  const QSizeF agg_size_before(getAggregateHardSizeMMLocked());

  const Container::iterator it(m_items.lower_bound(page_id));
  if ((it == m_items.end()) || (page_id < it->pageId)) {
    const Item item(page_id, m_defaultHardMarginsMM, m_invalidRect, m_invalidRect, m_invalidSize, alignment,
                    m_autoMarginsDefault);
    m_items.insert(it, item);
  } else {
    if (alignment.isNull() != it->alignment.isNull()) {
      updateAggregateContentRect();
    }

    m_items.modify(it, ModifyAlignment(alignment));
  }

  m_deviationProvider.addOrUpdate(page_id);

  const QSizeF agg_size_after(getAggregateHardSizeMMLocked());
  if (agg_size_before == agg_size_after) {
    return AGGREGATE_SIZE_UNCHANGED;
  } else {
    return AGGREGATE_SIZE_CHANGED;
  }
}

Settings::AggregateSizeChanged Settings::Impl::setContentSizeMM(const PageId& page_id, const QSizeF& content_size_mm) {
  const QMutexLocker locker(&m_mutex);

  const QSizeF agg_size_before(getAggregateHardSizeMMLocked());

  const Container::iterator it(m_items.lower_bound(page_id));
  if ((it == m_items.end()) || (page_id < it->pageId)) {
    const Item item(page_id, m_defaultHardMarginsMM, m_invalidRect, m_invalidRect, content_size_mm, m_defaultAlignment,
                    m_autoMarginsDefault);
    m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyContentSize(content_size_mm, m_invalidRect, it->pageRect));
  }

  m_deviationProvider.addOrUpdate(page_id);

  const QSizeF agg_size_after(getAggregateHardSizeMMLocked());
  if (agg_size_before == agg_size_after) {
    return AGGREGATE_SIZE_UNCHANGED;
  } else {
    return AGGREGATE_SIZE_CHANGED;
  }
}

void Settings::Impl::invalidateContentSize(const PageId& page_id) {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(page_id));
  if (it != m_items.end()) {
    m_items.modify(it, ModifyContentSize(m_invalidSize, m_invalidRect, it->pageRect));
  }

  m_deviationProvider.addOrUpdate(page_id);
}

QSizeF Settings::Impl::getAggregateHardSizeMM() const {
  const QMutexLocker locker(&m_mutex);

  return getAggregateHardSizeMMLocked();
}

QSizeF Settings::Impl::getAggregateHardSizeMMLocked() const {
  if (m_items.empty()) {
    return QSizeF(0.0, 0.0);
  }

  const Item& max_width_item = *m_descWidthOrder.begin();
  const Item& max_height_item = *m_descHeightOrder.begin();

  const double width = max_width_item.influenceHardWidthMM();
  const double height = max_height_item.influenceHardHeightMM();

  return QSizeF(width, height);
}

QSizeF Settings::Impl::getAggregateHardSizeMM(const PageId& page_id,
                                              const QSizeF& hard_size_mm,
                                              const Alignment& alignment) const {
  if (alignment.isNull()) {
    return getAggregateHardSizeMM();
  }

  const QMutexLocker locker(&m_mutex);

  if (m_items.empty()) {
    return QSizeF(0.0, 0.0);
  }

  double width = 0.0;

  {
    DescWidthOrder::iterator it(m_descWidthOrder.begin());
    if (it->pageId != page_id) {
      width = it->influenceHardWidthMM();
    } else {
      ++it;
      if (it == m_descWidthOrder.end()) {
        width = hard_size_mm.width();
      } else {
        width = std::max(hard_size_mm.width(), qreal(it->influenceHardWidthMM()));
      }
    }
  }

  double height = 0.0;

  {
    DescHeightOrder::iterator it(m_descHeightOrder.begin());
    if (it->pageId != page_id) {
      height = it->influenceHardHeightMM();
    } else {
      ++it;
      if (it == m_descHeightOrder.end()) {
        height = hard_size_mm.height();
      } else {
        height = std::max(hard_size_mm.height(), qreal(it->influenceHardHeightMM()));
      }
    }
  }

  return QSizeF(width, height);
}  // Settings::Impl::getAggregateHardSizeMM

bool Settings::Impl::isPageAutoMarginsEnabled(const PageId& page_id) {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.find(page_id));
  if (it == m_items.end()) {
    return m_autoMarginsDefault;
  } else {
    return it->autoMargins;
  }
}

void Settings::Impl::setPageAutoMarginsEnabled(const PageId& page_id, const bool state) {
  const QMutexLocker locker(&m_mutex);

  const Container::iterator it(m_items.lower_bound(page_id));
  if ((it == m_items.end()) || (page_id < it->pageId)) {
    const Item item(page_id, m_defaultHardMarginsMM, m_invalidRect, m_invalidRect, m_invalidSize, m_defaultAlignment,
                    state);
    m_items.insert(it, item);
  } else {
    m_items.modify(it, ModifyMargins(it->hardMarginsMM, state));
  }
}

bool Settings::Impl::isParamsNull(const PageId& page_id) const {
  const QMutexLocker locker(&m_mutex);

  return (m_items.find(page_id) == m_items.end());
}

const DeviationProvider<PageId>& Settings::Impl::deviationProvider() const {
  return m_deviationProvider;
}

std::vector<Guide>& Settings::Impl::guides() {
  return m_guides;
}

bool Settings::Impl::isShowingMiddleRectEnabled() const {
  return m_showMiddleRect;
}

void Settings::Impl::enableShowingMiddleRect(const bool state) {
  m_showMiddleRect = state;
}
}  // namespace page_layout