
#ifndef SCANTAILOR_STATUSBARPROVIDER_H
#define SCANTAILOR_STATUSBARPROVIDER_H

#include <memory>
#include <list>
#include <QPointF>
#include <QSizeF>
#include "ImageViewInfoObserver.h"

class ImageViewInfoProvider {
private:
    static std::unique_ptr<ImageViewInfoProvider> instance;

    std::list<ImageViewInfoObserver*> observers;
    QSizeF physSize;
    QPointF mousePos;

    ImageViewInfoProvider();

public:
    static ImageViewInfoProvider* getInstance();

    void attachObserver(ImageViewInfoObserver* observer);

    void detachObserver(ImageViewInfoObserver* observer);

    const QSizeF& getPhysSize() const;

    void setPhysSize(const QSizeF& physSize);

    const QPointF& getMousePos() const;

    void setMousePos(const QPointF& mousePos);

protected:
    void physSizeChanged();

    void mousePosChanged();
};


#endif //SCANTAILOR_STATUSBARPROVIDER_H
