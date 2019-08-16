// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Utils.h"
#include <QDir>
#include <QString>
#include <QTransform>
#include "Dpi.h"

namespace output {
QString Utils::automaskDir(const QString& out_dir) {
  return QDir(out_dir).absoluteFilePath("cache/automask");
}

QString Utils::predespeckleDir(const QString& out_dir) {
  return QDir(out_dir).absoluteFilePath("cache/predespeckle");
}

QString Utils::specklesDir(const QString& out_dir) {
  return QDir(out_dir).absoluteFilePath("cache/speckles");
}

QTransform Utils::scaleFromToDpi(const Dpi& from, const Dpi& to) {
  QTransform xform;
  xform.scale((double) to.horizontal() / from.horizontal(), (double) to.vertical() / from.vertical());

  return xform;
}

QString Utils::foregroundDir(const QString& out_dir) {
  return QDir(out_dir).absoluteFilePath("foreground");
}

QString Utils::backgroundDir(const QString& out_dir) {
  return QDir(out_dir).absoluteFilePath("background");
}

QString Utils::originalBackgroundDir(const QString& out_dir) {
  return QDir(out_dir).absoluteFilePath("original_background");
}

QTransform Utils::rotate(double degrees, const QRect& image_rect) {
  if (degrees == 0.0) {
    return QTransform();
  }

  QTransform rotate_xform;
  const QPointF origin = QRectF(image_rect).center();
  rotate_xform.translate(-origin.x(), -origin.y());
  rotate_xform *= QTransform().rotate(degrees);
  rotate_xform *= QTransform().translate(origin.x(), origin.y());

  return rotate_xform;
}
}  // namespace output