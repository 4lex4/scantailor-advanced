/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
}  // namespace output