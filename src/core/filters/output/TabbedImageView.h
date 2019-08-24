// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_TABBEDIMAGEVIEW_H_
#define SCANTAILOR_OUTPUT_TABBEDIMAGEVIEW_H_

#include <QScrollBar>
#include <QTabWidget>
#include <QWidget>
#include <memory>
#include <unordered_map>
#include "ImageViewTab.h"

class ImageViewBase;

namespace output {
class TabbedImageView : public QTabWidget {
  Q_OBJECT
 private:
  using TabImageRectMap = std::unordered_map<ImageViewTab, QRectF, std::hash<int>>;

 public:
  explicit TabbedImageView(QWidget* parent = nullptr);

  void addTab(QWidget* widget, const QString& label, ImageViewTab tab);

  void setImageRectMap(std::unique_ptr<TabImageRectMap> tabImageRectMap);

 public slots:

  void setCurrentTab(ImageViewTab tab);

 signals:

  void tabChanged(ImageViewTab tab);

 protected:
  void keyReleaseEvent(QKeyEvent* event) override;

 private slots:

  void tabChangedSlot(int idx);

 private:
  void copyViewZoomAndPos(int oldIdx, int newIdx) const;

  QPointF getFocus(const QRectF& rect, const QScrollBar& horBar, const QScrollBar& verBar) const;

  void setFocus(QScrollBar& horBar, QScrollBar& verBar, const QRectF& rect, const QPointF& focal) const;

  std::unordered_map<QWidget*, ImageViewTab> m_registry;
  std::unique_ptr<TabImageRectMap> m_tabImageRectMap;
  int m_prevImageViewTabIndex;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_TABBEDIMAGEVIEW_H_
