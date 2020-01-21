// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ThumbnailSequence.h"

#include <core/ApplicationSettings.h>
#include <core/IconProvider.h>

#include <QApplication>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QMessageBox>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <memory>

#include "ColorSchemeManager.h"
#include "IncompleteThumbnail.h"
#include "PageSequence.h"
#include "ThumbnailFactory.h"

using namespace ::boost::multi_index;
using namespace ::boost::lambda;


class ThumbnailSequence::Item {
 public:
  Item(const PageInfo& pageInfo, CompositeItem* compItem);

  const PageId& pageId() const { return pageInfo.id(); }

  bool isSelected() const { return m_isSelected; }

  bool isSelectionLeader() const { return m_isSelectionLeader; }

  void setSelected(bool selected) const;

  void setSelectionLeader(bool selectionLeader) const;

  PageInfo pageInfo;
  mutable CompositeItem* composite;
  mutable bool incompleteThumbnail;

 private:
  mutable bool m_isSelected;
  mutable bool m_isSelectionLeader;
};


class ThumbnailSequence::GraphicsScene : public QGraphicsScene {
 public:
  using ContextMenuEventCallback = boost::function<void(QGraphicsSceneContextMenuEvent*)>;

  void setContextMenuEventCallback(ContextMenuEventCallback callback) { m_contextMenuEventCallback = callback; }

 protected:
  void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override {
    QGraphicsScene::contextMenuEvent(event);

    if (!event->isAccepted() && m_contextMenuEventCallback) {
      m_contextMenuEventCallback(event);
    }
  }

 private:
  ContextMenuEventCallback m_contextMenuEventCallback;
};


class ThumbnailSequence::Impl {
 public:
  Impl(ThumbnailSequence& owner, const QSizeF& maxLogicalThumbSize, ViewMode viewMode);

  ~Impl();

  void setThumbnailFactory(intrusive_ptr<ThumbnailFactory> factory);

  void attachView(QGraphicsView* view);

  void reset(const PageSequence& pages,
             SelectionAction selectionAction,
             intrusive_ptr<const PageOrderProvider> provider);

  intrusive_ptr<const PageOrderProvider> pageOrderProvider() const;

  PageSequence toPageSequence() const;

  void updateSceneItemsPos();

  void invalidateThumbnail(const PageId& pageId);

  void invalidateThumbnail(const PageInfo& pageInfo);

  void invalidateAllThumbnails();

  bool setSelection(const PageId& pageId, SelectionAction selectionAction);

  PageInfo selectionLeader() const;

  PageInfo prevPage(const PageId& referencePage) const;

  PageInfo nextPage(const PageId& referencePage) const;

  PageInfo prevSelectedPage(const PageId& referencePage) const;

  PageInfo nextSelectedPage(const PageId& referencePage) const;

  PageInfo firstPage() const;

  PageInfo lastPage() const;

  void insert(const PageInfo& newPage, BeforeOrAfter beforeOrAfter, const ImageId& image);

  void removePages(const std::set<PageId>& pagesToRemove);

  QRectF selectionLeaderSceneRect() const;

  std::set<PageId> selectedItems() const;

  std::vector<PageRange> selectedRanges() const;

  void contextMenuRequested(const PageInfo& pageInfo, const QPoint& screenPos, bool selected);

  void itemSelectedByUser(CompositeItem* compositeItem, Qt::KeyboardModifiers modifiers);

  const QSizeF& getMaxLogicalThumbSize() const;

  void setMaxLogicalThumbSize(const QSizeF& size);

  ViewMode getViewMode() const;

  void setViewMode(ViewMode mode);

  void setSelectionModeEnabled(bool enabled);

 private:
  class ItemsByIdTag;
  class ItemsInOrderTag;
  class SelectedThenUnselectedTag;

  using Container = multi_index_container<
      Item,
      indexed_by<hashed_unique<tag<ItemsByIdTag>, const_mem_fun<Item, const PageId&, &Item::pageId>, std::hash<PageId>>,
                 sequenced<tag<ItemsInOrderTag>>,
                 sequenced<tag<SelectedThenUnselectedTag>>>>;

  using ItemsById = Container::index<ItemsByIdTag>::type;
  using ItemsInOrder = Container::index<ItemsInOrderTag>::type;
  using SelectedThenUnselected = Container::index<SelectedThenUnselectedTag>::type;

  void invalidateThumbnailImpl(ItemsById::iterator idIt);

  void sceneContextMenuEvent(QGraphicsSceneContextMenuEvent* evt);

  void selectItemNoModifiers(const ItemsById::iterator& it);

  void selectItemWithControl(const ItemsById::iterator& it);

  void selectItemWithShift(const ItemsById::iterator& it);

  bool multipleItemsSelected() const;

  void moveToSelected(const Item* item);

  void moveToUnselected(const Item* item);

  void clear();

  void clearSelection();

  /**
   * Calculates the insertion position for an item with the given PageId
   * based on m_orderProvider.
   *
   * \param begin Beginning of the interval to consider.
   * \param end End of the interval to consider.
   * \param pageId The item to find insertion position for.
   * \param pageIncomplete Whether the page is represented by IncompleteThumbnail.
   * \param hint The place to start the search.  Must be within [begin, end].
   * \param distFromHint If provided, the distance from \p hint
   *        to the calculated insertion position will be written there.
   *        For example, \p distFromHint == -2 would indicate that the
   *        insertion position is two elements to the left of \p hint.
   */
  ItemsInOrder::iterator itemInsertPosition(ItemsInOrder::iterator begin,
                                            ItemsInOrder::iterator end,
                                            const PageId& pageId,
                                            bool pageIncomplete,
                                            ItemsInOrder::iterator hint,
                                            int* distFromHint = nullptr);

  std::unique_ptr<QGraphicsItem> getThumbnail(const PageInfo& pageInfo);

  std::unique_ptr<LabelGroup> getLabelGroup(const PageInfo& pageInfo);

  std::unique_ptr<CompositeItem> getCompositeItem(const Item* item, const PageInfo& info);

  void commitSceneRect();

  int getGraphicsViewWidth() const;

  void orderItems();

  bool cancelingSelectionAccepted();

  static const int SPACING = 3;
  ThumbnailSequence& m_owner;
  QSizeF m_maxLogicalThumbSize;
  ViewMode m_viewMode;
  Container m_items;
  ItemsById& m_itemsById;
  ItemsInOrder& m_itemsInOrder;

  /**
   * As the name implies, selected items go first here (in no particular order),
   * then go unselected items (also in no particular order).
   */
  SelectedThenUnselected& m_selectedThenUnselected;

  const Item* m_selectionLeader;
  intrusive_ptr<ThumbnailFactory> m_factory;
  intrusive_ptr<const PageOrderProvider> m_orderProvider;
  GraphicsScene m_graphicsScene;
  QRectF m_sceneRect;
  bool m_selectionMode;
};


class ThumbnailSequence::PlaceholderThumb : public QGraphicsItem {
 public:
  PlaceholderThumb(const QSizeF& maxSize);

  virtual QRectF boundingRect() const;

  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

 private:
  static QPainterPath m_sCachedPath;
  QSizeF m_maxSize;
};


