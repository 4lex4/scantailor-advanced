
#include "StatusBarPanel.h"

StatusBarPanel::StatusBarPanel() {
    setupUi(this);
}

void StatusBarPanel::updateMousePos(const QPointF& mousePos) {
    if (!mousePos.isNull()) {
        mousePosLine->setVisible(true);
        mousePosLabel->setText(QString("%1, %2").arg(mousePos.x(), 0, 'f', 1).arg(mousePos.y(), 0, 'f', 1));
    } else {
        mousePosLabel->clear();
        mousePosLine->setVisible(false);
    }
}

void StatusBarPanel::updatePhysSize(const QSizeF& physSize) {
    if (!physSize.isNull()) {
        QString units = tr("px"); // TODO: DPI recalculation here

        physSizeLine->setVisible(true);
        physSizeLabel->setText(QString("%1 x %2 %3").arg(physSize.width()).arg(physSize.height()).arg(units));
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
