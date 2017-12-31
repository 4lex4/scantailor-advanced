
#ifndef SCANTAILOR_STATUSBARPANEL_H
#define SCANTAILOR_STATUSBARPANEL_H

#include <QtWidgets/QWidget>
#include "ui_StatusBarPanel.h"

class StatusBarPanel : public QWidget, private Ui::StatusBarPanel {
Q_OBJECT
public:
    StatusBarPanel();

    ~StatusBarPanel() override = default;

public:
    void updateMousePos(const QPointF& mousePos);

    void updatePhysSize(const QSizeF& physSize);

    void clear();

public slots:

    void updatePageNum(int pageNumber);
};


#endif //SCANTAILOR_STATUSBARPANEL_H
