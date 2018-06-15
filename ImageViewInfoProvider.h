
#ifndef SCANTAILOR_STATUSBARPROVIDER_H
#define SCANTAILOR_STATUSBARPROVIDER_H

#include <QPointF>
#include <QSizeF>
#include <list>
#include <memory>
#include "Dpi.h"
#include "ImageViewInfoObserver.h"
#include "NonCopyable.h"

class ImageViewInfoProvider {
  DECLARE_NON_COPYABLE(ImageViewInfoProvider)

 private:
  std::list<ImageViewInfoObserver*> observers;
  Dpi dpi;
  QPointF mousePos;
  QSizeF physSize;

 public:
  explicit ImageViewInfoProvider(const Dpi& dpi);

  ~ImageViewInfoProvider();

  void attachObserver(ImageViewInfoObserver* observer);

  void detachObserver(ImageViewInfoObserver* observer);

  void setPhysSize(const QSizeF& physSize);

  void setMousePos(const QPointF& mousePos);

  const Dpi& getDpi() const;

  const QPointF& getMousePos() const;

  const QSizeF& getPhysSize() const;

 private:
  void physSizeChanged(const QSizeF& physSize) const;

  void mousePosChanged(const QPointF& mousePos) const;
};


#endif  // SCANTAILOR_STATUSBARPROVIDER_H
