
#ifndef SCANTAILOR_STATUSBARPANEL_H
#define SCANTAILOR_STATUSBARPANEL_H

#include <QtWidgets/QWidget>
#include <QtCore/QMutex>
#include "ui_StatusBarPanel.h"
#include "UnitsObserver.h"
#include "ImageViewInfoObserver.h"
#include "Dpi.h"
#include "ImageViewInfoProvider.h"

class PageId;

class StatusBarPanel : public QWidget, public UnitsObserver, public ImageViewInfoObserver {
Q_OBJECT
private:
    mutable QMutex mutex;
    Ui::StatusBarPanel ui;
    QPointF mousePos;
    QSizeF physSize;
    Dpi dpi;
    ImageViewInfoProvider* infoProvider;

public:
    StatusBarPanel();

    ~StatusBarPanel() override = default;

public:
    void updateMousePos(const QPointF& mousePos) override;

    void updatePhysSize(const QSizeF& physSize) override;

    void updateDpi(const Dpi& dpi) override;

    void clearImageViewInfo() override;

    void updatePage(int pageNumber, const PageId& pageId);

    void clear();

    void updateUnits(Units) override;

    void setInfoProvider(ImageViewInfoProvider* infoProvider);

private:
    void mousePosChanged();

    void physSizeChanged();
};


#endif //SCANTAILOR_STATUSBARPANEL_H
