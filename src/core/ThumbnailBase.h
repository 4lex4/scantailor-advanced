// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_THUMBNAILBASE_H_
#define SCANTAILOR_CORE_THUMBNAILBASE_H_

#include <QGraphicsItem>
#include <QRectF>
#include <QSizeF>
#include <QTransform>
#include <memory>

#include "ImageId.h"
#include "ImageTransformation.h"
#include "NonCopyable.h"
#include "ThumbnailPixmapCache.h"

class ThumbnailLoadResult;

class ThumbnailBase : public QGraphicsItem {
  DECLARE_NON_COPYABLE(ThumbnailBase)

 public:
  ThumbnailBase(std::shared_ptr<ThumbnailPixmapCache> thumbnailCache,
                const QSizeF& maxSize,
                const ImageId& imageId,
                const ImageTransformation& imageXform);

  ThumbnailBase(std::shared_ptr<ThumbnailPixmapCache> thumbnailCache,
                const QSizeF& maxSize,
                const ImageId& imageId,
                const ImageTransformation& imageXform,
                QRectF displayArea);

  ~ThumbnailBase() override;

  QRectF boundingRect() const override;

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

 protected:
  /**
   * \brief A hook to allow subclasses to draw over the thumbnail.
   *
   * \param painter The painter to be used for drawing.
   * \param imageToDisplay Can be supplied to \p painter as a world
   *        transformation in order to draw in virtual image coordinates,
   *        that is in coordinates we get after applying the
   *        ImageTransformation to the physical image coordinates.
   *        We are talking about full-sized images here.
   * \param thumbToDisplay Can be supplied to \p painter as a world
   *        transformation in order to draw in thumbnail coordinates.
   *        Valid thumbnail coordinates lie within this->boundingRect().
   *
   * The painter is configured for drawing in thumbnail coordinates by
   * default.  No clipping is configured, but drawing should be
   * restricted to this->boundingRect().  Note that it's not necessary
   * for subclasses to restore the painter state.
   */
  virtual void paintOverImage(QPainter& painter, const QTransform& imageToDisplay, const QTransform& thumbToDisplay) {}

  /**
   * \brief This is the same as paintOverImage().
   *        The only difference is that the painted content will be cropped with the image.
   */
  virtual void prePaintOverImage(QPainter& painter,
                                 const QTransform& imageToDisplay,
                                 const QTransform& thumbToDisplay) {}

  virtual void paintDeviant(QPainter& painter);

  void setImageXform(const ImageTransformation& imageXform);

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

  std::shared_ptr<ThumbnailPixmapCache> m_thumbnailCache;
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

  std::shared_ptr<LoadCompletionHandler> m_completionHandler;
};


#endif  // ifndef SCANTAILOR_CORE_THUMBNAILBASE_H_
