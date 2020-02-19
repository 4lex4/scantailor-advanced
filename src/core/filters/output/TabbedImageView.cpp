// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <ImageViewBase.h>
#include <filters/output/TabbedImageView.h>

#include <QtWidgets/QShortcut>

#include "../../Utils.h"
#include "DespeckleView.h"

using namespace core;

namespace output {
TabbedImageView::TabbedImageView(QWidget* parent) : QTabWidget(parent), m_prevImageViewTabIndex(0) {
  connect(this, SIGNAL(currentChanged(int)), SLOT(tabChangedSlot(int)));
  setStatusTip(tr("Use Ctrl+1..5 to switch the tabs."));

  for (int i = 0; i < 5; ++i) {
    m_tabShortcuts[i] = new QShortcut(static_cast<Qt::Key>(Qt::CTRL + (Qt::Key_1 + i)), this);
    m_tabShortcuts[i]->setAutoRepeat(false);
    connect(m_tabShortcuts[i], &QShortcut::activated, std::bind(&QTabWidget::setCurrentIndex, this, i));
  }
}

void TabbedImageView::addTab(QWidget* widget, const QString& label, ImageViewTab tab) {
  QTabWidget::addTab(widget, label);
  m_registry[widget] = tab;

  if (auto* despeckleView = dynamic_cast<DespeckleView*>(widget)) {
    connect(despeckleView, &DespeckleView::imageViewCreated,
            [this](ImageViewBase*) { copyViewZoomAndPos(m_prevImageViewTabIndex, currentIndex()); });
  }
}

void TabbedImageView::setCurrentTab(const ImageViewTab tab) {
  const int cnt = count();
  for (int i = 0; i < cnt; ++i) {
    QWidget* wgt = widget(i);
    auto it = m_registry.find(wgt);
    if (it != m_registry.end()) {
      if (it->second == tab) {
        setCurrentIndex(i);
        break;
      }
    }
  }
}

void TabbedImageView::tabChangedSlot(const int idx) {
  QWidget* wgt = widget(idx);
  auto it = m_registry.find(wgt);
  if (it != m_registry.end()) {
    emit tabChanged(it->second);
  }

  copyViewZoomAndPos(m_prevImageViewTabIndex, idx);

  if (Utils::castOrFindChild<ImageViewBase*>(widget(idx)) != nullptr) {
    m_prevImageViewTabIndex = idx;
  }
}

void TabbedImageView::setImageRectMap(std::unique_ptr<TabImageRectMap> tabImageRectMap) {
  m_tabImageRectMap = std::move(tabImageRectMap);
}

void TabbedImageView::copyViewZoomAndPos(const int oldIdx, const int newIdx) const {
  if (m_tabImageRectMap == nullptr) {
    return;
  }

  if ((m_registry.find(widget(oldIdx)) == m_registry.end()) || (m_registry.find(widget(newIdx)) == m_registry.end())) {
    return;
  }
  const ImageViewTab oldViewTab = m_registry.at(widget(oldIdx));
  const ImageViewTab newViewTab = m_registry.at(widget(newIdx));

  if ((m_tabImageRectMap->find(oldViewTab) == m_tabImageRectMap->end())
      || (m_tabImageRectMap->find(newViewTab) == m_tabImageRectMap->end())) {
    return;
  }
  const QRectF& oldViewRect = m_tabImageRectMap->at(oldViewTab);
  const QRectF& newViewRect = m_tabImageRectMap->at(newViewTab);

  auto* oldImageView = Utils::castOrFindChild<ImageViewBase*>(widget(oldIdx));
  auto* newImageView = Utils::castOrFindChild<ImageViewBase*>(widget(newIdx));
  if ((oldImageView == nullptr) || (newImageView == nullptr)) {
    return;
  }
  if (oldImageView == newImageView) {
    return;
  }

  if (oldImageView->zoomLevel() != 1.0) {
    const QPointF viewFocus
        = getFocus(oldViewRect, *oldImageView->horizontalScrollBar(), *oldImageView->verticalScrollBar());
    const double zoomFactor
        = std::max(newViewRect.width(), newViewRect.height()) / std::max(oldViewRect.width(), oldViewRect.height());
    newImageView->setZoomLevel(qMax(1., oldImageView->zoomLevel() * zoomFactor));
    setFocus(*newImageView->horizontalScrollBar(), *newImageView->verticalScrollBar(), newViewRect, viewFocus);
  }
}

QPointF TabbedImageView::getFocus(const QRectF& rect, const QScrollBar& horBar, const QScrollBar& verBar) const {
  const int horBarLength = horBar.maximum() - horBar.minimum() + horBar.pageStep();
  const int verBarLength = verBar.maximum() - verBar.minimum() + verBar.pageStep();

  qreal x = ((horBar.value() + (horBar.pageStep() / 2.0)) / horBarLength) * rect.width() + rect.left();
  qreal y = ((verBar.value() + (verBar.pageStep() / 2.0)) / verBarLength) * rect.height() + rect.top();
  return QPointF(x, y);
}

void TabbedImageView::setFocus(QScrollBar& horBar, QScrollBar& verBar, const QRectF& rect, const QPointF& focal) const {
  const int horBarLength = horBar.maximum() - horBar.minimum() + horBar.pageStep();
  const int verBarLength = verBar.maximum() - verBar.minimum() + verBar.pageStep();

  auto horValue
      = (int) std::round(((focal.x() - rect.left()) / rect.width()) * horBarLength - (horBar.pageStep() / 2.0));
  auto verValue
      = (int) std::round(((focal.y() - rect.top()) / rect.height()) * verBarLength - (verBar.pageStep() / 2.0));

  horValue = qBound(horBar.minimum(), horValue, horBar.maximum());
  verValue = qBound(verBar.minimum(), verValue, verBar.maximum());

  horBar.setValue(horValue);
  verBar.setValue(verValue);
}
}  // namespace output
