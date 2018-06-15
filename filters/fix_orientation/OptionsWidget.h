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

#ifndef FIX_ORIENTATION_OPTIONSWIDGET_H_
#define FIX_ORIENTATION_OPTIONSWIDGET_H_

#include "FilterOptionsWidget.h"
#include "OrthogonalRotation.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "intrusive_ptr.h"
#include "ui_OrientationOptionsWidget.h"

namespace fix_orientation {
class Settings;

class OptionsWidget : public FilterOptionsWidget, private Ui::OrientationOptionsWidget {
  Q_OBJECT
 public:
  OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& page_selection_accessor);

  ~OptionsWidget() override;

  void preUpdateUI(const PageId& page_id, OrthogonalRotation rotation);

  void postUpdateUI(OrthogonalRotation rotation);

 signals:

  void rotated(OrthogonalRotation rotation);

 private slots:

  void rotateLeft();

  void rotateRight();

  void resetRotation();

  void showApplyToDialog();

  void appliedTo(const std::set<PageId>& pages);

  void appliedToAllPages(const std::set<PageId>& pages);

 private:
  void setRotation(const OrthogonalRotation& rotation);

  void setRotationPixmap();

  void setupUiConnections();

  void removeUiConnections();

  intrusive_ptr<Settings> m_ptrSettings;
  PageSelectionAccessor m_pageSelectionAccessor;
  PageId m_pageId;
  OrthogonalRotation m_rotation;
};
}  // namespace fix_orientation
#endif  // ifndef FIX_ORIENTATION_OPTIONSWIDGET_H_
