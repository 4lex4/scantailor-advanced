// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PngMetadataLoader.h"
#include <png.h>
#include <QIODevice>
#include "Dpm.h"
#include "ImageMetadata.h"
#include "NonCopyable.h"

namespace {
class PngHandle {
  DECLARE_NON_COPYABLE(PngHandle)

 public:
  PngHandle();

  ~PngHandle();

  png_structp handle() const { return m_png; }

  png_infop info() const { return m_info; }

 private:
  png_structp m_png;
  png_infop m_info;
};


PngHandle::PngHandle() {
  m_png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!m_png) {
    throw std::bad_alloc();
  }
  m_info = png_create_info_struct(m_png);
  if (!m_info) {
    throw std::bad_alloc();
  }
}

PngHandle::~PngHandle() {
  png_destroy_read_struct(&m_png, &m_info, nullptr);
}
}  // namespace

static void readFn(png_structp pngPtr, png_bytep data, png_size_t length) {
  auto* ioDevice = (QIODevice*) png_get_io_ptr(pngPtr);
  while (length > 0) {
    const qint64 read = ioDevice->read((char*) data, length);
    if (read <= 0) {
      png_error(pngPtr, "Read Error");

      return;
    }
    length -= read;
  }
}

ImageMetadataLoader::Status PngMetadataLoader::loadMetadata(QIODevice& ioDevice,
                                                            const VirtualFunction<void, const ImageMetadata&>& out) {
  if (!ioDevice.isReadable()) {
    return GENERIC_ERROR;
  }

  png_byte signature[8];
  if (ioDevice.peek((char*) signature, 8) != 8) {
    return FORMAT_NOT_RECOGNIZED;
  }

  if (png_sig_cmp(signature, 0, sizeof(signature)) != 0) {
    return FORMAT_NOT_RECOGNIZED;
  }

  PngHandle png;

  if (setjmp(png_jmpbuf(png.handle()))) {
    return GENERIC_ERROR;
  }

  png_set_read_fn(png.handle(), &ioDevice, &readFn);
  png_read_info(png.handle(), png.info());

  QSize size;
  Dpi dpi;
  size.setWidth(png_get_image_width(png.handle(), png.info()));
  size.setHeight(png_get_image_height(png.handle(), png.info()));
  png_uint_32 resX, res_y;
  int unitType;
  if (png_get_pHYs(png.handle(), png.info(), &resX, &res_y, &unitType)) {
    if (unitType == PNG_RESOLUTION_METER) {
      dpi = Dpm(resX, res_y);
    }
  }

  out(ImageMetadata(size, dpi));

  return LOADED;
}  // PngMetadataLoader::loadMetadata
