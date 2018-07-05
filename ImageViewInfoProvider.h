
#ifndef SCANTAILOR_STATUSBARPROVIDER_H
#define SCANTAILOR_STATUSBARPROVIDER_H

#include <foundation/NonCopyable.h>
#include <QPointF>
#include <QSizeF>
#include <list>
#include <memory>
#include "Dpi.h"
#include "ImageViewInfoObserver.h"
#include "NonCopyable.h"

class ImageViewInfoProvider {
  DECLARE_NON_COPYABLE(ImageViewInfoProvider)
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

  std::list<ImageViewInfoObserver*> m_observers;
  Dpi m_dpi;
  QPointF m_mousePos;
  QSizeF m_physSize;
};


#endif  // SCANTAILOR_STATUSBARPROVIDER_H
