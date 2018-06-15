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

#ifndef BASICIMAGEVIEW_H_
#define BASICIMAGEVIEW_H_

#include <QImage>
#include "DragHandler.h"
#include "ImagePixmapUnion.h"
#include "ImageViewBase.h"
#include "Margins.h"
#include "ZoomHandler.h"

class BasicImageView : public ImageViewBase {
  Q_OBJECT
 public:
  explicit BasicImageView(const QImage& image,
                          const ImagePixmapUnion& downscaled_image = ImagePixmapUnion(),
                          const Margins& margins = Margins());

  ~BasicImageView() override;

 private:
  DragHandler m_dragHandler;
  ZoomHandler m_zoomHandler;
};


#endif
