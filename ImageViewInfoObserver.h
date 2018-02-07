
#ifndef SCANTAILOR_IMAGEVIEWINFOOBSERVER_H
#define SCANTAILOR_IMAGEVIEWINFOOBSERVER_H


#include <QtCore/QPointF>
#include <QtCore/QSizeF>

class ImageViewInfoObserver {
public:
    ImageViewInfoObserver();

    virtual ~ImageViewInfoObserver();

    virtual void updateMousePos(const QPointF& mousePos) = 0;

    virtual void updatePhysSize(const QSizeF& physSize) = 0;
};


#endif //SCANTAILOR_IMAGEVIEWINFOOBSERVER_H
