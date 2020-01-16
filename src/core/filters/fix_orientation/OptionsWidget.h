// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FIX_ORIENTATION_OPTIONSWIDGET_H_
#define SCANTAILOR_FIX_ORIENTATION_OPTIONSWIDGET_H_

#include <list>

#include "FilterOptionsWidget.h"
#include "OrthogonalRotation.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "intrusive_ptr.h"
#include "ui_OptionsWidget.h"

namespace fix_orientation {
class Settings;

class OptionsWidget : public FilterOptionsWidget, private Ui::OptionsWidget {
  Q_OBJECT
 public:
  OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& pageSelectionAccessor);

  ~OptionsWidget() override;

  void preUpdateUI(const PageId& pageId, OrthogonalRotation rotation);

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

  void setupIcons();

  intrusive_ptr<Settings> m_settings;
  PageSelectionAccessor m_pageSelectionAccessor;
  PageId m_pageId;
  OrthogonalRotation m_rotation;

  std::list<QMetaObject::Connection> m_connectionList;
};
}  // namespace fix_orientation
#endif  // ifndef SCANTAILOR_FIX_ORIENTATION_OPTIONSWIDGET_H_
