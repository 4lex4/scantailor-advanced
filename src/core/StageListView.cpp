// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "StageListView.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>
#include <cassert>
#include <utility>
#include "BubbleAnimation.h"
#include "ChangedStateItemDelegate.h"
#include "ColorSchemeManager.h"
#include "IconProvider.h"
#include "SkinnedButton.h"
#include "StageSequence.h"

class StageListView::Model : public QAbstractTableModel {
 public:
  Model(QObject* parent, intrusive_ptr<StageSequence> stages);

  void updateBatchProcessingAnimation(int selectedRow, const QPixmap& animationFrame);

  void disableBatchProcessingAnimation();

  int columnCount(const QModelIndex& parent) const override;

  int rowCount(const QModelIndex& parent) const override;

  QVariant data(const QModelIndex& index, int role) const override;

 private:
  intrusive_ptr<StageSequence> m_stages;
  QPixmap m_curAnimationFrame;
  int m_curSelectedRow;
};


class StageListView::LeftColDelegate : public ChangedStateItemDelegate<QStyledItemDelegate> {
 public:
  explicit LeftColDelegate(StageListView* view);

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

 private:
  typedef ChangedStateItemDelegate<QStyledItemDelegate> SuperClass;

  StageListView* m_view;
};


class StageListView::RightColDelegate : public ChangedStateItemDelegate<QStyledItemDelegate> {
 public:
  explicit RightColDelegate(QObject* parent = nullptr);

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

 private:
  typedef ChangedStateItemDelegate<QStyledItemDelegate> SuperClass;
};


StageListView::StageListView(QWidget* parent)
    : QTableView(parent),
      m_sizeHint(QTableView::sizeHint()),
      m_model(nullptr),
      m_firstColDelegate(new LeftColDelegate(this)),
      m_secondColDelegate(new RightColDelegate(this)),
      m_curBatchAnimationFrame(0),
      m_timerId(0),
      m_batchProcessingPossible(false),
      m_batchProcessingInProgress(false) {
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // Prevent current item visualization. Not to be confused
  // with selected items.
  m_firstColDelegate->flagsForceDisabled(QStyle::State_HasFocus);
  m_secondColDelegate->flagsForceDisabled(QStyle::State_HasFocus);
  setItemDelegateForColumn(0, m_firstColDelegate);
  setItemDelegateForColumn(1, m_secondColDelegate);

  QHeaderView* hHeader = horizontalHeader();
  hHeader->setSectionResizeMode(QHeaderView::Stretch);
  hHeader->hide();

  QHeaderView* vHeader = verticalHeader();
  vHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
  vHeader->setSectionsMovable(false);

  const auto& iconProvider = IconProvider::getInstance();
  m_launchBtn = new SkinnedButton(iconProvider.getIcon("play"), iconProvider.getIcon("play-hovered"),
                                  iconProvider.getIcon("play-pressed"), viewport());
  static_cast<QToolButton*>(m_launchBtn)->setMinimumSize({18, 18});
  static_cast<QToolButton*>(m_launchBtn)->setIconSize({18, 18});
  m_launchBtn->setStatusTip(tr("Launch batch processing"));
  m_launchBtn->hide();

  connect(m_launchBtn, SIGNAL(clicked()), this, SIGNAL(launchBatchProcessing()));

  connect(verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(ensureSelectedRowVisible()),
          Qt::QueuedConnection);
}

StageListView::~StageListView() = default;

void StageListView::setStages(const intrusive_ptr<StageSequence>& stages) {
  if (QAbstractItemModel* m = model()) {
    // Q*View classes don't own their models.
    m->deleteLater();
  }

  m_model = new Model(this, stages);
  setModel(m_model);

  QHeaderView* hHeader = horizontalHeader();
  QHeaderView* vHeader = verticalHeader();
  hHeader->setSectionResizeMode(0, QHeaderView::Stretch);
  hHeader->setSectionResizeMode(1, QHeaderView::Fixed);
  if (vHeader->count() != 0) {
    // Make the cells in the last column square.
    const int squareSide = vHeader->sectionSize(0);
    hHeader->resizeSection(1, squareSide);
    const int reducedSquareSide = std::max(1, squareSide - 6);
    createBatchAnimationSequence(reducedSquareSide);
  } else {
    // Just to avoid special cases elsewhere.
    createBatchAnimationSequence(1);
  }
  m_curBatchAnimationFrame = 0;

  updateRowSpans();
  // Limit the vertical size to make it just enough to get
  // rid of the scrollbars, but not more.
  int height = verticalHeader()->length();
  height += this->height() - viewport()->height();
  m_sizeHint.setHeight(height);
  QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Maximum);
  sp.setVerticalStretch(1);
  setSizePolicy(sp);
  updateGeometry();
}  // StageListView::setStages

