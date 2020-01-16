// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGEMETADATALOADER_H_
#define SCANTAILOR_CORE_IMAGEMETADATALOADER_H_

#include <vector>

#include "VirtualFunction.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class QString;
class QIODevice;
class ImageMetadata;

class ImageMetadataLoader : public ref_countable {
 public:
  enum Status {
    LOADED,                /**< Loaded successfully */
    NO_IMAGES,             /**< File contained no images. */
    FORMAT_NOT_RECOGNIZED, /**< File format not recognized. */
    GENERIC_ERROR          /**< Some other error has occured. */
  };

  /**
   * \brief Registers a loader for a particular image format.
   *
   * This function may not be called before main() or after additional
   * threads have been created.
   */
  static void registerLoader(intrusive_ptr<ImageMetadataLoader> loader);

  template <typename OutFunc>
  static Status load(QIODevice& ioDevice, OutFunc out);

  template <typename OutFunc>
  static Status load(const QString& filePath, OutFunc out);

 protected:
  ~ImageMetadataLoader() override = default;

  /**
   * \brief Loads metadata from a particular image format.
   *
   * This function must be reentrant, as it may be called from multiple
   * threads at the same time.
   *
   * \param ioDevice The I/O device to read from.  Usually a QFile.
   *        In case FORMAT_NO_RECOGNIZED is returned, the implementation
   *        must leave \p ioDevice in its original state.
   * \param out A callback functional object that will be called to handle
   *        the image metadata.  If there are multiple images (pages) in
   *        the file, this object will be called multiple times.
   */
  virtual Status loadMetadata(QIODevice& ioDevice, const VirtualFunction<void, const ImageMetadata&>& out) = 0;

 private:
  static Status loadImpl(QIODevice& ioDevice, const VirtualFunction<void, const ImageMetadata&>& out);

  static Status loadImpl(const QString& filePath, const VirtualFunction<void, const ImageMetadata&>& out);

  using LoaderList = std::vector<intrusive_ptr<ImageMetadataLoader>>;

  static LoaderList m_sLoaders;

  static class StaticInit {
   public:
    StaticInit();
  } m_staticInit;
};


template <typename Callable>
ImageMetadataLoader::Status ImageMetadataLoader::load(QIODevice& ioDevice, Callable out) {
  return loadImpl(ioDevice, ProxyFunction<Callable, void, const ImageMetadata&>(out));
}

template <typename Callable>
ImageMetadataLoader::Status ImageMetadataLoader::load(const QString& filePath, Callable out) {
  return loadImpl(filePath, ProxyFunction<Callable, void, const ImageMetadata&>(out));
}

#endif  // ifndef SCANTAILOR_CORE_IMAGEMETADATALOADER_H_
