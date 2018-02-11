
#ifndef SCANTAILOR_IMAGEVIEWINFOOBSERVER_H
#define SCANTAILOR_IMAGEVIEWINFOOBSERVER_H


#include <QtCore/QPointF>
#include <QtCore/QSizeF>

class Dpi;

class ImageViewInfoObserver {
public:
    virtual ~ImageViewInfoObserver() = default;

    virtual void updateMousePos(const QPointF& mousePos) = 0;

    virtual void updatePhysSize(const QSizeF& physSize) = 0;

    virtual void updateDpi(const Dpi& dpi) = 0;

    virtual void clearImageViewInfo() = 0;
};


#endif //SCANTAILOR_IMAGEVIEWINFOOBSERVER_H