void StageListView::setBatchProcessingPossible(const bool possible) {
  if (m_batchProcessingPossible == possible) {
    return;
  }
  m_batchProcessingPossible = possible;

  if (possible) {
    placeLaunchButton(selectedRow());
  } else {
    removeLaunchButton(selectedRow());
  }
}

void StageListView::setBatchProcessingInProgress(const bool inProgress) {
  if (m_batchProcessingInProgress == inProgress) {
    return;
  }
  m_batchProcessingInProgress = inProgress;

  if (inProgress) {
    removeLaunchButton(selectedRow());
    updateRowSpans();  // Join columns.
    // Some styles (Oxygen) visually separate items in a selected row.
    // We really don't want that, so we pretend the items are not selected.
    // Same goes for hovered items.
    m_firstColDelegate->flagsForceDisabled(QStyle::State_Selected | QStyle::State_MouseOver);
    m_secondColDelegate->flagsForceDisabled(QStyle::State_Selected | QStyle::State_MouseOver);

    initiateBatchAnimationFrameRendering();
    m_timerId = startTimer(180);
  } else {
    updateRowSpans();  // Separate columns.
    placeLaunchButton(selectedRow());

    m_firstColDelegate->removeChanges(QStyle::State_Selected | QStyle::State_MouseOver);
    m_secondColDelegate->removeChanges(QStyle::State_Selected | QStyle::State_MouseOver);

    if (m_model) {
      m_model->disableBatchProcessingAnimation();
    }
    killTimer(m_timerId);
    m_timerId = 0;
  }
}  // StageListView::setBatchProcessingInProgress

void StageListView::timerEvent(QTimerEvent* event) {
  if (event->timerId() != m_timerId) {
    QTableView::timerEvent(event);

    return;
  }

  initiateBatchAnimationFrameRendering();
}

void StageListView::initiateBatchAnimationFrameRendering() {
  if (!m_model || !m_batchProcessingInProgress) {
    return;
  }

  const int selectedRow = this->selectedRow();
  if (selectedRow == -1) {
    return;
  }

  m_model->updateBatchProcessingAnimation(selectedRow, m_batchAnimationPixmaps[m_curBatchAnimationFrame]);
  if (++m_curBatchAnimationFrame == (int) m_batchAnimationPixmaps.size()) {
    m_curBatchAnimationFrame = 0;
  }
}

void StageListView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
  // Call the base version.
  QTableView::selectionChanged(selected, deselected);

  if (!deselected.isEmpty()) {
    removeLaunchButton(deselected.front().topLeft().row());
  }

  if (!selected.isEmpty()) {
    placeLaunchButton(selected.front().topLeft().row());
  }
}

void StageListView::ensureSelectedRowVisible() {
  // This loop won't run more than one iteration.
  for (const QModelIndex& idx : selectionModel()->selectedRows(0)) {
    scrollTo(idx, EnsureVisible);
  }
}

void StageListView::removeLaunchButton(const int row) {
  if (row == -1) {
    return;
  }

  m_launchBtn->hide();
}

void StageListView::placeLaunchButton(int row) {
  if (row == -1) {
    return;
  }

  const QModelIndex idx(m_model->index(row, 0));
  QRect buttonGeometry(visualRect(idx));

  // Place it to the right (assuming height is less than width).
  buttonGeometry.setLeft(buttonGeometry.right() + 1 - buttonGeometry.height());

  m_launchBtn->setGeometry(buttonGeometry);
  m_launchBtn->show();
}

