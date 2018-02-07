
#ifndef SCANTAILOR_STATUSBARPANEL_H
#define SCANTAILOR_STATUSBARPANEL_H

#include <QtWidgets/QWidget>
#include <QtCore/QMutex>
#include "ui_StatusBarPanel.h"
#include "UnitsObserver.h"
#include "ImageViewInfoObserver.h"

class PageId;

class StatusBarPanel : public QWidget, public UnitsObserver, public ImageViewInfoObserver {
Q_OBJECT
private:
    mutable QMutex mutex;
    Ui::StatusBarPanel ui;

public:
    StatusBarPanel();

    ~StatusBarPanel() override = default;

public:
    void updateMousePos(const QPointF& mousePos) override;

    void updatePhysSize(const QSizeF& physSize) override;

    void updatePage(int pageNumber, const PageId& pageId);

    void clear();

    void updateUnits(Units) override;
};


#endif //SCANTAILOR_STATUSBARPANEL_H
