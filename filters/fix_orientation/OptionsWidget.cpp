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

#include "OptionsWidget.h"
#include <cassert>
#include <utility>
#include "ApplyDialog.h"
#include "Filter.h"
#include "ProjectPages.h"
#include "Settings.h"

namespace fix_orientation {
OptionsWidget::OptionsWidget(intrusive_ptr<Settings> settings, const PageSelectionAccessor& page_selection_accessor)
    : m_ptrSettings(std::move(settings)), m_pageSelectionAccessor(page_selection_accessor) {
  setupUi(this);

  setupUiConnections();
}

OptionsWidget::~OptionsWidget() = default;

void OptionsWidget::preUpdateUI(const PageId& page_id, const OrthogonalRotation rotation) {
  removeUiConnections();

  m_pageId = page_id;
  m_rotation = rotation;
  setRotationPixmap();

  setupUiConnections();
}

void OptionsWidget::postUpdateUI(const OrthogonalRotation rotation) {
  removeUiConnections();

  setRotation(rotation);

  setupUiConnections();
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

  m_ptrSettings->applyRotation(pages, m_rotation);

  if (pages.size() > 1) {
    emit invalidateAllThumbnails();
  } else {
    for (const PageId& page_id : pages) {
      emit invalidateThumbnail(page_id);
    }
  }
}

void OptionsWidget::appliedToAllPages(const std::set<PageId>& pages) {
  m_ptrSettings->applyRotation(pages, m_rotation);
  emit invalidateAllThumbnails();
}

void OptionsWidget::setRotation(const OrthogonalRotation& rotation) {
  if (rotation == m_rotation) {
    return;
  }

  m_rotation = rotation;
  setRotationPixmap();

  m_ptrSettings->applyRotation(m_pageId.imageId(), rotation);

  emit rotated(rotation);
  emit invalidateThumbnail(m_pageId);
}

void OptionsWidget::setRotationPixmap() {
  const char* path = nullptr;

  switch (m_rotation.toDegrees()) {
    case 0:
      path = ":/icons/big-up-arrow.png";
      break;
    case 90:
      path = ":/icons/big-right-arrow.png";
      break;
    case 180:
      path = ":/icons/big-down-arrow.png";
      break;
    case 270:
      path = ":/icons/big-left-arrow.png";
      break;
    default:
      assert(!"Unreachable");
  }

  rotationIndicator->setPixmap(QPixmap(path));
}

void OptionsWidget::setupUiConnections() {
  connect(rotateLeftBtn, SIGNAL(clicked()), this, SLOT(rotateLeft()));
  connect(rotateRightBtn, SIGNAL(clicked()), this, SLOT(rotateRight()));
  connect(resetBtn, SIGNAL(clicked()), this, SLOT(resetRotation()));
  connect(applyToBtn, SIGNAL(clicked()), this, SLOT(showApplyToDialog()));
}

void OptionsWidget::removeUiConnections() {
  disconnect(rotateLeftBtn, SIGNAL(clicked()), this, SLOT(rotateLeft()));
  disconnect(rotateRightBtn, SIGNAL(clicked()), this, SLOT(rotateRight()));
  disconnect(resetBtn, SIGNAL(clicked()), this, SLOT(resetRotation()));
  disconnect(applyToBtn, SIGNAL(clicked()), this, SLOT(showApplyToDialog()));
}
}  // namespace fix_orientation