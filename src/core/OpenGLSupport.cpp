// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OpenGLSupport.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSettings>
#include <QSurfaceFormat>

bool OpenGLSupport::supported() {
  QSurfaceFormat format;
  format.setSamples(2);
  format.setAlphaBufferSize(8);

  QOpenGLContext context;
  context.setFormat(format);
  if (!context.create()) {
    return false;
  }
  format = context.format();

  if (format.samples() < 2) {
    return false;
  }
  if (!format.hasAlpha()) {
    return false;
  }
  return true;
}

QString OpenGLSupport::deviceName() {
  QString name;
  QOpenGLContext context;
  QOffscreenSurface surface;
  if (context.create() && (surface.create(), true) && context.makeCurrent(&surface)) {
    name = QString::fromUtf8((const char*) context.functions()->glGetString(GL_RENDERER));
    context.doneCurrent();
  }
  return name;
}