class ThumbnailSequence::LabelGroup : public QGraphicsItemGroup {
 public:
  LabelGroup(std::unique_ptr<QGraphicsSimpleTextItem> normalLabel,
             std::unique_ptr<QGraphicsSimpleTextItem> boldLabel,
             std::unique_ptr<QGraphicsPixmapItem> pixmap = nullptr,
             std::unique_ptr<QGraphicsPixmapItem> pixmapSelected = nullptr);

  void updateAppearence(bool selected, bool selectionLeader);

 private:
  QGraphicsSimpleTextItem* m_normalLabel;
  QGraphicsSimpleTextItem* m_boldLabel;
  QGraphicsPixmapItem* m_pixmap;
  QGraphicsPixmapItem* m_pixmapSelected;
};


class ThumbnailSequence::CompositeItem : public QGraphicsItemGroup {
 public:
  CompositeItem(ThumbnailSequence::Impl& owner,
                std::unique_ptr<QGraphicsItem> thumbnail,
                std::unique_ptr<LabelGroup> labelGroup);

  void setItem(const Item* item) { m_item = item; }

  const Item* item() { return m_item; }

  bool incompleteThumbnail() const;

  void updateSceneRect(QRectF& sceneRect);

  void updateAppearence(bool selected, bool selectionLeader);

  virtual QRectF boundingRect() const;

  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

 protected:
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);

  virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);

 private:
  // We no longer use QGraphicsView's selection mechanism, so we
  // shadow isSelected() and setSelected() with unimplemented private
  // functions.  Just to be safe.
  bool isSelected() const;

  void setSelected(bool selected);

  ThumbnailSequence::Impl& m_owner;
  const ThumbnailSequence::Item* m_item;
  QGraphicsItem* m_thumb;
  LabelGroup* m_labelGroup;
};


/*============================= ThumbnailSequence ===========================*/

ThumbnailSequence::ThumbnailSequence(const QSizeF& maxLogicalThumbSize, const ViewMode viewMode)
    : m_impl(new Impl(*this, maxLogicalThumbSize, viewMode)) {}

ThumbnailSequence::~ThumbnailSequence() {}

void ThumbnailSequence::setThumbnailFactory(intrusive_ptr<ThumbnailFactory> factory) {
  m_impl->setThumbnailFactory(std::move(factory));
}

void ThumbnailSequence::attachView(QGraphicsView* const view) {
  m_impl->attachView(view);
}

void ThumbnailSequence::reset(const PageSequence& pages,
                              const SelectionAction selectionAction,
                              intrusive_ptr<const PageOrderProvider> orderProvider) {
  m_impl->reset(pages, selectionAction, std::move(orderProvider));
}

intrusive_ptr<const PageOrderProvider> ThumbnailSequence::pageOrderProvider() const {
  return m_impl->pageOrderProvider();
}

PageSequence ThumbnailSequence::toPageSequence() const {
  return m_impl->toPageSequence();
}

void ThumbnailSequence::invalidateThumbnail(const PageId& pageId) {
  m_impl->invalidateThumbnail(pageId);
}

void ThumbnailSequence::invalidateThumbnail(const PageInfo& pageInfo) {
  m_impl->invalidateThumbnail(pageInfo);
}

void ThumbnailSequence::invalidateAllThumbnails() {
  m_impl->invalidateAllThumbnails();
}

bool ThumbnailSequence::setSelection(const PageId& pageId, const SelectionAction selectionAction) {
  return m_impl->setSelection(pageId, selectionAction);
}

PageInfo ThumbnailSequence::selectionLeader() const {
  return m_impl->selectionLeader();
}

PageInfo ThumbnailSequence::prevPage(const PageId& referencePage) const {
  return m_impl->prevPage(referencePage);
}

PageInfo ThumbnailSequence::nextPage(const PageId& referencePage) const {
  return m_impl->nextPage(referencePage);
}

PageInfo ThumbnailSequence::prevSelectedPage(const PageId& referencePage) const {
  return m_impl->prevSelectedPage(referencePage);
}

PageInfo ThumbnailSequence::nextSelectedPage(const PageId& referencePage) const {
  return m_impl->nextSelectedPage(referencePage);
}

PageInfo ThumbnailSequence::firstPage() const {
  return m_impl->firstPage();
}

PageInfo ThumbnailSequence::lastPage() const {
  return m_impl->lastPage();
}

void ThumbnailSequence::insert(const PageInfo& newPage, BeforeOrAfter beforeOrAfter, const ImageId& image) {
  m_impl->insert(newPage, beforeOrAfter, image);
}

void ThumbnailSequence::removePages(const std::set<PageId>& pages) {
  m_impl->removePages(pages);
}

QRectF ThumbnailSequence::selectionLeaderSceneRect() const {
  return m_impl->selectionLeaderSceneRect();
}

std::set<PageId> ThumbnailSequence::selectedItems() const {
  return m_impl->selectedItems();
}

std::vector<PageRange> ThumbnailSequence::selectedRanges() const {
  return m_impl->selectedRanges();
}

void ThumbnailSequence::emitNewSelectionLeader(const PageInfo& pageInfo,
                                               const CompositeItem* composite,
                                               const SelectionFlags flags) {
  const QRectF thumbRect(composite->mapToScene(composite->boundingRect()).boundingRect());
  emit newSelectionLeader(pageInfo, thumbRect, flags);
}

const QSizeF& ThumbnailSequence::getMaxLogicalThumbSize() const {
  return m_impl->getMaxLogicalThumbSize();
}

void ThumbnailSequence::setMaxLogicalThumbSize(const QSizeF& size) {
  m_impl->setMaxLogicalThumbSize(size);
}

void ThumbnailSequence::updateSceneItemsPos() {
  m_impl->updateSceneItemsPos();
}

ThumbnailSequence::ViewMode ThumbnailSequence::getViewMode() const {
  return m_impl->getViewMode();
}

void ThumbnailSequence::setViewMode(ThumbnailSequence::ViewMode mode) {
  m_impl->setViewMode(mode);
}

void ThumbnailSequence::setSelectionModeEnabled(bool enabled) {
  m_impl->setSelectionModeEnabled(enabled);
}

/*======================== ThumbnailSequence::Impl ==========================*/

ThumbnailSequence::Impl::Impl(ThumbnailSequence& owner, const QSizeF& maxLogicalThumbSize, const ViewMode viewMode)
    : m_owner(owner),
      m_maxLogicalThumbSize(maxLogicalThumbSize),
      m_viewMode(viewMode),
      m_items(),
      m_itemsById(m_items.get<ItemsByIdTag>()),
      m_itemsInOrder(m_items.get<ItemsInOrderTag>()),
      m_selectedThenUnselected(m_items.get<SelectedThenUnselectedTag>()),
      m_selectionLeader(nullptr),
      m_selectionMode(false) {
  m_graphicsScene.setContextMenuEventCallback(
      [&](QGraphicsSceneContextMenuEvent* evt) { this->sceneContextMenuEvent(evt); });
}

ThumbnailSequence::Impl::~Impl() {}

