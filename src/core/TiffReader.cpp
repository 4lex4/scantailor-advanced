// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TiffReader.h"
#include <tiff.h>
#include <tiffio.h>
#include <QDebug>
#include <QIODevice>
#include <QImage>
#include <cassert>
#include <cmath>
#include "Dpm.h"
#include "ImageMetadata.h"
#include "NonCopyable.h"

class TiffReader::TiffHeader {
 public:
  enum Signature { INVALID_SIGNATURE, TIFF_BIG_ENDIAN, TIFF_LITTLE_ENDIAN };

  TiffHeader() : m_signature(INVALID_SIGNATURE), m_version(0) {}

  TiffHeader(Signature signature, int version) : m_signature(signature), m_version(version) {}

  Signature signature() const { return m_signature; }

  int version() const { return m_version; }

 private:
  Signature m_signature;
  int m_version;
};


class TiffReader::TiffHandle {
 public:
  explicit TiffHandle(TIFF* handle) : m_handle(handle) {}

  ~TiffHandle() {
    if (m_handle) {
      TIFFClose(m_handle);
    }
  }

  TIFF* handle() const { return m_handle; }

 private:
  TIFF* m_handle;
};


template <typename T>
class TiffReader::TiffBuffer {
  DECLARE_NON_COPYABLE(TiffBuffer)

 public:
  TiffBuffer() : m_data(nullptr) {}

  explicit TiffBuffer(tsize_t numItems) {
    m_data = (T*) _TIFFmalloc(numItems * sizeof(T));
    if (!m_data) {
      throw std::bad_alloc();
    }
  }

  ~TiffBuffer() {
    if (m_data) {
      _TIFFfree(m_data);
    }
  }

  T* data() { return m_data; }

  void swap(TiffBuffer& other) { std::swap(m_data, other.m_data); }

 private:
  T* m_data;
};


struct TiffReader::TiffInfo {
  int width;
  int height;
  uint16 bitsPerSample;
  uint16 samplesPerPixel;
  uint16 sampleFormat;
  uint16 photometric;
  bool hostBigEndian;
  bool fileBigEndian;

  TiffInfo(const TiffHandle& tif, const TiffHeader& header);

  bool mapsToBinaryOrIndexed8() const;
};


