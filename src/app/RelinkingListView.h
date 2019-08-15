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

#ifndef RELINKING_LIST_VIEW_H_
#define RELINKING_LIST_VIEW_H_

#include <QListView>

class QPainter;
class QRect;
class QModelIndex;

class RelinkingListView : public QListView {
 public:
  explicit RelinkingListView(QWidget* parent = nullptr);

 protected:
  void paintEvent(QPaintEvent* e) override;

 private:
  class Delegate;
  class IndicationGroup;

  class GroupAggregator;

  void maybeDrawStatusLayer(QPainter* painter, const QModelIndex& item_index, const QRect& item_paint_rect);

  void drawStatusLayer(QPainter* painter);

  bool m_statusLayerDrawn;
};


#endif  // ifndef RELINKING_LIST_VIEW_H_
