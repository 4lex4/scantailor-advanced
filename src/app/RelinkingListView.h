// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_RELINKINGLISTVIEW_H_
#define SCANTAILOR_APP_RELINKINGLISTVIEW_H_

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

  void maybeDrawStatusLayer(QPainter* painter, const QModelIndex& itemIndex, const QRect& itemPaintRect);

  void drawStatusLayer(QPainter* painter);

  bool m_statusLayerDrawn;
};


#endif  // ifndef SCANTAILOR_APP_RELINKINGLISTVIEW_H_