void ThumbnailSequence::Impl::setThumbnailFactory(intrusive_ptr<ThumbnailFactory> factory) {
  m_factory = std::move(factory);
}

void ThumbnailSequence::Impl::attachView(QGraphicsView* const view) {
  view->setScene(&m_graphicsScene);
}

void ThumbnailSequence::Impl::reset(const PageSequence& pages,
                                    const SelectionAction selectionAction,
                                    intrusive_ptr<const PageOrderProvider> orderProvider) {
  m_orderProvider = std::move(orderProvider);

  std::set<PageId> selected;
  PageInfo selectionLeader;

  if (selectionAction == KEEP_SELECTION) {
    selectedItems().swap(selected);
    if (m_selectionLeader) {
      selectionLeader = m_selectionLeader->pageInfo;
    }
  }

  clear();  // Also clears the selection.

  if (pages.numPages() == 0) {
    return;
  }

  const Item* someSelectedItem = nullptr;
  for (const PageInfo& pageInfo : pages) {
    std::unique_ptr<CompositeItem> composite(getCompositeItem(0, pageInfo));
    m_itemsInOrder.push_back(Item(pageInfo, composite.release()));
    const Item* item = &m_itemsInOrder.back();
    item->composite->setItem(item);

    const ImageId& imageId = pageInfo.id().imageId();

    bool itemFound = (selected.find(pageInfo.id()) != selected.end());
    if (!itemFound) {
      switch (pageInfo.id().subPage()) {
        case PageId::LEFT_PAGE:
        case PageId::RIGHT_PAGE:
          itemFound = (selected.find(PageId(imageId, PageId::SINGLE_PAGE)) != selected.end());
          break;
        case PageId::SINGLE_PAGE:
          itemFound = (selected.find(PageId(imageId, PageId::LEFT_PAGE)) != selected.end())
                      || (selected.find(PageId(imageId, PageId::RIGHT_PAGE)) != selected.end());
          break;
      }
    }
    if (itemFound) {
      item->setSelected(true);
      moveToSelected(item);
      someSelectedItem = item;

      if ((pageInfo.id() == selectionLeader.id())
          || (!m_selectionLeader && (imageId == selectionLeader.id().imageId()))) {
        m_selectionLeader = item;
      }
    }
  }

  invalidateAllThumbnails();

  if (!m_selectionLeader) {
    if (someSelectedItem) {
      m_selectionLeader = someSelectedItem;
    }
  }
  if (m_selectionLeader) {
    m_selectionLeader->setSelectionLeader(true);
    m_owner.emitNewSelectionLeader(selectionLeader, m_selectionLeader->composite, DEFAULT_SELECTION_FLAGS);
  }
}  // ThumbnailSequence::Impl::reset

intrusive_ptr<const PageOrderProvider> ThumbnailSequence::Impl::pageOrderProvider() const {
  return m_orderProvider;
}

PageSequence ThumbnailSequence::Impl::toPageSequence() const {
  PageSequence pages;

  for (const Item& item : m_itemsInOrder) {
    pages.append(item.pageInfo);
  }
  return pages;
}

void ThumbnailSequence::Impl::invalidateThumbnail(const PageId& pageId) {
  const ItemsById::iterator idIt(m_itemsById.find(pageId));
  if (idIt != m_itemsById.end()) {
    invalidateThumbnailImpl(idIt);
  }
}

void ThumbnailSequence::Impl::invalidateThumbnail(const PageInfo& pageInfo) {
  const ItemsById::iterator idIt(m_itemsById.find(pageInfo.id()));
  if (idIt != m_itemsById.end()) {
    m_itemsById.modify(idIt, [&pageInfo](Item& item) { item.pageInfo = pageInfo; });
    invalidateThumbnailImpl(idIt);
  }
}

int ThumbnailSequence::Impl::getGraphicsViewWidth() const {
  int viewWidth = 0;
  if (!m_graphicsScene.views().isEmpty()) {
    QGraphicsView* gv = m_graphicsScene.views().first();
    viewWidth = gv->width();
    viewWidth -= gv->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    if (gv->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, 0, gv)) {
      viewWidth -= gv->frameWidth() * 2;
    }
  }
  return viewWidth;
}

void ThumbnailSequence::Impl::orderItems() {
  // Sort pages in m_itemsInOrder using m_orderProvider.
  if (m_orderProvider) {
    m_itemsInOrder.sort([this](const Item& lhs, const Item& rhs) {
      return m_orderProvider->precedes(lhs.pageId(), lhs.incompleteThumbnail, rhs.pageId(), rhs.incompleteThumbnail);
    });
  }
}

void ThumbnailSequence::Impl::updateSceneItemsPos() {
  m_sceneRect = QRectF(0.0, 0.0, 0.0, 0.0);

  const int viewWidth = getGraphicsViewWidth();
  assert(viewWidth > 0);
  double yOffset = SPACING;

  ItemsInOrder::iterator ordIt = m_itemsInOrder.begin();
  const ItemsInOrder::iterator ordEnd(m_itemsInOrder.end());

  while (ordIt != ordEnd) {
    int itemsInRow = 0;
    double sumItemWidths = 0;
    double xOffset = SPACING;

    if (m_viewMode == MULTI_COLUMN) {
      // Determine how many items can fit into the current row.
      for (ItemsInOrder::iterator rowIt = ordIt; rowIt != ordEnd; ++rowIt) {
        const double itemWidth = rowIt->composite->boundingRect().width();
        xOffset += itemWidth + SPACING;
        if (xOffset > viewWidth) {
          if (itemsInRow == 0) {
            // At least one page must be in a row.
            itemsInRow = 1;
            sumItemWidths = itemWidth;
          }
          break;
        }
        itemsInRow++;
        sumItemWidths += itemWidth;
      }
    } else {
      itemsInRow = 1;
      sumItemWidths = ordIt->composite->boundingRect().width();
    }

    // Split free space between the items in the current row.
    const double adjSpacing = std::floor((viewWidth - sumItemWidths) / (itemsInRow + 1));
    xOffset = adjSpacing;
    double nextYOffset = 0;
    for (; itemsInRow > 0; --itemsInRow, ++ordIt) {
      CompositeItem* composite = ordIt->composite;
      composite->setPos(xOffset, yOffset);
      composite->updateSceneRect(m_sceneRect);
      xOffset += composite->boundingRect().width() + adjSpacing;
      nextYOffset = std::max(composite->boundingRect().height() + SPACING, nextYOffset);
    }

    if (ordIt != ordEnd) {
      yOffset += nextYOffset;
    }
  }

  commitSceneRect();
}

