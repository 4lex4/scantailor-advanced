// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef IMAGELOADER_H_
#define IMAGELOADER_H_

class ImageId;
class QImage;
class QString;
class QIODevice;

class ImageLoader {
 public:
  static QImage load(const QString& file_path, int page_num = 0);

  static QImage load(const ImageId& image_id);

  static QImage load(QIODevice& io_dev, int page_num);
};


#endif
