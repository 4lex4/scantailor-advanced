// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OptionsWidget.h"

#include <core/IconProvider.h>

#include <cassert>
#include <utility>

#include "ApplyDialog.h"
#include "Filter.h"
#include "ProjectPages.h"
#include "Settings.h"

namespace fix_orientation {
OptionsWidget::OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& pageSelectionAccessor)
    : m_settings(std::move(settings)),
      m_pageSelectionAccessor(pageSelectionAccessor),
      m_connectionManager(std::bind(&OptionsWidget::setupUiConnections, this)) {
  setupUi(this);
  setupIcons();

  setupUiConnections();
}

OptionsWidget::~OptionsWidget() = default;

void OptionsWidget::preUpdateUI(const PageId& pageId, const OrthogonalRotation rotation) {
  auto block = m_connectionManager.getScopedBlock();

  m_pageId = pageId;
  m_rotation = rotation;
  setRotationPixmap();
}

void OptionsWidget::postUpdateUI(const OrthogonalRotation rotation) {
  auto block = m_connectionManager.getScopedBlock();

  setRotation(rotation);
}

void OptionsWidget::rotateLeft() {
  OrthogonalRotation rotation(m_rotation);
  rotation.prevClockwiseDirection();
  setRotation(rotation);
}

void OptionsWidget::rotateRight() {
  OrthogonalRotation rotation(m_rotation);
  rotation.nextClockwiseDirection();
  setRotation(rotation);
}

void OptionsWidget::resetRotation() {
  setRotation(OrthogonalRotation());
}

void OptionsWidget::showApplyToDialog() {
  auto* dialog = new ApplyDialog(this, m_pageId, m_pageSelectionAccessor);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  connect(dialog, SIGNAL(appliedTo(const std::set<PageId>&)), this, SLOT(appliedTo(const std::set<PageId>&)));
  connect(dialog, SIGNAL(appliedToAllPages(const std::set<PageId>&)), this,
          SLOT(appliedToAllPages(const std::set<PageId>&)));
  dialog->show();
}

void OptionsWidget::appliedTo(const std::set<PageId>& pages) {
  if (pages.empty()) {
    return;
  }

  m_settings->applyRotation(pages, m_rotation);

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& pageId : pages) {
      emit invalidateThumbnail(pageId);
    }
  }
}

void OptionsWidget::appliedToAllPages(const std::set<PageId>& pages) {
  m_settings->applyRotation(pages, m_rotation);
  emit invalidateAllThumbnails();
}

void OptionsWidget::setRotation(const OrthogonalRotation& rotation) {
  if (rotation == m_rotation) {
    return;
  }

  m_rotation = rotation;
  setRotationPixmap();

  m_settings->applyRotation(m_pageId.imageId(), rotation);

  emit rotated(rotation);
  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::setRotationPixmap() {
  QIcon icon;
  switch (m_rotation.toDegrees()) {
    case 0:
      icon = IconProvider::getInstance().getIcon("big-up-arrow");
      break;
    case 90:
      icon = IconProvider::getInstance().getIcon("big-right-arrow");
      break;
    case 180:
      icon = IconProvider::getInstance().getIcon("big-down-arrow");
      break;
    case 270:
      icon = IconProvider::getInstance().getIcon("big-left-arrow");
      break;
    default:
      assert(!"Unreachable");
  }
  rotationIndicator->setPixmap(icon.pixmap(32, 32));
}

#define CONNECT(...) m_connectionManager.addConnection(connect(__VA_ARGS__))

void OptionsWidget::setupUiConnections() {
  CONNECT(rotateLeftBtn, SIGNAL(clicked()), this, SLOT(rotateLeft()));
  CONNECT(rotateRightBtn, SIGNAL(clicked()), this, SLOT(rotateRight()));
  CONNECT(resetBtn, SIGNAL(clicked()), this, SLOT(resetRotation()));
  CONNECT(applyToBtn, SIGNAL(clicked()), this, SLOT(showApplyToDialog()));
}

#undef CONNECT

void OptionsWidget::setupIcons() {
  auto& iconProvider = IconProvider::getInstance();
  rotateLeftBtn->setIcon(iconProvider.getIcon("object-rotate-left"));
  rotateRightBtn->setIcon(iconProvider.getIcon("object-rotate-right"));
}
}  // namespace fix_orientation