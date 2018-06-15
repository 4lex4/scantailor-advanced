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
