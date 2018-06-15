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

#include <ImageViewBase.h>
#include <filters/output/TabbedImageView.h>
#include <QKeyEvent>
#include "../../Utils.h"
#include "DespeckleView.h"

namespace output {
TabbedImageView::TabbedImageView(QWidget* parent) : QTabWidget(parent), m_prevImageViewTabIndex(0) {
  connect(this, SIGNAL(currentChanged(int)), SLOT(tabChangedSlot(int)));
  setStatusTip(tr("Use Ctrl+1..5 to switch the tabs."));
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

void TabbedImageView::setImageRectMap(std::unique_ptr<TabImageRectMap> tab_image_rect_map) {
  m_tabImageRectMap = std::move(tab_image_rect_map);
}

void TabbedImageView::copyViewZoomAndPos(const int old_idx, const int new_idx) const {
  if (m_tabImageRectMap == nullptr) {
    return;
  }

  if ((m_registry.find(widget(old_idx)) == m_registry.end())
      || (m_registry.find(widget(new_idx)) == m_registry.end())) {
    return;
  }
  const ImageViewTab old_view_tab = m_registry.at(widget(old_idx));
  const ImageViewTab new_view_tab = m_registry.at(widget(new_idx));

  if ((m_tabImageRectMap->find(old_view_tab) == m_tabImageRectMap->end())
      || (m_tabImageRectMap->find(new_view_tab) == m_tabImageRectMap->end())) {
    return;
  }
  const QRectF& old_view_rect = m_tabImageRectMap->at(old_view_tab);
  const QRectF& new_view_rect = m_tabImageRectMap->at(new_view_tab);

  auto* old_image_view = Utils::castOrFindChild<ImageViewBase*>(widget(old_idx));
  auto* new_image_view = Utils::castOrFindChild<ImageViewBase*>(widget(new_idx));
  if ((old_image_view == nullptr) || (new_image_view == nullptr)) {
    return;
  }
  if (old_image_view == new_image_view) {
    return;
  }

  if (old_image_view->zoomLevel() != 1.0) {
    const QPointF view_focus
        = getFocus(old_view_rect, *old_image_view->horizontalScrollBar(), *old_image_view->verticalScrollBar());
    const double zoom_factor = std::max(new_view_rect.width(), new_view_rect.height())
                               / std::max(old_view_rect.width(), old_view_rect.height());
    new_image_view->setZoomLevel(qMax(1., old_image_view->zoomLevel() * zoom_factor));
    setFocus(*new_image_view->horizontalScrollBar(), *new_image_view->verticalScrollBar(), new_view_rect, view_focus);
  }
}

QPointF TabbedImageView::getFocus(const QRectF& rect, const QScrollBar& hor_bar, const QScrollBar& ver_bar) const {
  const int hor_bar_length = hor_bar.maximum() - hor_bar.minimum() + hor_bar.pageStep();
  const int ver_bar_length = ver_bar.maximum() - ver_bar.minimum() + ver_bar.pageStep();

  qreal x = ((hor_bar.value() + (hor_bar.pageStep() / 2.0)) / hor_bar_length) * rect.width() + rect.left();
  qreal y = ((ver_bar.value() + (ver_bar.pageStep() / 2.0)) / ver_bar_length) * rect.height() + rect.top();

  return QPointF(x, y);
}

void TabbedImageView::setFocus(QScrollBar& hor_bar,
                               QScrollBar& ver_bar,
                               const QRectF& rect,
                               const QPointF& focal) const {
  const int hor_bar_length = hor_bar.maximum() - hor_bar.minimum() + hor_bar.pageStep();
  const int ver_bar_length = ver_bar.maximum() - ver_bar.minimum() + ver_bar.pageStep();

  auto hor_value
      = (int) std::round(((focal.x() - rect.left()) / rect.width()) * hor_bar_length - (hor_bar.pageStep() / 2.0));
  auto ver_value
      = (int) std::round(((focal.y() - rect.top()) / rect.height()) * ver_bar_length - (ver_bar.pageStep() / 2.0));

  hor_value = qBound(hor_bar.minimum(), hor_value, hor_bar.maximum());
  ver_value = qBound(ver_bar.minimum(), ver_value, ver_bar.maximum());

  hor_bar.setValue(hor_value);
  ver_bar.setValue(ver_value);
}

void TabbedImageView::keyReleaseEvent(QKeyEvent* event) {
  event->setAccepted(false);
  if (event->modifiers() != Qt::ControlModifier) {
    return;
  }

  switch (event->key()) {
    case Qt::Key_1:
      setCurrentIndex(0);
      event->accept();
      break;
    case Qt::Key_2:
      setCurrentIndex(1);
      event->accept();
      break;
    case Qt::Key_3:
      setCurrentIndex(2);
      event->accept();
      break;
    case Qt::Key_4:
      setCurrentIndex(3);
      event->accept();
      break;
    case Qt::Key_5:
      setCurrentIndex(4);
      event->accept();
      break;
    default:
      break;
  }
}
}  // namespace output
