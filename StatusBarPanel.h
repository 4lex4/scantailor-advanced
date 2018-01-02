
#ifndef SCANTAILOR_STATUSBARPANEL_H
#define SCANTAILOR_STATUSBARPANEL_H

#include <QtWidgets/QWidget>
#include <QtCore/QMutex>
#include "ui_StatusBarPanel.h"
#include "MetricUnitsObserver.h"

class StatusBarPanel : public QWidget, public MetricUnitsObserver, private Ui::StatusBarPanel {
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

    void updateMetricUnits(MetricUnits) override;

public slots:

    void updatePageNum(int pageNumber);
};


#endif //SCANTAILOR_STATUSBARPANEL_H
