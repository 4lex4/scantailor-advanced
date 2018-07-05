/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef IMAGEMETADATALOADER_H_
#define IMAGEMETADATALOADER_H_

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
  static Status load(QIODevice& io_device, OutFunc out);

  template <typename OutFunc>
  static Status load(const QString& file_path, OutFunc out);

 protected:
  ~ImageMetadataLoader() override = default;

  /**
   * \brief Loads metadata from a particular image format.
   *
   * This function must be reentrant, as it may be called from multiple
   * threads at the same time.
   *
   * \param io_device The I/O device to read from.  Usually a QFile.
   *        In case FORMAT_NO_RECOGNIZED is returned, the implementation
   *        must leave \p io_device in its original state.
   * \param out A callback functional object that will be called to handle
   *        the image metadata.  If there are multiple images (pages) in
   *        the file, this object will be called multiple times.
   */
  virtual Status loadMetadata(QIODevice& io_device, const VirtualFunction<void, const ImageMetadata&>& out) = 0;

 private:
  static Status loadImpl(QIODevice& io_device, const VirtualFunction<void, const ImageMetadata&>& out);

  static Status loadImpl(const QString& file_path, const VirtualFunction<void, const ImageMetadata&>& out);

  typedef std::vector<intrusive_ptr<ImageMetadataLoader>> LoaderList;
  static LoaderList m_sLoaders;
};


template <typename Callable>
ImageMetadataLoader::Status ImageMetadataLoader::load(QIODevice& io_device, Callable out) {
  return loadImpl(io_device, ProxyFunction<Callable, void, const ImageMetadata&>(out));
}

template <typename Callable>
ImageMetadataLoader::Status ImageMetadataLoader::load(const QString& file_path, Callable out) {
  return loadImpl(file_path, ProxyFunction<Callable, void, const ImageMetadata&>(out));
}

#endif  // ifndef IMAGEMETADATALOADER_H_
