
#include <cmath>
#include "StatusBarPanel.h"
#include "StatusBarProvider.h"
#include "MetricUnitsProvider.h"

StatusBarPanel::StatusBarPanel() {
    setupUi(this);
}

void StatusBarPanel::updateMousePos(const QPointF& mousePos) {
    QMutexLocker const locker(&m_mutex);

    if (!mousePos.isNull()) {
        double x = mousePos.x();
        double y = mousePos.y();
        MetricUnitsProvider::getInstance()->convertFrom(x, y, PIXELS);

        switch (MetricUnitsProvider::getInstance()->getMetricUnits()) {
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
    QMutexLocker const locker(&m_mutex);

    if (!physSize.isNull()) {
        double width = physSize.width();
        double height = physSize.height();
        MetricUnitsProvider::getInstance()->convertFrom(width, height, PIXELS);

        QString units;
        switch (MetricUnitsProvider::getInstance()->getMetricUnits()) {
            case PIXELS:
                width = round(width);
                height = round(height);
                units = tr("px");
                break;
            case MILLIMETRES:
                width = round(width);
                height = round(height);
                units = tr("mm");
                break;
            case CENTIMETRES:
                width = round(width * 10) / 10;
                height = round(height * 10) / 10;
                units = tr("cm");
                break;
            case INCHES:
                width = round(width * 10) / 10;
                height = round(height * 10) / 10;
                units = tr("in");
                break;
        }

        physSizeLine->setVisible(true);
        physSizeLabel->setText(QString("%1 x %2 %3").arg(width).arg(height).arg(units));
    } else {
        physSizeLabel->clear();
        physSizeLine->setVisible(false);
    }
}

void StatusBarPanel::updatePageNum(int pageNumber) {
    pageNoLabel->setText(tr("p. %1").arg(pageNumber));
}

void StatusBarPanel::clear() {
    mousePosLabel->clear();
    physSizeLabel->clear();
    pageNoLabel->clear();

    mousePosLine->setVisible(false);
    physSizeLine->setVisible(false);
}

void StatusBarPanel::updateMetricUnits(MetricUnits) {
    updateMousePos(StatusBarProvider::getInstance()->getMousePos());
    updatePhysSize(StatusBarProvider::getInstance()->getPhysSize());
}
