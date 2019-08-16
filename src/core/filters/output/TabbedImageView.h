// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OUTPUT_TABBED_IMAGE_VIEW_H_
#define OUTPUT_TABBED_IMAGE_VIEW_H_

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
  typedef std::unordered_map<ImageViewTab, QRectF, std::hash<int>> TabImageRectMap;

 public:
  explicit TabbedImageView(QWidget* parent = nullptr);

  void addTab(QWidget* widget, const QString& label, ImageViewTab tab);

  void setImageRectMap(std::unique_ptr<TabImageRectMap> tab_image_rect_map);

 public slots:

  void setCurrentTab(ImageViewTab tab);

 signals:

  void tabChanged(ImageViewTab tab);

 protected:
  void keyReleaseEvent(QKeyEvent* event) override;

 private slots:

  void tabChangedSlot(int idx);

 private:
  void copyViewZoomAndPos(int old_idx, int new_idx) const;

  QPointF getFocus(const QRectF& rect, const QScrollBar& hor_bar, const QScrollBar& ver_bar) const;

  void setFocus(QScrollBar& hor_bar, QScrollBar& ver_bar, const QRectF& rect, const QPointF& focal) const;

  std::unordered_map<QWidget*, ImageViewTab> m_registry;
  std::unique_ptr<TabImageRectMap> m_tabImageRectMap;
  int m_prevImageViewTabIndex;
};
}  // namespace output
#endif  // ifndef OUTPUT_TABBED_IMAGE_VIEW_H_
