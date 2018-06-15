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
#include <QTextDocument>

#ifdef Q_OS_WIN

#include <windows.h>

#else

#include <stdio.h>
#endif

bool Utils::overwritingRename(const QString& from, const QString& to) {
#ifdef Q_OS_WIN
  return MoveFileExW((WCHAR*) from.utf16(), (WCHAR*) to.utf16(), MOVEFILE_REPLACE_EXISTING) != 0;

#else
  return rename(QFile::encodeName(from).data(), QFile::encodeName(to).data()) == 0;
#endif
}

QString Utils::richTextForLink(const QString& label, const QString& target) {
  return QString::fromLatin1(
             "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\""
             "\"http://www.w3.org/TR/REC-html40/strict.dtd\">"
             "<html><head><meta name=\"qrichtext\" content=\"1\" />"
             "</head><body><p style=\"margin-top:0px; margin-bottom:0px;"
             "margin-left:0px; margin-right:0px; -qt-block-indent:0;"
             "text-indent:0px;\"><a href=\"%1\">%2</a></p></body></html>")
      .arg(target.toHtmlEscaped(), label.toHtmlEscaped());
}

void Utils::maybeCreateCacheDir(const QString& output_dir) {
  QDir(output_dir).mkdir(QString::fromLatin1("cache"));

  // QDir::mkdir() returns false if the directory already exists,
  // so to prevent confusion this function return void.
}

QString Utils::outputDirToThumbDir(const QString& output_dir) {
  return output_dir + QLatin1String("/cache/thumbs");
}

intrusive_ptr<ThumbnailPixmapCache> Utils::createThumbnailCache(const QString& output_dir) {
  const QSize max_pixmap_size(200, 200);
  const QString thumbs_cache_path(outputDirToThumbDir(output_dir));

  return make_intrusive<ThumbnailPixmapCache>(thumbs_cache_path, max_pixmap_size, 40, 5);
}
