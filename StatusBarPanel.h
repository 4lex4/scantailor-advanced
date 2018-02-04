
#ifndef SCANTAILOR_STATUSBARPANEL_H
#define SCANTAILOR_STATUSBARPANEL_H

#include <QtWidgets/QWidget>
#include <QtCore/QMutex>
#include "ui_StatusBarPanel.h"
#include "UnitsObserver.h"

class PageId;

class StatusBarPanel : public QWidget, public UnitsObserver, private Ui::StatusBarPanel {
Q_OBJECT
private:
    mutable QMutex m_mutex;

public:
    StatusBarPanel();

    ~StatusBarPanel() override = default;

public:
    void updateMousePos(const QPointF& mousePos);

    void updatePhysSize(const QSizeF& physSize);

    void clear();

    void updateUnits(Units) override;

public slots:

    void updatePage(int pageNumber, const PageId& pageId);
};


#endif //SCANTAILOR_STATUSBARPANEL_H
