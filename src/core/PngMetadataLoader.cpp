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

static void readFn(png_structp png_ptr, png_bytep data, png_size_t length) {
  auto* io_device = (QIODevice*) png_get_io_ptr(png_ptr);
  while (length > 0) {
    const qint64 read = io_device->read((char*) data, length);
    if (read <= 0) {
      png_error(png_ptr, "Read Error");

      return;
    }
    length -= read;
  }
}

ImageMetadataLoader::Status PngMetadataLoader::loadMetadata(QIODevice& io_device,
                                                            const VirtualFunction<void, const ImageMetadata&>& out) {
  if (!io_device.isReadable()) {
    return GENERIC_ERROR;
  }

  png_byte signature[8];
  if (io_device.peek((char*) signature, 8) != 8) {
    return FORMAT_NOT_RECOGNIZED;
  }

  if (png_sig_cmp(signature, 0, sizeof(signature)) != 0) {
    return FORMAT_NOT_RECOGNIZED;
  }

  PngHandle png;

  if (setjmp(png_jmpbuf(png.handle()))) {
    return GENERIC_ERROR;
  }

  png_set_read_fn(png.handle(), &io_device, &readFn);
  png_read_info(png.handle(), png.info());

  QSize size;
  Dpi dpi;
  size.setWidth(png_get_image_width(png.handle(), png.info()));
  size.setHeight(png_get_image_height(png.handle(), png.info()));
  png_uint_32 res_x, res_y;
  int unit_type;
  if (png_get_pHYs(png.handle(), png.info(), &res_x, &res_y, &unit_type)) {
    if (unit_type == PNG_RESOLUTION_METER) {
      dpi = Dpm(res_x, res_y);
    }
  }

  out(ImageMetadata(size, dpi));

  return LOADED;
}  // PngMetadataLoader::loadMetadata