void ThumbnailSequence::Impl::invalidateThumbnailImpl(const ItemsById::iterator idIt) {
  CompositeItem* const newComposite = getCompositeItem(&*idIt, idIt->pageInfo).release();
  CompositeItem* const oldComposite = idIt->composite;
  const QSizeF oldSize(oldComposite->boundingRect().size());
  const QSizeF newSize(newComposite->boundingRect().size());
  const QPointF oldPos(newComposite->pos());

  idIt->composite = newComposite;
  idIt->incompleteThumbnail = newComposite->incompleteThumbnail();
  delete oldComposite;

  newComposite->updateAppearence(idIt->isSelected(), idIt->isSelectionLeader());
  m_graphicsScene.addItem(newComposite);

  ItemsInOrder::iterator afterOld(m_items.project<ItemsInOrderTag>(idIt));
  // Notice afterOld++ below.
  // Move our item to the beginning of m_itemsInOrder, to make it out of range
  // we are going to pass to itemInsertPosition().
  m_itemsInOrder.relocate(m_itemsInOrder.begin(), afterOld++);
  const ItemsInOrder::iterator afterNew(itemInsertPosition(++m_itemsInOrder.begin(), m_itemsInOrder.end(),
                                                           idIt->pageInfo.id(), idIt->incompleteThumbnail, afterOld));
  // Move our item to its intended position.
  m_itemsInOrder.relocate(afterNew, m_itemsInOrder.begin());

  updateSceneItemsPos();

  // Possibly emit the newSelectionLeader() signal.
  if (m_selectionLeader == &*idIt) {
    if ((oldSize != newSize) || (oldPos != idIt->composite->pos())) {
      m_owner.emitNewSelectionLeader(idIt->pageInfo, idIt->composite, REDUNDANT_SELECTION);
    }
  }
}  // ThumbnailSequence::Impl::invalidateThumbnailImpl

void ThumbnailSequence::Impl::invalidateAllThumbnails() {
  // Recreate thumbnails now, whether a thumbnail is incomplete
  // is taken into account when sorting.
  ItemsInOrder::iterator ordIt(m_itemsInOrder.begin());
  const ItemsInOrder::iterator ordEnd(m_itemsInOrder.end());
  for (; ordIt != ordEnd; ++ordIt) {
    CompositeItem* const oldComposite = ordIt->composite;
    CompositeItem* const newComposite = getCompositeItem(&*ordIt, ordIt->pageInfo).release();

    ordIt->composite = newComposite;
    ordIt->incompleteThumbnail = newComposite->incompleteThumbnail();
    delete oldComposite;

    newComposite->updateAppearence(ordIt->isSelected(), ordIt->isSelectionLeader());
    m_graphicsScene.addItem(newComposite);
  }

  orderItems();
  updateSceneItemsPos();
}  // ThumbnailSequence::Impl::invalidateAllThumbnails

bool ThumbnailSequence::Impl::cancelingSelectionAccepted() {
  auto& appSettings = ApplicationSettings::getInstance();
  if (appSettings.isCancelingSelectionQuestionEnabled()) {
    std::set<PageId> selectedPages = selectedItems();
    if (selectedPages.size() < 2) {
      return true;
    } else if (selectedPages.size() == 2) {
      auto it = selectedPages.begin();
      if ((*it).imageId() == (*(it++)).imageId()) {
        return true;
      }
    }

    QMessageBox msgBox;
    msgBox.setWindowTitle(QObject::tr("Canceling multi page selection"));
    msgBox.setText(QObject::tr("%1 pages selection are going to be canceled. Continue?").arg(selectedPages.size()));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    QCheckBox dontAskAgainCb(QObject::tr("Don't show this message again."));
    msgBox.setCheckBox(&dontAskAgainCb);
    QObject::connect(&msgBox, &QMessageBox::accepted, [&appSettings, &dontAskAgainCb]() {
      if (dontAskAgainCb.isChecked()) {
        appSettings.setCancelingSelectionQuestionEnabled(false);
      }
    });
    if (msgBox.exec() != QMessageBox::Ok) {
      return false;
    }
  }
  return true;
}

bool ThumbnailSequence::Impl::setSelection(const PageId& pageId, const SelectionAction selectionAction) {
  if ((selectionAction == RESET_SELECTION) && !cancelingSelectionAccepted()) {
    return false;
  }

  const ItemsById::iterator idIt(m_itemsById.find(pageId));
  if (idIt == m_itemsById.end()) {
    return false;
  }

  const bool wasSelectionLeader = (&*idIt == m_selectionLeader);

  if (selectionAction != KEEP_SELECTION) {
    // Clear selection from all items except the one for which
    // selection is requested.
    SelectedThenUnselected::iterator it(m_selectedThenUnselected.begin());
    while (it != m_selectedThenUnselected.end()) {
      const Item& item = *it;
      if (!item.isSelected()) {
        break;
      }

      ++it;

      if (&*idIt != &item) {
        item.setSelected(false);
        moveToUnselected(&item);
        if (m_selectionLeader == &item) {
          m_selectionLeader = nullptr;
        }
      }
    }
  } else {
    // Only reset selection leader.
    if (m_selectionLeader && !wasSelectionLeader) {
      m_selectionLeader->setSelectionLeader(false);
      m_selectionLeader = nullptr;
    }
  }

  if (!wasSelectionLeader) {
    m_selectionLeader = &*idIt;
    m_selectionLeader->setSelectionLeader(true);
    moveToSelected(m_selectionLeader);
  }

  SelectionFlags flags = DEFAULT_SELECTION_FLAGS;
  if (wasSelectionLeader) {
    flags |= REDUNDANT_SELECTION;
  }
  if (selectionAction != KEEP_SELECTION) {
    flags |= SELECTION_CLEARED;
  }

  m_owner.emitNewSelectionLeader(idIt->pageInfo, idIt->composite, flags);
  return true;
}  // ThumbnailSequence::Impl::setSelection

PageInfo ThumbnailSequence::Impl::selectionLeader() const {
  if (m_selectionLeader) {
    return m_selectionLeader->pageInfo;
  } else {
    return PageInfo();
  }
}

PageInfo ThumbnailSequence::Impl::prevPage(const PageId& referencePage) const {
  ItemsInOrder::iterator ordIt;

  if (m_selectionLeader && (m_selectionLeader->pageInfo.id() == referencePage)) {
    // Common case optimization.
    ordIt = m_itemsInOrder.iterator_to(*m_selectionLeader);
  } else {
    ordIt = m_items.project<ItemsInOrderTag>(m_itemsById.find(referencePage));
  }

  if (ordIt != m_itemsInOrder.end()) {
    if (ordIt != m_itemsInOrder.begin()) {
      --ordIt;
      return ordIt->pageInfo;
    }
  }
  return PageInfo();
}

PageInfo ThumbnailSequence::Impl::nextPage(const PageId& referencePage) const {
  ItemsInOrder::iterator ordIt;

  if (m_selectionLeader && (m_selectionLeader->pageInfo.id() == referencePage)) {
    // Common case optimization.
    ordIt = m_itemsInOrder.iterator_to(*m_selectionLeader);
  } else {
    ordIt = m_items.project<ItemsInOrderTag>(m_itemsById.find(referencePage));
  }

  if (ordIt != m_itemsInOrder.end()) {
    ++ordIt;
    if (ordIt != m_itemsInOrder.end()) {
      return ordIt->pageInfo;
    }
  }
  return PageInfo();
}

