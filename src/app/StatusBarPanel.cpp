// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "StatusBarPanel.h"

#include <QtCore/QFileInfo>
#include <cmath>

#include "ImageViewInfoProvider.h"
#include "PageId.h"
#include "UnitsProvider.h"

StatusBarPanel::StatusBarPanel() {
  ui.setupUi(this);
}

void StatusBarPanel::onMousePosChanged(const QPointF& mousePos) {
  StatusBarPanel::m_mousePos = mousePos;
  mousePosChanged();
}

void StatusBarPanel::onPhysSizeChanged(const QSizeF& physSize) {
  StatusBarPanel::m_physSize = physSize;
  physSizeChanged();
}

void StatusBarPanel::onDpiChanged(const Dpi& dpi) {
  StatusBarPanel::m_dpi = dpi;
}

void StatusBarPanel::onProviderStopped() {
  onMousePosChanged(QPointF());
  onPhysSizeChanged(QRectF().size());
  m_dpi = Dpi();
}

void StatusBarPanel::updatePage(int pageNumber, size_t pageCount, const PageId& pageId) {
  ui.pageNoLabel->setText(tr("p. %1 / %2").arg(pageNumber).arg(pageCount));

  QString pageFileInfo = QFileInfo(pageId.imageId().filePath()).completeBaseName();
  if (pageFileInfo.size() > 15) {
    pageFileInfo = "..." + pageFileInfo.right(13);
  }
  if (pageId.subPage() != PageId::SINGLE_PAGE) {
    pageFileInfo = pageFileInfo.right(11) + ((pageId.subPage() == PageId::LEFT_PAGE) ? tr(" [L]") : tr(" [R]"));
  }

  ui.pageInfoLine->setVisible(true);
  ui.pageInfo->setText(pageFileInfo);
}

void StatusBarPanel::clear() {
  ui.mousePosLabel->clear();
  ui.physSizeLabel->clear();
  ui.pageNoLabel->clear();
  ui.pageInfo->clear();

  ui.mousePosLine->setVisible(false);
  ui.physSizeLine->setVisible(false);
  ui.pageInfoLine->setVisible(false);
}

void StatusBarPanel::onUnitsChanged(Units) {
  mousePosChanged();
  physSizeChanged();
}

void StatusBarPanel::mousePosChanged() {
  if (!m_mousePos.isNull() && !m_dpi.isNull()) {
    double x = m_mousePos.x();
    double y = m_mousePos.y();
    UnitsProvider::getInstance().convertFrom(x, y, PIXELS, m_dpi);

    switch (UnitsProvider::getInstance().getUnits()) {
      case PIXELS:
      case MILLIMETRES:
        x = std::ceil(x);
        y = std::ceil(y);
        break;
      default:
        x = std::ceil(x * 10) / 10;
        y = std::ceil(y * 10) / 10;
        break;
    }

    ui.mousePosLine->setVisible(true);
    ui.mousePosLabel->setText(QString("%1, %2").arg(x).arg(y));
  } else {
    ui.mousePosLabel->clear();
    ui.mousePosLine->setVisible(false);
  }
}

void StatusBarPanel::physSizeChanged() {
  if (!m_physSize.isNull() && !m_dpi.isNull()) {
    double width = m_physSize.width();
    double height = m_physSize.height();
    UnitsProvider::getInstance().convertFrom(width, height, PIXELS, m_dpi);

    const Units units = UnitsProvider::getInstance().getUnits();
    switch (units) {
      case PIXELS:
        width = std::round(width);
        height = std::round(height);
        break;
      case MILLIMETRES:
        width = std::round(width);
        height = std::round(height);
        break;
      case CENTIMETRES:
        width = std::round(width * 10) / 10;
        height = std::round(height * 10) / 10;
        break;
      case INCHES:
        width = std::round(width * 10) / 10;
        height = std::round(height * 10) / 10;
        break;
    }

    ui.physSizeLine->setVisible(true);
    ui.physSizeLabel->setText(QString("%1 x %2 %3").arg(width).arg(height).arg(unitsToLocalizedString(units)));
  } else {
    ui.physSizeLabel->clear();
    ui.physSizeLine->setVisible(false);
  }
}
