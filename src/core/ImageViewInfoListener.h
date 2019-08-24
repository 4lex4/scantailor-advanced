// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGEVIEWINFOLISTENER_H_
#define SCANTAILOR_CORE_IMAGEVIEWINFOLISTENER_H_


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


#endif  // SCANTAILOR_CORE_IMAGEVIEWINFOLISTENER_H_