PageInfo ThumbnailSequence::Impl::prevSelectedPage(const PageId& referencePage) const {
  ItemsInOrder::iterator ordIt;

  if (m_selectionLeader && (m_selectionLeader->pageInfo.id() == referencePage)) {
    // Common case optimization.
    ordIt = m_itemsInOrder.iterator_to(*m_selectionLeader);
  } else {
    ordIt = m_items.project<ItemsInOrderTag>(m_itemsById.find(referencePage));
  }

  if (ordIt != m_itemsInOrder.end()) {
    while (ordIt != m_itemsInOrder.begin()) {
      --ordIt;
      if (ordIt->isSelected()) {
        return ordIt->pageInfo;
      }
    }
  }
  return PageInfo();
}

PageInfo ThumbnailSequence::Impl::nextSelectedPage(const PageId& referencePage) const {
  ItemsInOrder::iterator ordIt;

  if (m_selectionLeader && (m_selectionLeader->pageInfo.id() == referencePage)) {
    // Common case optimization.
    ordIt = m_itemsInOrder.iterator_to(*m_selectionLeader);
  } else {
    ordIt = m_items.project<ItemsInOrderTag>(m_itemsById.find(referencePage));
  }

  if (ordIt != m_itemsInOrder.end()) {
    ++ordIt;
    while (ordIt != m_itemsInOrder.end()) {
      if (ordIt->isSelected()) {
        return ordIt->pageInfo;
      }
      ++ordIt;
    }
  }
  return PageInfo();
}

PageInfo ThumbnailSequence::Impl::firstPage() const {
  if (m_items.empty()) {
    return PageInfo();
  }
  return m_itemsInOrder.front().pageInfo;
}

PageInfo ThumbnailSequence::Impl::lastPage() const {
  if (m_items.empty()) {
    return PageInfo();
  }
  return m_itemsInOrder.back().pageInfo;
}

void ThumbnailSequence::Impl::insert(const PageInfo& newPage, BeforeOrAfter beforeOrAfter, const ImageId& image) {
  ItemsInOrder::iterator ordIt;

  if ((beforeOrAfter == BEFORE) && image.isNull()) {
    ordIt = m_itemsInOrder.end();
  } else {
    // Note that we have to use lower_bound() rather than find() because
    // we are not searching for PageId(image) exactly, which implies
    // PageId::SINGLE_PAGE configuration, but rather we search for
    // a page with any configuration, as long as it references the same image.
    ItemsById::iterator idIt(m_itemsById.find(PageId(image)));
    if ((idIt == m_itemsById.end()) || (idIt->pageInfo.imageId() != image)) {
      // Reference page not found.
      return;
    }

    ordIt = m_items.project<ItemsInOrderTag>(idIt);

    if (beforeOrAfter == AFTER) {
      ++ordIt;
      if (!m_orderProvider) {
        // Advance past not only the target page, but also its other half, if it follows.
        while (ordIt != m_itemsInOrder.end() && ordIt->pageInfo.imageId() == image) {
          ++ordIt;
        }
      }
    }
  }

  // If m_orderProvider is not set, ordIt won't change.
  ordIt = itemInsertPosition(m_itemsInOrder.begin(), m_itemsInOrder.end(), newPage.id(),
                             /*pageIncomplete=*/true, ordIt);

  double offset = 0.0;
  if (!m_items.empty()) {
    if (ordIt != m_itemsInOrder.end()) {
      offset = ordIt->composite->pos().y();
    } else {
      ItemsInOrder::iterator it(ordIt);
      --it;
      offset = it->composite->y() + it->composite->boundingRect().height() + SPACING;
    }
  }
  std::unique_ptr<CompositeItem> composite(getCompositeItem(0, newPage));
  composite->setPos(0.0, offset);
  composite->updateSceneRect(m_sceneRect);

  const QPointF posDelta(0.0, composite->boundingRect().height() + SPACING);

  const Item item(newPage, composite.get());
  const std::pair<ItemsInOrder::iterator, bool> ins(m_itemsInOrder.insert(ordIt, item));
  composite->setItem(&*ins.first);
  m_graphicsScene.addItem(composite.release());

  const ItemsInOrder::iterator ordEnd(m_itemsInOrder.end());
  for (; ordIt != ordEnd; ++ordIt) {
    ordIt->composite->setPos(ordIt->composite->pos() + posDelta);
    ordIt->composite->updateSceneRect(m_sceneRect);
  }

  commitSceneRect();
}  // ThumbnailSequence::Impl::insert

void ThumbnailSequence::Impl::removePages(const std::set<PageId>& pagesToRemove) {
  m_sceneRect = QRectF(0, 0, 0, 0);

  const std::set<PageId>::const_iterator to_remove_end(pagesToRemove.end());
  QPointF posDelta(0, 0);

  ItemsInOrder::iterator ordIt(m_itemsInOrder.begin());
  const ItemsInOrder::iterator ordEnd(m_itemsInOrder.end());
  while (ordIt != ordEnd) {
    if (pagesToRemove.find(ordIt->pageInfo.id()) == to_remove_end) {
      // Keeping this page.
      if (posDelta != QPointF(0, 0)) {
        ordIt->composite->setPos(ordIt->composite->pos() + posDelta);
      }
      ordIt->composite->updateSceneRect(m_sceneRect);
      ++ordIt;
    } else {
      // Removing this page.
      if (m_selectionLeader == &*ordIt) {
        m_selectionLeader = nullptr;
      }
      posDelta.ry() -= ordIt->composite->boundingRect().height() + SPACING;
      delete ordIt->composite;
      m_itemsInOrder.erase(ordIt++);
    }
  }

  commitSceneRect();
}

bool ThumbnailSequence::Impl::multipleItemsSelected() const {
  SelectedThenUnselected::iterator it(m_selectedThenUnselected.begin());
  const SelectedThenUnselected::iterator end(m_selectedThenUnselected.end());
  for (int i = 0; i < 2; ++i, ++it) {
    if ((it == end) || !it->isSelected()) {
      return false;
    }
  }
  return true;
}

void ThumbnailSequence::Impl::moveToSelected(const Item* item) {
  m_selectedThenUnselected.relocate(m_selectedThenUnselected.begin(), m_selectedThenUnselected.iterator_to(*item));
}

void ThumbnailSequence::Impl::moveToUnselected(const Item* item) {
  m_selectedThenUnselected.relocate(m_selectedThenUnselected.end(), m_selectedThenUnselected.iterator_to(*item));
}

QRectF ThumbnailSequence::Impl::selectionLeaderSceneRect() const {
  if (!m_selectionLeader) {
    return QRectF();
  }
  return m_selectionLeader->composite->mapToScene(m_selectionLeader->composite->boundingRect()).boundingRect();
}

std::set<PageId> ThumbnailSequence::Impl::selectedItems() const {
  std::set<PageId> selection;
  for (const Item& item : m_selectedThenUnselected) {
    if (!item.isSelected()) {
      break;
    }
    selection.insert(item.pageInfo.id());
  }
  return selection;
}

std::vector<PageRange> ThumbnailSequence::Impl::selectedRanges() const {
  std::vector<PageRange> ranges;

  ItemsInOrder::iterator it(m_itemsInOrder.begin());
  const ItemsInOrder::iterator end(m_itemsInOrder.end());
  while (true) {
    for (; it != end && !it->isSelected(); ++it) {
      // Skip unselected items.
    }
    if (it == end) {
      break;
    }

    ranges.emplace_back();
    PageRange& range = ranges.back();
    for (; it != end && it->isSelected(); ++it) {
      range.pages.push_back(it->pageInfo.id());
    }
  }
  return ranges;
}

