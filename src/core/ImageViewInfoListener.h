
#ifndef SCANTAILOR_IMAGEVIEWINFOLISTENER_H
#define SCANTAILOR_IMAGEVIEWINFOLISTENER_H


#include <QtCore/QPointF>
#include <QtCore/QSizeF>

class Dpi;

class ImageViewInfoListener {
 public:
  virtual ~ImageViewInfoListener() = default;

  virtual void onMousePosChanged(const QPointF& mousePos) = 0;

  virtual void onPhysSizeChanged(const QSizeF& physSize) = 0;

  virtual void onDpiChanged(const Dpi& dpi) = 0;

  virtual void onProviderStopped() = 0;
};


#endif  //SCANTAILOR_IMAGEVIEWINFOLISTENER_H