void StageListView::createBatchAnimationSequence(const int squareSide) {
  const int numFrames = 8;
  m_batchAnimationPixmaps.resize(numFrames);

  const QColor headColor = ColorSchemeManager::instance().getColorParam(ColorScheme::StageListHead,
                                                                        palette().color(QPalette::Window).darker(200));
  const QColor tailColor = ColorSchemeManager::instance().getColorParam(ColorScheme::StageListTail,
                                                                        palette().color(QPalette::Window).darker(130));

  BubbleAnimation animation(numFrames);
  for (int i = 0; i < numFrames; ++i) {
    QPixmap& pixmap = m_batchAnimationPixmaps[i];
    if ((pixmap.width() != squareSide) || (pixmap.height() != squareSide)) {
      pixmap = QPixmap(squareSide, squareSide);
    }
    pixmap.fill(Qt::transparent);
    animation.nextFrame(headColor, tailColor, &pixmap);
  }
}

void StageListView::updateRowSpans() {
  if (!m_model) {
    return;
  }

  const int count = m_model->rowCount(QModelIndex());
  for (int i = 0; i < count; ++i) {
    setSpan(i, 0, 1, m_batchProcessingInProgress ? 1 : 2);
  }
}

int StageListView::selectedRow() const {
  const QModelIndexList selection(selectionModel()->selectedRows(0));
  if (selection.empty()) {
    return -1;
  }

  return selection.front().row();
}

/*========================= StageListView::Model ======================*/

StageListView::Model::Model(QObject* parent, intrusive_ptr<StageSequence> stages)
    : QAbstractTableModel(parent), m_stages(std::move(stages)), m_curSelectedRow(0) {
  assert(m_stages);
}

void StageListView::Model::updateBatchProcessingAnimation(const int selectedRow, const QPixmap& animationFrame) {
  const int maxRow = std::max(selectedRow, m_curSelectedRow);
  m_curSelectedRow = selectedRow;
  m_curAnimationFrame = animationFrame;
  emit dataChanged(index(0, 1), index(maxRow, 1));
}

void StageListView::Model::disableBatchProcessingAnimation() {
  m_curAnimationFrame = QPixmap();
  emit dataChanged(index(0, 1), index(m_curSelectedRow, 1));
}

int StageListView::Model::columnCount(const QModelIndex& parent) const {
  return 2;
}

int StageListView::Model::rowCount(const QModelIndex& parent) const {
  return m_stages->count();
}

QVariant StageListView::Model::data(const QModelIndex& index, const int role) const {
  if (role == Qt::DisplayRole) {
    if (index.column() == 0) {
      return m_stages->filterAt(index.row())->getName();
    }
  }
  if (role == Qt::UserRole) {
    if (index.column() == 1) {
      if (index.row() <= m_curSelectedRow) {
        return m_curAnimationFrame;
      }
    }
  }

  return QVariant();
}

/*================= StageListView::LeftColDelegate ===================*/

StageListView::LeftColDelegate::LeftColDelegate(StageListView* view) : SuperClass(view), m_view(view) {}

void StageListView::LeftColDelegate::paint(QPainter* painter,
                                           const QStyleOptionViewItem& option,
                                           const QModelIndex& index) const {
  SuperClass::paint(painter, option, index);

  if ((index.row() == m_view->selectedRow()) && m_view->m_launchBtn->isVisible()) {
    QRect buttonGeometry(option.rect);
    // Place it to the right (assuming height is less than width).
    buttonGeometry.setLeft(buttonGeometry.right() + 1 - buttonGeometry.height());
    m_view->m_launchBtn->setGeometry(buttonGeometry);
  }
}

/*================= StageListView::RightColDelegate ===================*/

StageListView::RightColDelegate::RightColDelegate(QObject* parent) : SuperClass(parent) {}

void StageListView::RightColDelegate::paint(QPainter* painter,
                                            const QStyleOptionViewItem& option,
                                            const QModelIndex& index) const {
  SuperClass::paint(painter, option, index);

  const QVariant var(index.data(Qt::UserRole));
  if (!var.isNull()) {
    const QPixmap pixmap(var.value<QPixmap>());
    if (!pixmap.isNull()) {
      QRect r(pixmap.rect());
      r.moveCenter(option.rect.center());
      painter->drawPixmap(r, pixmap);
    }
  }
}