void ThumbnailSequence::Impl::contextMenuRequested(const PageInfo& pageInfo, const QPoint& screenPos, bool selected) {
  emit m_owner.pageContextMenuRequested(pageInfo, screenPos, selected);
}

void ThumbnailSequence::Impl::sceneContextMenuEvent(QGraphicsSceneContextMenuEvent* evt) {
  if (!m_itemsInOrder.empty()) {
    CompositeItem* composite = m_itemsInOrder.back().composite;
    const QRectF lastThumbRect(composite->mapToScene(composite->boundingRect()).boundingRect());
    if (evt->scenePos().y() <= lastThumbRect.bottom()) {
      return;
    }
  }

  emit m_owner.pastLastPageContextMenuRequested(evt->screenPos());
}

void ThumbnailSequence::Impl::itemSelectedByUser(CompositeItem* compositeItem, const Qt::KeyboardModifiers modifiers) {
  const ItemsById::iterator idIt(m_itemsById.iterator_to(*compositeItem->item()));

  if ((modifiers & Qt::ControlModifier) || m_selectionMode) {
    selectItemWithControl(idIt);
  } else if (modifiers & Qt::ShiftModifier) {
    selectItemWithShift(idIt);
  } else {
    selectItemNoModifiers(idIt);
  }
}

void ThumbnailSequence::Impl::selectItemWithControl(const ItemsById::iterator& idIt) {
  SelectionFlags flags = SELECTED_BY_USER;

  if (!idIt->isSelected()) {
    if (m_selectionLeader) {
      m_selectionLeader->setSelectionLeader(false);
    }
    m_selectionLeader = &*idIt;
    m_selectionLeader->setSelectionLeader(true);
    moveToSelected(m_selectionLeader);

    m_owner.emitNewSelectionLeader(m_selectionLeader->pageInfo, m_selectionLeader->composite, flags);
    return;
  }

  if (!multipleItemsSelected()) {
    // Clicked on the only selected item.
    flags |= REDUNDANT_SELECTION;
    m_owner.emitNewSelectionLeader(m_selectionLeader->pageInfo, m_selectionLeader->composite, flags);
    return;
  }

  // Unselect it.
  idIt->setSelected(false);
  moveToUnselected(&*idIt);

  if (m_selectionLeader != &*idIt) {
    // The selection leader remains the same - we are done.
    return;
  }
  // Select the new selection leader among other selected items.
  m_selectionLeader = nullptr;
  flags |= AVOID_SCROLLING_TO;
  ItemsInOrder::iterator ordIt1(m_items.project<ItemsInOrderTag>(idIt));
  ItemsInOrder::iterator ordIt2(ordIt1);
  while (true) {
    if (ordIt1 != m_itemsInOrder.begin()) {
      --ordIt1;
      if (ordIt1->isSelected()) {
        m_selectionLeader = &*ordIt1;
        break;
      }
    }
    if (ordIt2 != m_itemsInOrder.end()) {
      ++ordIt2;
      if (ordIt2 != m_itemsInOrder.end()) {
        if (ordIt2->isSelected()) {
          m_selectionLeader = &*ordIt2;
          break;
        }
      }
    }
  }
  assert(m_selectionLeader);  // We had multiple selected items.

  m_selectionLeader->setSelectionLeader(true);
  // No need to moveToSelected() as it was and remains selected.

  m_owner.emitNewSelectionLeader(m_selectionLeader->pageInfo, m_selectionLeader->composite, flags);
}  // ThumbnailSequence::Impl::selectItemWithControl

void ThumbnailSequence::Impl::selectItemWithShift(const ItemsById::iterator& idIt) {
  if (!m_selectionLeader) {
    selectItemNoModifiers(idIt);
    return;
  }

  SelectionFlags flags = SELECTED_BY_USER;
  if (m_selectionLeader == &*idIt) {
    flags |= REDUNDANT_SELECTION;
  }

  // Select all the items between the selection leader and the item that was clicked.
  ItemsInOrder::iterator endpoint1(m_itemsInOrder.iterator_to(*m_selectionLeader));
  ItemsInOrder::iterator endpoint2(m_items.project<ItemsInOrderTag>(idIt));

  if (endpoint1 == endpoint2) {
    // One-element sequence, already selected.
    return;
  }
  // The problem is that we don't know which endpoint precedes the other.
  // Let's find out.
  ItemsInOrder::iterator ordIt1(endpoint1);
  ItemsInOrder::iterator ordIt2(endpoint1);
  while (true) {
    if (ordIt1 != m_itemsInOrder.begin()) {
      --ordIt1;
      if (ordIt1 == endpoint2) {
        // endpoint2 was found before endpoint1.
        std::swap(endpoint1, endpoint2);
        break;
      }
    }
    if (ordIt2 != m_itemsInOrder.end()) {
      ++ordIt2;
      if (ordIt2 != m_itemsInOrder.end()) {
        if (ordIt2 == endpoint2) {
          // endpoint2 was found after endpoint1.
          break;
        }
      }
    }
  }

  ++endpoint2;  // Make the interval inclusive.
  for (; endpoint1 != endpoint2; ++endpoint1) {
    endpoint1->setSelected(true);
    moveToSelected(&*endpoint1);
  }
  // Switch the selection leader.
  assert(m_selectionLeader);
  m_selectionLeader->setSelectionLeader(false);
  m_selectionLeader = &*idIt;
  m_selectionLeader->setSelectionLeader(true);

  m_owner.emitNewSelectionLeader(idIt->pageInfo, idIt->composite, flags);
}  // ThumbnailSequence::Impl::selectItemWithShift

void ThumbnailSequence::Impl::selectItemNoModifiers(const ItemsById::iterator& idIt) {
  if (!cancelingSelectionAccepted()) {
    return;
  }

  SelectionFlags flags = SELECTED_BY_USER;
  if (m_selectionLeader == &*idIt) {
    flags |= REDUNDANT_SELECTION;
  }
  flags |= SELECTION_CLEARED;

  clearSelection();

  m_selectionLeader = &*idIt;
  m_selectionLeader->setSelectionLeader(true);
  moveToSelected(m_selectionLeader);

  m_owner.emitNewSelectionLeader(idIt->pageInfo, idIt->composite, flags);
}

void ThumbnailSequence::Impl::clear() {
  m_selectionLeader = nullptr;

  ItemsInOrder::iterator it(m_itemsInOrder.begin());
  const ItemsInOrder::iterator end(m_itemsInOrder.end());
  while (it != end) {
    delete it->composite;
    m_itemsInOrder.erase(it++);
  }

  assert(m_graphicsScene.items().empty());

  m_sceneRect = QRectF(0.0, 0.0, 0.0, 0.0);
  commitSceneRect();
}

void ThumbnailSequence::Impl::clearSelection() {
  m_selectionLeader = nullptr;

  for (const Item& item : m_selectedThenUnselected) {
    if (!item.isSelected()) {
      break;
    }
    item.setSelected(false);
  }
}

