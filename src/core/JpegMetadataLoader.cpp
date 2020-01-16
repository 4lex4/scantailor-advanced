// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "JpegMetadataLoader.h"

#include <QDebug>
#include <QIODevice>
#include <cassert>
#include <csetjmp>

#include "Dpm.h"
#include "ImageMetadata.h"
#include "NonCopyable.h"

extern "C" {
#include <jpeglib.h>
}

namespace {
/*======================== JpegDecompressionHandle =======================*/

class JpegDecompressHandle {
  DECLARE_NON_COPYABLE(JpegDecompressHandle)

 public:
  JpegDecompressHandle(jpeg_error_mgr* errMgr, jpeg_source_mgr* srcMgr);

  ~JpegDecompressHandle();

  jpeg_decompress_struct* ptr() { return &m_info; }

  jpeg_decompress_struct* operator->() { return &m_info; }

 private:
  jpeg_decompress_struct m_info{};
};


JpegDecompressHandle::JpegDecompressHandle(jpeg_error_mgr* errMgr, jpeg_source_mgr* srcMgr) {
  m_info.err = errMgr;
  jpeg_create_decompress(&m_info);
  m_info.src = srcMgr;
}

JpegDecompressHandle::~JpegDecompressHandle() {
  jpeg_destroy_decompress(&m_info);
}

/*============================ JpegSourceManager =========================*/

class JpegSourceManager : public jpeg_source_mgr {
  DECLARE_NON_COPYABLE(JpegSourceManager)

 public:
  explicit JpegSourceManager(QIODevice& ioDevice);

 private:
  static void initSource(j_decompress_ptr cinfo);

  static boolean fillInputBuffer(j_decompress_ptr cinfo);

  boolean fillInputBufferImpl();

  static void skipInputData(j_decompress_ptr cinfo, long numBytes);

  void skipInputDataImpl(long numBytes);

  static void termSource(j_decompress_ptr cinfo);

  static JpegSourceManager* object(j_decompress_ptr cinfo);

  QIODevice& m_device;
  JOCTET m_buf[4096]{};
};


JpegSourceManager::JpegSourceManager(QIODevice& ioDevice) : jpeg_source_mgr(), m_device(ioDevice) {
  init_source = &JpegSourceManager::initSource;
  fill_input_buffer = &JpegSourceManager::fillInputBuffer;
  skip_input_data = &JpegSourceManager::skipInputData;
  resync_to_restart = &jpeg_resync_to_restart;
  term_source = &JpegSourceManager::termSource;
  bytes_in_buffer = 0;
  next_input_byte = m_buf;
}

void JpegSourceManager::initSource(j_decompress_ptr cinfo) {
  // No-op.
}

boolean JpegSourceManager::fillInputBuffer(j_decompress_ptr cinfo) {
  return object(cinfo)->fillInputBufferImpl();
}

boolean JpegSourceManager::fillInputBufferImpl() {
  const qint64 bytesRead = m_device.read((char*) m_buf, sizeof(m_buf));
  if (bytesRead > 0) {
    bytes_in_buffer = bytesRead;
  } else {
    // Insert a fake EOI marker.
    m_buf[0] = 0xFF;
    m_buf[1] = JPEG_EOI;
    bytes_in_buffer = 2;
  }
  next_input_byte = m_buf;
  return 1;
}

void JpegSourceManager::skipInputData(j_decompress_ptr cinfo, long numBytes) {
  object(cinfo)->skipInputDataImpl(numBytes);
}

void JpegSourceManager::skipInputDataImpl(long numBytes) {
  if (numBytes <= 0) {
    return;
  }

  while (numBytes > (long) bytes_in_buffer) {
    numBytes -= (long) bytes_in_buffer;
    fillInputBufferImpl();
  }
  next_input_byte += numBytes;
  bytes_in_buffer -= numBytes;
}

void JpegSourceManager::termSource(j_decompress_ptr cinfo) {
  // No-op.
}

JpegSourceManager* JpegSourceManager::object(j_decompress_ptr cinfo) {
  return static_cast<JpegSourceManager*>(cinfo->src);
}

/*============================= JpegErrorManager ===========================*/

class JpegErrorManager : public jpeg_error_mgr {
  DECLARE_NON_COPYABLE(JpegErrorManager)

 public:
  JpegErrorManager();

  jmp_buf& jmpBuf() { return m_jmpBuf; }

 private:
  static void errorExit(j_common_ptr cinfo);

  static JpegErrorManager* object(j_common_ptr cinfo);

  jmp_buf m_jmpBuf{};
};


JpegErrorManager::JpegErrorManager() : jpeg_error_mgr() {
  jpeg_std_error(this);
  error_exit = &JpegErrorManager::errorExit;
}

void JpegErrorManager::errorExit(j_common_ptr cinfo) {
  longjmp(object(cinfo)->jmpBuf(), 1);
}

JpegErrorManager* JpegErrorManager::object(j_common_ptr cinfo) {
  return static_cast<JpegErrorManager*>(cinfo->err);
}
}  // namespace

/*============================= JpegMetadataLoader ==========================*/

ImageMetadataLoader::Status JpegMetadataLoader::loadMetadata(QIODevice& ioDevice,
                                                             const VirtualFunction<void, const ImageMetadata&>& out) {
  if (!ioDevice.isReadable()) {
    return GENERIC_ERROR;
  }

  static const unsigned char jpeg_signature[] = {0xff, 0xd8, 0xff};
  static const int sigSize = sizeof(jpeg_signature);

  unsigned char signature[sigSize];
  if (ioDevice.peek((char*) signature, sigSize) != sigSize) {
    return FORMAT_NOT_RECOGNIZED;
  }
  if (memcmp(jpeg_signature, signature, sigSize) != 0) {
    return FORMAT_NOT_RECOGNIZED;
  }

  JpegErrorManager errMgr;
  if (setjmp(errMgr.jmpBuf())) {
    // Returning from longjmp().
    return GENERIC_ERROR;
  }

  JpegSourceManager srcMgr(ioDevice);
  JpegDecompressHandle cinfo(&errMgr, &srcMgr);

  const int headerStatus = jpeg_read_header(cinfo.ptr(), 0);
  if (headerStatus == JPEG_HEADER_TABLES_ONLY) {
    return NO_IMAGES;
  }

  // The other possible value is JPEG_SUSPENDED, but we never suspend it.
  assert(headerStatus == JPEG_HEADER_OK);

  if (!jpeg_start_decompress(cinfo.ptr())) {
    // libjpeg doesn't support all compression types.
    return GENERIC_ERROR;
  }

  const QSize size(cinfo->image_width, cinfo->image_height);
  Dpi dpi;
  if (cinfo->density_unit == 1) {
    // Dots per inch.
    dpi = Dpi(cinfo->X_density, cinfo->Y_density);
  } else if (cinfo->density_unit == 2) {
    // Dots per centimeter.
    dpi = Dpm(cinfo->X_density * 100, cinfo->Y_density * 100);
  }

  out(ImageMetadata(size, dpi));
  return LOADED;
}  // JpegMetadataLoader::loadMetadata
