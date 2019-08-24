// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGELOADER_H_
#define SCANTAILOR_CORE_IMAGELOADER_H_

class ImageId;
class QImage;
class QString;
class QIODevice;

class ImageLoader {
 public:
  static QImage load(const QString& filePath, int pageNum = 0);

  static QImage load(const ImageId& imageId);

  static QImage load(QIODevice& ioDev, int pageNum);
};


#endif
