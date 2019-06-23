
#ifndef SCANTAILOR_STATUSBARPROVIDER_H
#define SCANTAILOR_STATUSBARPROVIDER_H

#include <foundation/NonCopyable.h>
#include <QPointF>
#include <QSizeF>
#include <list>
#include <memory>
#include "Dpi.h"
#include "ImageViewInfoListener.h"
#include "NonCopyable.h"

class ImageViewInfoProvider {
  DECLARE_NON_COPYABLE(ImageViewInfoProvider)
 public:
  explicit ImageViewInfoProvider(const Dpi& dpi);

  ~ImageViewInfoProvider();

  void addListener(ImageViewInfoListener* listener);

  void removeListener(ImageViewInfoListener* listener);

  void setPhysSize(const QSizeF& physSize);

  void setMousePos(const QPointF& mousePos);

  const Dpi& getDpi() const;

  const QPointF& getMousePos() const;

  const QSizeF& getPhysSize() const;

 private:
  void physSizeChanged(const QSizeF& physSize) const;

  void mousePosChanged(const QPointF& mousePos) const;

  std::list<ImageViewInfoListener*> m_infoListeners;
  Dpi m_dpi;
  QPointF m_mousePos;
  QSizeF m_physSize;
};


#endif  // SCANTAILOR_STATUSBARPROVIDER_H
