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

#ifndef THUMBNAILBASE_H_
#define THUMBNAILBASE_H_

#include <QGraphicsItem>
#include <QRectF>
#include <QSizeF>
#include <QTransform>
#include "ImageId.h"
#include "ImageTransformation.h"
#include "NonCopyable.h"
#include "ThumbnailPixmapCache.h"
#include "intrusive_ptr.h"

class ThumbnailLoadResult;

class ThumbnailBase : public QGraphicsItem {
  DECLARE_NON_COPYABLE(ThumbnailBase)

 public:
  ThumbnailBase(intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
                const QSizeF& max_size,
                const ImageId& image_id,
                const ImageTransformation& image_xform);

  ThumbnailBase(intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
                const QSizeF& max_size,
                const ImageId& image_id,
                const ImageTransformation& image_xform,
                QRectF displayArea);

  ~ThumbnailBase() override;

  QRectF boundingRect() const override;

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

 protected:
  /**
   * \brief A hook to allow subclasses to draw over the thumbnail.
   *
   * \param painter The painter to be used for drawing.
   * \param image_to_display Can be supplied to \p painter as a world
   *        transformation in order to draw in virtual image coordinates,
   *        that is in coordinates we get after applying the
   *        ImageTransformation to the physical image coordinates.
   *        We are talking about full-sized images here.
   * \param thumb_to_display Can be supplied to \p painter as a world
   *        transformation in order to draw in thumbnail coordinates.
   *        Valid thumbnail coordinates lie within this->boundingRect().
   *
   * The painter is configured for drawing in thumbnail coordinates by
   * default.  No clipping is configured, but drawing should be
   * restricted to this->boundingRect().  Note that it's not necessary
   * for subclasses to restore the painter state.
   */
  virtual void paintOverImage(QPainter& painter,
                              const QTransform& image_to_display,
                              const QTransform& thumb_to_display) {}

  /**
   * \brief This is the same as paintOverImage().
   *        The only difference is that the painted content will be cropped with the image.
   */
  virtual void prePaintOverImage(QPainter& painter,
                                 const QTransform& image_to_display,
                                 const QTransform& thumb_to_display) {}

  virtual void paintDeviant(QPainter& painter);

  /**
   * By default, the image is clipped by both the crop area (as defined
   * by imageXform().resultingPostCropArea()), and the physical boundaries of
   * the image itself.  Basically a point won't be clipped only if it's both
   * inside of the crop area and inside the image.
   * Extended clipping area only includes the cropping area, so it's possible
   * to draw outside of the image but inside the crop area.
   */
  void setExtendedClipArea(bool enabled) { m_extendedClipArea = enabled; }

  void setImageXform(const ImageTransformation& image_xform);

  const ImageTransformation& imageXform() const { return m_imageXform; }

  /**
   * \brief Converts from the virtual image coordinates to thumbnail image coordinates.
   *
   * Virtual image coordinates is what you get after ImageTransformation.
   */
  const QTransform& virtToThumb() const { return m_postScaleXform; }

 private:
  class LoadCompletionHandler;

  void handleLoadResult(const ThumbnailLoadResult& result);

  intrusive_ptr<ThumbnailPixmapCache> m_ptrThumbnailCache;
  QSizeF m_maxSize;
  ImageId m_imageId;
  ImageTransformation m_imageXform;
  QRectF m_boundingRect;
  QRectF m_displayArea;

  /**
   * Transforms virtual image coordinates into thumbnail coordinates.
   * Valid thumbnail coordinates lie within this->boundingRect().
   */
  QTransform m_postScaleXform;

  std::shared_ptr<LoadCompletionHandler> m_ptrCompletionHandler;
  bool m_extendedClipArea;
};


#endif  // ifndef THUMBNAILBASE_H_
