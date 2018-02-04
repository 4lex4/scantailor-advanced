
#include <cmath>
#include <QtCore/QFileInfo>
#include "StatusBarPanel.h"
#include "StatusBarProvider.h"
#include "UnitsProvider.h"
#include "PageId.h"

StatusBarPanel::StatusBarPanel() {
    setupUi(this);
}

void StatusBarPanel::updateMousePos(const QPointF& mousePos) {
    const QMutexLocker locker(&m_mutex);

    if (!mousePos.isNull()) {
        double x = mousePos.x();
        double y = mousePos.y();
        UnitsProvider::getInstance()->convertFrom(x, y, PIXELS);

        switch (UnitsProvider::getInstance()->getUnits()) {
            case PIXELS:
            case MILLIMETRES:
                x = ceil(x);
                y = ceil(y);
                break;
            default:
                x = ceil(x * 10) / 10;
                y = ceil(y * 10) / 10;
                break;
        }

        mousePosLine->setVisible(true);
        mousePosLabel->setText(QString("%1, %2").arg(x).arg(y));
    } else {
        mousePosLabel->clear();
        mousePosLine->setVisible(false);
    }
}

void StatusBarPanel::updatePhysSize(const QSizeF& physSize) {
    const QMutexLocker locker(&m_mutex);

    if (!physSize.isNull()) {
        double width = physSize.width();
        double height = physSize.height();
        UnitsProvider::getInstance()->convertFrom(width, height, PIXELS);

        const Units units = UnitsProvider::getInstance()->getUnits();
        switch (units) {
            case PIXELS:
                width = round(width);
                height = round(height);
                break;
            case MILLIMETRES:
                width = round(width);
                height = round(height);
                break;
            case CENTIMETRES:
                width = round(width * 10) / 10;
                height = round(height * 10) / 10;
                break;
            case INCHES:
                width = round(width * 10) / 10;
                height = round(height * 10) / 10;
                break;
        }

        physSizeLine->setVisible(true);
        physSizeLabel->setText(QString("%1 x %2 %3").arg(width).arg(height).arg(unitsToString(units)));
    } else {
        physSizeLabel->clear();
        physSizeLine->setVisible(false);
    }
}

void StatusBarPanel::updatePage(int pageNumber, const PageId& pageId) {
    pageNoLabel->setText(tr("p. %1").arg(pageNumber));

    QString pageFileInfo = QFileInfo(pageId.imageId().filePath()).baseName();
    if (pageFileInfo.size() > 15) {
        pageFileInfo = "..." + pageFileInfo.right(13);
    }
    if (pageId.subPage() != PageId::SINGLE_PAGE) {
        pageFileInfo = pageFileInfo.right(11) + ((pageId.subPage() == PageId::LEFT_PAGE) ? tr(" [L]") : tr(" [R]"));
    }

    pageInfoLine->setVisible(true);
    pageInfo->setText(pageFileInfo);
}

void StatusBarPanel::clear() {
    mousePosLabel->clear();
    physSizeLabel->clear();
    pageNoLabel->clear();
    pageInfo->clear();

    mousePosLine->setVisible(false);
    physSizeLine->setVisible(false);
    pageInfoLine->setVisible(false);
}

void StatusBarPanel::updateUnits(Units) {
    updateMousePos(StatusBarProvider::getInstance()->getMousePos());
    updatePhysSize(StatusBarProvider::getInstance()->getPhysSize());
}