ThumbnailSequence::Impl::ItemsInOrder::iterator ThumbnailSequence::Impl::itemInsertPosition(
    const ItemsInOrder::iterator begin,
    const ItemsInOrder::iterator end,
    const PageId& pageId,
    const bool pageIncomplete,
    const ItemsInOrder::iterator hint,
    int* distFromHint) {
  // Note that to preserve stable ordering, this function *must* return hint,
  // as long as it's an acceptable position.

  if (!m_orderProvider) {
    if (distFromHint) {
      *distFromHint = 0;
    }
    return hint;
  }

  ItemsInOrder::iterator insPos(hint);
  int dist = 0;
  // While the element immediately preceeding insPos is supposed to
  // follow the page we are inserting, move insPos one element back.
  while (insPos != begin) {
    ItemsInOrder::iterator prev(insPos);
    --prev;
    const bool precedes = m_orderProvider->precedes(pageId, pageIncomplete, prev->pageId(), prev->incompleteThumbnail);
    if (precedes) {
      insPos = prev;
      --dist;
    } else {
      break;
    }
  }

  // While the element pointed to by insPos is supposed to precede
  // the page we are inserting, advance insPos.
  while (insPos != end) {
    const bool precedes
        = m_orderProvider->precedes(insPos->pageId(), insPos->incompleteThumbnail, pageId, pageIncomplete);
    if (precedes) {
      ++insPos;
      ++dist;
    } else {
      break;
    }
  }

  if (distFromHint) {
    *distFromHint = dist;
  }
  return insPos;
}  // ThumbnailSequence::Impl::itemInsertPosition

std::unique_ptr<QGraphicsItem> ThumbnailSequence::Impl::getThumbnail(const PageInfo& pageInfo) {
  std::unique_ptr<QGraphicsItem> thumb;

  if (m_factory) {
    thumb = m_factory->get(pageInfo);
  }

  if (!thumb) {
    thumb = std::make_unique<PlaceholderThumb>(m_maxLogicalThumbSize);
  }
  return thumb;
}

std::unique_ptr<ThumbnailSequence::LabelGroup> ThumbnailSequence::Impl::getLabelGroup(const PageInfo& pageInfo) {
  const PageId& pageId = pageInfo.id();
  const QFileInfo fileInfo(pageId.imageId().filePath());
  const QString fileName(fileInfo.completeBaseName());

  QString text;
  if (fileName.size() <= 15) {
    text = fileName;
  } else {
    text = "..." + fileName.right(15);
  }
  if (pageInfo.imageId().isMultiPageFile()) {
    text = ThumbnailSequence::tr("%1 (page %2)").arg(text).arg(pageId.imageId().page());
  }

  auto normalTextItem = std::make_unique<QGraphicsSimpleTextItem>();
  normalTextItem->setText(text);

  auto boldTextItem = std::make_unique<QGraphicsSimpleTextItem>();
  boldTextItem->setText(text);

  const QBrush selectedItemTextColor = ColorSchemeManager::instance().getColorParam(
      ColorScheme::ThumbnailSequenceSelectedItemText, QApplication::palette().highlightedText());
  const QBrush selectionLeaderTextColor = ColorSchemeManager::instance().getColorParam(
      ColorScheme::ThumbnailSequenceSelectionLeaderText, selectedItemTextColor);
  boldTextItem->setBrush(selectionLeaderTextColor);

  QRectF normalTextBox(normalTextItem->boundingRect());
  QRectF boldTextBox(boldTextItem->boundingRect());
  normalTextBox.moveCenter(boldTextBox.center());
  normalTextBox.moveRight(boldTextBox.right());
  normalTextItem->setPos(normalTextBox.topLeft());
  boldTextItem->setPos(boldTextBox.topLeft());

  QIcon pageThumb;
  switch (pageId.subPage()) {
    case PageId::LEFT_PAGE:
      pageThumb = IconProvider::getInstance().getIcon("left_page_thumb");
      break;
    case PageId::RIGHT_PAGE:
      pageThumb = IconProvider::getInstance().getIcon("right_page_thumb");
      break;
    default:
      return std::make_unique<LabelGroup>(std::move(normalTextItem), std::move(boldTextItem));
  }

  const QSize size = {23, 17};
  const QPixmap pixmap = pageThumb.pixmap(size);
  const QPixmap pixmapSelected = pageThumb.pixmap(size, QIcon::Selected);
  auto pixmapItem = std::make_unique<QGraphicsPixmapItem>();
  pixmapItem->setPixmap(pixmap);
  auto pixmapItemSelected = std::make_unique<QGraphicsPixmapItem>();
  pixmapItemSelected->setPixmap(pixmapSelected);

  const int labelPixmapSpacing = 5;

  QRectF pixmapBox(pixmapItem->boundingRect());
  pixmapBox.moveTop(boldTextBox.top());
  pixmapBox.moveLeft(boldTextBox.right() + labelPixmapSpacing);
  pixmapItem->setPos(pixmapBox.topLeft());
  pixmapItemSelected->setPos(pixmapBox.topLeft());
  return std::make_unique<LabelGroup>(std::move(normalTextItem), std::move(boldTextItem), std::move(pixmapItem),
                                      std::move(pixmapItemSelected));
}  // ThumbnailSequence::Impl::getLabelGroup

std::unique_ptr<ThumbnailSequence::CompositeItem> ThumbnailSequence::Impl::getCompositeItem(const Item* item,
                                                                                            const PageInfo& pageInfo) {
  std::unique_ptr<QGraphicsItem> thumb(getThumbnail(pageInfo));
  std::unique_ptr<LabelGroup> labelGroup(getLabelGroup(pageInfo));

  auto composite = std::make_unique<CompositeItem>(*this, std::move(thumb), std::move(labelGroup));
  composite->setItem(item);
  return composite;
}

void ThumbnailSequence::Impl::commitSceneRect() {
  if (m_sceneRect.isNull()) {
    m_graphicsScene.setSceneRect(QRectF(0.0, 0.0, 1.0, 1.0));
  } else {
    m_graphicsScene.setSceneRect(m_sceneRect);
  }
}

const QSizeF& ThumbnailSequence::Impl::getMaxLogicalThumbSize() const {
  return m_maxLogicalThumbSize;
}

void ThumbnailSequence::Impl::setMaxLogicalThumbSize(const QSizeF& size) {
  m_maxLogicalThumbSize = size;
}

ThumbnailSequence::ViewMode ThumbnailSequence::Impl::getViewMode() const {
  return m_viewMode;
}

void ThumbnailSequence::Impl::setViewMode(ThumbnailSequence::ViewMode mode) {
  m_viewMode = mode;
}

void ThumbnailSequence::Impl::setSelectionModeEnabled(bool enabled) {
  m_selectionMode = enabled;
}

/*==================== ThumbnailSequence::Item ======================*/

ThumbnailSequence::Item::Item(const PageInfo& pageInfo, CompositeItem* compItem)
    : pageInfo(pageInfo),
      composite(compItem),
      incompleteThumbnail(compItem->incompleteThumbnail()),
      m_isSelected(false),
      m_isSelectionLeader(false) {}

