// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Utils.h"

#include <QDir>
#include <QString>
#include <QTransform>

#include "Dpi.h"

namespace output {
QString Utils::automaskDir(const QString& outDir) {
  return QDir(outDir).absoluteFilePath("cache/automask");
}

QString Utils::predespeckleDir(const QString& outDir) {
  return QDir(outDir).absoluteFilePath("cache/predespeckle");
}

QString Utils::specklesDir(const QString& outDir) {
  return QDir(outDir).absoluteFilePath("cache/speckles");
}

QTransform Utils::scaleFromToDpi(const Dpi& from, const Dpi& to) {
  QTransform xform;
  xform.scale((double) to.horizontal() / from.horizontal(), (double) to.vertical() / from.vertical());
  return xform;
}

QString Utils::foregroundDir(const QString& outDir) {
  return QDir(outDir).absoluteFilePath("foreground");
}

QString Utils::backgroundDir(const QString& outDir) {
  return QDir(outDir).absoluteFilePath("background");
}

QString Utils::originalBackgroundDir(const QString& outDir) {
  return QDir(outDir).absoluteFilePath("original_background");
}

QTransform Utils::rotate(double degrees, const QRect& imageRect) {
  if (degrees == 0.0) {
    return QTransform();
  }

  QTransform rotateXform;
  const QPointF origin = QRectF(imageRect).center();
  rotateXform.translate(-origin.x(), -origin.y());
  rotateXform *= QTransform().rotate(degrees);
  rotateXform *= QTransform().translate(origin.x(), origin.y());
  return rotateXform;
}
}  // namespace output