TiffReader::TiffInfo::TiffInfo(const TiffHandle& tif, const TiffHeader& header)
    : width(0),
      height(0),
      bitsPerSample(1),
      samplesPerPixel(1),
      sampleFormat(SAMPLEFORMAT_UINT),
      photometric(PHOTOMETRIC_MINISBLACK),
      hostBigEndian(QSysInfo::ByteOrder == QSysInfo::BigEndian),
      fileBigEndian(header.signature() == TiffHeader::TIFF_BIG_ENDIAN) {
  uint16 compression = 1;
  TIFFGetField(tif.handle(), TIFFTAG_COMPRESSION, &compression);
  switch (compression) {
    case COMPRESSION_CCITTFAX3:
    case COMPRESSION_CCITTFAX4:
    case COMPRESSION_CCITTRLE:
    case COMPRESSION_CCITTRLEW:
      photometric = PHOTOMETRIC_MINISWHITE;
      break;
    default:
      break;
  }

  TIFFGetField(tif.handle(), TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif.handle(), TIFFTAG_IMAGELENGTH, &height);
  TIFFGetField(tif.handle(), TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
  TIFFGetField(tif.handle(), TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
  TIFFGetField(tif.handle(), TIFFTAG_SAMPLEFORMAT, &sampleFormat);
  TIFFGetField(tif.handle(), TIFFTAG_PHOTOMETRIC, &photometric);
}

bool TiffReader::TiffInfo::mapsToBinaryOrIndexed8() const {
  if ((samplesPerPixel != 1) || (sampleFormat != SAMPLEFORMAT_UINT) || (bitsPerSample > 8)) {
    return false;
  }

  switch (photometric) {
    case PHOTOMETRIC_PALETTE:
    case PHOTOMETRIC_MINISBLACK:
    case PHOTOMETRIC_MINISWHITE:
      return true;
    default:
      break;
  }

  return false;
}

static tsize_t deviceRead(thandle_t context, tdata_t data, tsize_t size) {
  auto* dev = (QIODevice*) context;

  return (tsize_t) dev->read(static_cast<char*>(data), size);
}

static tsize_t deviceWrite(thandle_t context, tdata_t data, tsize_t size) {
  // Not implemented.
  return 0;
}

static toff_t deviceSeek(thandle_t context, toff_t offset, int whence) {
  auto* dev = (QIODevice*) context;

  switch (whence) {
    case SEEK_SET:
      dev->seek(offset);
      break;
    case SEEK_CUR:
      dev->seek(dev->pos() + offset);
      break;
    case SEEK_END:
      dev->seek(dev->size() + offset);
      break;
    default:
      break;
  }

  return dev->pos();
}

static int deviceClose(thandle_t context) {
  auto* dev = (QIODevice*) context;
  dev->close();

  return 0;
}

static toff_t deviceSize(thandle_t context) {
  auto* dev = (QIODevice*) context;

  return dev->size();
}

static int deviceMap(thandle_t, tdata_t*, toff_t*) {
  // Not implemented.
  return 0;
}

static void deviceUnmap(thandle_t, tdata_t, toff_t) {
  // Not implemented.
}

bool TiffReader::canRead(QIODevice& device) {
  if (!device.isReadable()) {
    return false;
  }
  if (device.isSequential()) {
    // libtiff needs to be able to seek.
    return false;
  }

  TiffHeader header(readHeader(device));

  return checkHeader(header);
}

ImageMetadataLoader::Status TiffReader::readMetadata(QIODevice& device,
                                                     const VirtualFunction<void, const ImageMetadata&>& out) {
  if (!device.isReadable()) {
    return ImageMetadataLoader::GENERIC_ERROR;
  }
  if (device.isSequential()) {
    // libtiff needs to be able to seek.
    return ImageMetadataLoader::GENERIC_ERROR;
  }

  if (!checkHeader(TiffHeader(readHeader(device)))) {
    return ImageMetadataLoader::FORMAT_NOT_RECOGNIZED;
  }

  TiffHandle tif(TIFFClientOpen("file", "rBm", &device, &deviceRead, &deviceWrite, &deviceSeek, &deviceClose,
                                &deviceSize, &deviceMap, &deviceUnmap));
  if (!tif.handle()) {
    return ImageMetadataLoader::GENERIC_ERROR;
  }

  do {
    out(currentPageMetadata(tif));
  } while (TIFFReadDirectory(tif.handle()));

  return ImageMetadataLoader::LOADED;
}

static void convertAbgrToArgb(const uint32* src, uint32* dst, int count) {
  for (int i = 0; i < count; ++i) {
    const uint32 srcWord = src[i];
    uint32 dstWord = srcWord & 0xFF000000;    // A
    dstWord |= (srcWord & 0x00FF0000) >> 16;  // B
    dstWord |= srcWord & 0x0000FF00;          // G
    dstWord |= (srcWord & 0x000000FF) << 16;  // R
    dst[i] = dstWord;
  }
}

QImage TiffReader::readImage(QIODevice& device, const int pageNum) {
  if (!device.isReadable()) {
    return QImage();
  }
  if (device.isSequential()) {
    // libtiff needs to be able to seek.
    return QImage();
  }

  TiffHeader header(readHeader(device));
  if (!checkHeader(header)) {
    return QImage();
  }

  TiffHandle tif(TIFFClientOpen("file", "rBm", &device, &deviceRead, &deviceWrite, &deviceSeek, &deviceClose,
                                &deviceSize, &deviceMap, &deviceUnmap));
  if (!tif.handle()) {
    return QImage();
  }

  if (!TIFFSetDirectory(tif.handle(), (uint16) pageNum)) {
    return QImage();
  }

  const TiffInfo info(tif, header);

  const ImageMetadata metadata(currentPageMetadata(tif));

  QImage image;

  if (info.mapsToBinaryOrIndexed8()) {
    // Common case optimization.
    image = extractBinaryOrIndexed8Image(tif, info);
  } else {
    // General case.
    image = QImage(info.width, info.height, info.samplesPerPixel == 3 ? QImage::Format_RGB32 : QImage::Format_ARGB32);
    if (image.isNull()) {
      throw std::bad_alloc();
    }

    // For ABGR -> ARGB conversion.
    TiffBuffer<uint32> tmpBuffer;
    const uint32* srcLine = nullptr;

    if (image.bytesPerLine() == 4 * info.width) {
      // We can avoid creating a temporary buffer in this case.
      if (!TIFFReadRGBAImageOriented(tif.handle(), info.width, info.height, (uint32*) image.bits(), ORIENTATION_TOPLEFT,
                                     0)) {
        return QImage();
      }
      srcLine = (const uint32*) image.bits();
    } else {
      TiffBuffer<uint32>(info.width * info.height).swap(tmpBuffer);
      if (!TIFFReadRGBAImageOriented(tif.handle(), info.width, info.height, tmpBuffer.data(), ORIENTATION_TOPLEFT, 0)) {
        return QImage();
      }
      srcLine = tmpBuffer.data();
    }

    auto* dstLine = (uint32*) image.bits();
    assert(image.bytesPerLine() % 4 == 0);
    const int dstStride = image.bytesPerLine() / 4;
    for (int y = 0; y < info.height; ++y) {
      convertAbgrToArgb(srcLine, dstLine, info.width);
      srcLine += info.width;
      dstLine += dstStride;
    }
  }

  if (!metadata.dpi().isNull()) {
    const Dpm dpm(metadata.dpi());
    image.setDotsPerMeterX(dpm.horizontal());
    image.setDotsPerMeterY(dpm.vertical());
  }

  return image;
}  // TiffReader::readImage

TiffReader::TiffHeader TiffReader::readHeader(QIODevice& device) {
  unsigned char data[4];
  if (device.peek((char*) data, sizeof(data)) != sizeof(data)) {
    return TiffHeader();
  }

  const uint16 versionByte0 = data[2];
  const uint16 versionByte1 = data[3];

  if ((data[0] == 0x4d) && (data[1] == 0x4d)) {
    const uint16 version = (versionByte0 << 8) + versionByte1;

    return TiffHeader(TiffHeader::TIFF_BIG_ENDIAN, version);
  } else if ((data[0] == 0x49) && (data[1] == 0x49)) {
    const uint16 version = (versionByte1 << 8) + versionByte0;

    return TiffHeader(TiffHeader::TIFF_LITTLE_ENDIAN, version);
  } else {
    return TiffHeader();
  }
}

bool TiffReader::checkHeader(const TiffHeader& header) {
  if (header.signature() == TiffHeader::INVALID_SIGNATURE) {
    return false;
  }
  if ((header.version() != 42) && (header.version() != 43)) {
    return false;
  }

  return true;
}

ImageMetadata TiffReader::currentPageMetadata(const TiffHandle& tif) {
  uint32 width = 0, height = 0;
  float xres = 0, yres = 0;
  uint16 resUnit = 0;
  TIFFGetField(tif.handle(), TIFFTAG_IMAGEWIDTH, &width);
  TIFFGetField(tif.handle(), TIFFTAG_IMAGELENGTH, &height);
  TIFFGetField(tif.handle(), TIFFTAG_XRESOLUTION, &xres);
  TIFFGetField(tif.handle(), TIFFTAG_YRESOLUTION, &yres);
  TIFFGetFieldDefaulted(tif.handle(), TIFFTAG_RESOLUTIONUNIT, &resUnit);

  return ImageMetadata(QSize(width, height), getDpi(xres, yres, resUnit));
}

Dpi TiffReader::getDpi(float xres, float yres, unsigned resUnit) {
  switch (resUnit) {
    case RESUNIT_INCH:  // inch
      return Dpi(qRound(xres), qRound(yres));
    case RESUNIT_CENTIMETER:  // cm
      return Dpm(qRound(xres * 100), qRound(yres * 100));
    default:
      break;
  }

  return Dpi();
}

QImage TiffReader::extractBinaryOrIndexed8Image(const TiffHandle& tif, const TiffInfo& info) {
  QImage::Format format = QImage::Format_Indexed8;
  if (info.bitsPerSample == 1) {
    // Because we specify B option when opening, we can
    // always use Format_Mono, and not Format_MonoLSB.
    format = QImage::Format_Mono;
  }

  QImage image(info.width, info.height, format);
  if (image.isNull()) {
    throw std::bad_alloc();
  }

  const int numColors = 1 << info.bitsPerSample;
  image.setColorCount(numColors);

  if (info.photometric == PHOTOMETRIC_PALETTE) {
    uint16* pr = nullptr;
    uint16* pg = nullptr;
    uint16* pb = nullptr;
    TIFFGetField(tif.handle(), TIFFTAG_COLORMAP, &pr, &pg, &pb);
    if (!pr || !pg || !pb) {
      return QImage();
    }
    if (info.hostBigEndian != info.fileBigEndian) {
      TIFFSwabArrayOfShort(pr, numColors);
      TIFFSwabArrayOfShort(pg, numColors);
      TIFFSwabArrayOfShort(pb, numColors);
    }
    const double f = 255.0 / 65535.0;
    for (int i = 0; i < numColors; ++i) {
      const auto r = (uint32) std::lround(pr[i] * f);
      const auto g = (uint32) std::lround(pg[i] * f);
      const auto b = (uint32) std::lround(pb[i] * f);
      const uint32 a = 0xFF000000;
      image.setColor(i, a | (r << 16) | (g << 8) | b);
    }
  } else if (info.photometric == PHOTOMETRIC_MINISBLACK) {
    const double f = 255.0 / (numColors - 1);
    for (int i = 0; i < numColors; ++i) {
      const auto gray = (int) std::lround(i * f);
      image.setColor(i, qRgb(gray, gray, gray));
    }
  } else if (info.photometric == PHOTOMETRIC_MINISWHITE) {
    const double f = 255.0 / (numColors - 1);
    int c = numColors - 1;
    for (int i = 0; i < numColors; ++i, --c) {
      const auto gray = (int) std::lround(c * f);
      image.setColor(i, qRgb(gray, gray, gray));
    }
  } else {
    return QImage();
  }

  if ((info.bitsPerSample == 1) || (info.bitsPerSample == 8)) {
    readLines(tif, image);
  } else {
    readAndUnpackLines(tif, info, image);
  }

  return image;
}  // TiffReader::extractBinaryOrIndexed8Image

void TiffReader::readLines(const TiffHandle& tif, QImage& image) {
  const int height = image.height();
  for (int y = 0; y < height; ++y) {
    TIFFReadScanline(tif.handle(), image.scanLine(y), y);
  }
}

void TiffReader::readAndUnpackLines(const TiffHandle& tif, const TiffInfo& info, QImage& image) {
  TiffBuffer<uint8> buf(TIFFScanlineSize(tif.handle()));

  const int width = image.width();
  const int height = image.height();
  const int bitsPerSample = info.bitsPerSample;
  const unsigned dstMask = (1 << bitsPerSample) - 1;

  for (int y = 0; y < height; ++y) {
    TIFFReadScanline(tif.handle(), buf.data(), y);

    unsigned accum = 0;
    int bitsInAccum = 0;

    const uint8* src = buf.data();
    auto* dst = image.scanLine(y);

    for (int i = width; i > 0; --i, ++dst) {
      while (bitsInAccum < bitsPerSample) {
        accum <<= 8;
        accum |= *src;
        bitsInAccum += 8;
        ++src;
      }
      bitsInAccum -= bitsPerSample;
      *dst = static_cast<uint8>((accum >> bitsInAccum) & dstMask);
    }
  }
}