void ThumbnailSequence::Item::setSelected(bool selected) const {
  const bool wasSelected = m_isSelected;
  const bool wasSelectionLeader = m_isSelectionLeader;
  m_isSelected = selected;
  m_isSelectionLeader = m_isSelectionLeader && selected;

  if ((wasSelected != m_isSelected) || (wasSelectionLeader != m_isSelectionLeader)) {
    composite->updateAppearence(m_isSelected, m_isSelectionLeader);
    composite->update();
  }
}

void ThumbnailSequence::Item::setSelectionLeader(bool selectionLeader) const {
  const bool wasSelected = m_isSelected;
  const bool wasSelectionLeader = m_isSelectionLeader;
  m_isSelected = m_isSelected || selectionLeader;
  m_isSelectionLeader = selectionLeader;

  if ((wasSelected != m_isSelected) || (wasSelectionLeader != m_isSelectionLeader)) {
    composite->updateAppearence(m_isSelected, m_isSelectionLeader);
    composite->update();
  }
}

/*================== ThumbnailSequence::PlaceholderThumb ====================*/

QPainterPath ThumbnailSequence::PlaceholderThumb::m_sCachedPath;

ThumbnailSequence::PlaceholderThumb::PlaceholderThumb(const QSizeF& maxSize) : m_maxSize(maxSize) {}

QRectF ThumbnailSequence::PlaceholderThumb::boundingRect() const {
  return QRectF(QPointF(0.0, 0.0), m_maxSize);
}

void ThumbnailSequence::PlaceholderThumb::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
  IncompleteThumbnail::drawQuestionMark(*painter, boundingRect());
}

/*====================== ThumbnailSequence::LabelGroup ======================*/

ThumbnailSequence::LabelGroup::LabelGroup(std::unique_ptr<QGraphicsSimpleTextItem> normalLabel,
                                          std::unique_ptr<QGraphicsSimpleTextItem> boldLabel,
                                          std::unique_ptr<QGraphicsPixmapItem> pixmap,
                                          std::unique_ptr<QGraphicsPixmapItem> pixmapSelected)
    : m_normalLabel(normalLabel.get()),
      m_boldLabel(boldLabel.get()),
      m_pixmap(pixmap.get()),
      m_pixmapSelected(pixmapSelected.get()) {
  m_normalLabel->setVisible(true);
  m_boldLabel->setVisible(false);

  boldLabel->setPos(
      boldLabel->pos().x() + 0.5 * (boldLabel->boundingRect().width() - normalLabel->boundingRect().width()),
      boldLabel->pos().y());

  addToGroup(normalLabel.release());
  addToGroup(boldLabel.release());
  if (pixmap) {
    addToGroup(pixmap.release());
  }
  if (pixmapSelected) {
    addToGroup(pixmapSelected.release());
  }
}

void ThumbnailSequence::LabelGroup::updateAppearence(bool selected, bool selectionLeader) {
  m_normalLabel->setVisible(!selectionLeader);
  m_boldLabel->setVisible(selectionLeader);

  const QBrush itemTextColor = ColorSchemeManager::instance().getColorParam(ColorScheme::ThumbnailSequenceItemText,
                                                                            QApplication::palette().text());
  const QBrush selectedItemTextColor = ColorSchemeManager::instance().getColorParam(
      ColorScheme::ThumbnailSequenceSelectedItemText, QApplication::palette().highlightedText());

  if (selectionLeader) {
    assert(selected);
  } else if (selected) {
    m_normalLabel->setBrush(selectedItemTextColor);
  } else {
    m_normalLabel->setBrush(itemTextColor);
  }

  if (m_pixmap && m_pixmapSelected) {
    m_pixmap->setVisible(!selected);
    m_pixmapSelected->setVisible(selected);
  }
}

/*==================== ThumbnailSequence::CompositeItem =====================*/

ThumbnailSequence::CompositeItem::CompositeItem(ThumbnailSequence::Impl& owner,
                                                std::unique_ptr<QGraphicsItem> thumbnail,
                                                std::unique_ptr<LabelGroup> labelGroup)
    : m_owner(owner), m_item(0), m_thumb(thumbnail.get()), m_labelGroup(labelGroup.get()) {
  const QSizeF thumbSize(thumbnail->boundingRect().size());
  const QSizeF labelSize(labelGroup->boundingRect().size());

  const int thumbLabelSpacing = 1;
  thumbnail->setPos(0.0, 0.0);
  labelGroup->setPos(thumbnail->pos().x() + 0.5 * (thumbSize.width() - labelSize.width()),
                     thumbSize.height() + thumbLabelSpacing);

  addToGroup(thumbnail.release());
  addToGroup(labelGroup.release());

  setCursor(Qt::PointingHandCursor);
  setZValue(-1);
}

bool ThumbnailSequence::CompositeItem::incompleteThumbnail() const {
  return dynamic_cast<IncompleteThumbnail*>(m_thumb) != 0;
}

void ThumbnailSequence::CompositeItem::updateSceneRect(QRectF& sceneRect) {
  QRectF rect(m_thumb->boundingRect());
  rect.translate(m_thumb->pos());
  rect.translate(pos());

  QRectF boundingRect(this->boundingRect());
  boundingRect.translate(pos());

  rect.setTop(boundingRect.top());
  rect.setBottom(boundingRect.bottom());

  sceneRect |= rect;
}

void ThumbnailSequence::CompositeItem::updateAppearence(bool selected, bool selectionLeader) {
  m_labelGroup->updateAppearence(selected, selectionLeader);
}

QRectF ThumbnailSequence::CompositeItem::boundingRect() const {
  QRectF rect(QGraphicsItemGroup::boundingRect());
  rect.adjust(-10, -5, 10, 3);
  return rect;
}

void ThumbnailSequence::CompositeItem::paint(QPainter* painter,
                                             const QStyleOptionGraphicsItem* option,
                                             QWidget* widget) {
  const QBrush selectedItemBackgroundColor = ColorSchemeManager::instance().getColorParam(
      ColorScheme::ThumbnailSequenceSelectedItemBackground, QApplication::palette().color(QPalette::Highlight));
  const QBrush selectionLeaderBackgroundColor = ColorSchemeManager::instance().getColorParam(
      ColorScheme::ThumbnailSequenceSelectionLeaderBackground, selectedItemBackgroundColor);

  if (m_item->isSelectionLeader()) {
    painter->fillRect(boundingRect(), selectionLeaderBackgroundColor);
  } else if (m_item->isSelected()) {
    painter->fillRect(boundingRect(), selectedItemBackgroundColor);
  }
}

void ThumbnailSequence::CompositeItem::mousePressEvent(QGraphicsSceneMouseEvent* const event) {
  QGraphicsItemGroup::mousePressEvent(event);

  event->accept();

  if (event->button() == Qt::LeftButton) {
    m_owner.itemSelectedByUser(this, event->modifiers());
  }
}

void ThumbnailSequence::CompositeItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* const event) {
  event->accept();  // Prevent it from propagating further.
  m_owner.contextMenuRequested(m_item->pageInfo, event->screenPos(), m_item->isSelected());
}
