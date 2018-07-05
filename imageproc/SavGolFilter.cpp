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

#include "SavGolFilter.h"
#include "Grayscale.h"
#include "SavGolKernel.h"

namespace imageproc {
namespace {
int calcNumTerms(const int hor_degree, const int vert_degree) {
  return (hor_degree + 1) * (vert_degree + 1);
}

class Kernel : public SavGolKernel {
 public:
  Kernel(const QSize& size, const QPoint& origin, int hor_degree, int vert_degree)
      : SavGolKernel(size, origin, hor_degree, vert_degree) {}

  void convolve(uint8_t* dst, const uint8_t* src_top_left, int src_bpl) const;
};


inline void Kernel::convolve(uint8_t* dst, const uint8_t* src_top_left, int src_bpl) const {
  const uint8_t* p_src = src_top_left;
  const float* p_kernel = data();
  float sum = 0.5;  // For rounding purposes.
  const int w = width();
  const int h = height();

  for (int y = 0; y < h; ++y, p_src += src_bpl) {
    for (int x = 0; x < w; ++x) {
      sum += p_src[x] * *p_kernel;
      ++p_kernel;
    }
  }

  const auto val = static_cast<int>(sum);
  *dst = static_cast<uint8_t>(qBound(0, val, 255));
}

QImage savGolFilterGrayToGray(const QImage& src,
                              const QSize& window_size,
                              const int hor_degree,
                              const int vert_degree) {
  const int width = src.width();
  const int height = src.height();

  // Kernel width and height.
  const int kw = window_size.width();
  const int kh = window_size.height();

  if ((kw > width) || (kh > height)) {
    return src;
  }

  /*
   * Consider a 5x5 kernel:
   * |x|x|T|x|x|
   * |x|x|T|x|x|
   * |L|L|C|R|R|
   * |x|x|B|x|x|
   * |x|x|B|x|x|
   */

  // Co-ordinates of the central point (C) of the kernel.
  const QPoint k_center(kw / 2, kh / 2);

  // Origin is the current hot spot of the kernel.
  // Normally it's at k_center, but sometimes we move
  // it to other locations to avoid parts of the kernel
  // from going outside of the image area.
  QPoint k_origin;

  // Length of the top segment (T) of the kernel.
  const int k_top = k_center.y();

  // Length of the bottom segment (B) of the kernel.
  const int k_bottom = kh - k_top - 1;

  // Length of the left segment (L) of the kernel.
  const int k_left = k_center.x();
  // Length of the right segment (R) of the kernel.
  const int k_right = kw - k_left - 1;

  const uint8_t* const src_data = src.bits();
  const int src_bpl = src.bytesPerLine();

  QImage dst(width, height, QImage::Format_Indexed8);
  dst.setColorTable(createGrayscalePalette());
  if ((width > 0) && (height > 0) && dst.isNull()) {
    throw std::bad_alloc();
  }

  uint8_t* const dst_data = dst.bits();
  const int dst_bpl = dst.bytesPerLine();
  // Top-left corner.
  const uint8_t* src_line = src_data;
  uint8_t* dst_line = dst_data;
  Kernel kernel(window_size, QPoint(0, 0), hor_degree, vert_degree);
  for (int y = 0; y < k_top; ++y, dst_line += dst_bpl) {
    k_origin.setY(y);
    for (int x = 0; x < k_left; ++x) {
      k_origin.setX(x);
      kernel.recalcForOrigin(k_origin);
      kernel.convolve(dst_line + x, src_line, src_bpl);
    }
  }

  // Top area between two corners.
  k_origin.setX(k_center.x());
  src_line = src_data - k_left;
  dst_line = dst_data;
  for (int y = 0; y < k_top; ++y, dst_line += dst_bpl) {
    k_origin.setY(y);
    kernel.recalcForOrigin(k_origin);
    for (int x = k_left; x < width - k_right; ++x) {
      kernel.convolve(dst_line + x, src_line + x, src_bpl);
    }
  }
  // Top-right corner.
  k_origin.setY(0);
  src_line = src_data + width - kw;
  dst_line = dst_data;
  for (int y = 0; y < k_top; ++y, dst_line += dst_bpl) {
    k_origin.setX(k_center.x() + 1);
    for (int x = width - k_right; x < width; ++x) {
      kernel.recalcForOrigin(k_origin);
      kernel.convolve(dst_line + x, src_line, src_bpl);
      k_origin.rx() += 1;
    }
    k_origin.ry() += 1;
  }
  // Central area.
#if 0
            // Simple but slow implementation.
                    kernel.recalcForOrigin(k_center);
                    src_line = src_data - k_left;
                    dst_line = dst_data + dst_bpl * k_top;
                    for (int y = k_top; y < height - k_bottom; ++y) {
                        for (int x = k_left; x < width - k_right; ++x) {
                            kernel.convolve(dst_line + x, src_line + x, src_bpl);
                        }
                        src_line += src_bpl;
                        dst_line += dst_bpl;
                    }
#else
  // Take advantage of Savitzky-Golay filter being separable.
  const SavGolKernel hor_kernel(QSize(window_size.width(), 1), QPoint(k_center.x(), 0), hor_degree, 0);
  const SavGolKernel vert_kernel(QSize(1, window_size.height()), QPoint(0, k_center.y()), 0, vert_degree);

  const int shift = kw - 1;

  // Allocate a 16-byte aligned temporary storage.
  // That may help the compiler to emit efficient SSE code.
  const int temp_stride = (width - shift + 3) & ~3;
  AlignedArray<float, 4> temp_array(temp_stride * height);
  // Horizontal pass.
  src_line = src_data - shift;
  float* temp_line = temp_array.data() - shift;
  for (int y = 0; y < height; ++y) {
    for (int i = shift; i < width; ++i) {
      float sum = 0.0f;

      const uint8_t* src = src_line + i;
      for (int j = 0; j < kw; ++j) {
        sum += src[j] * hor_kernel[j];
      }
      temp_line[i] = sum;
    }
    temp_line += temp_stride;
    src_line += src_bpl;
  }
  // Vertical pass.
  dst_line = dst_data + k_top * dst_bpl + k_left - shift;
  temp_line = temp_array.data() - shift;
  for (int y = k_top; y < height - k_bottom; ++y) {
    for (int i = shift; i < width; ++i) {
      float sum = 0.0f;

      float* tmp = temp_line + i;
      for (int j = 0; j < kh; ++j, tmp += temp_stride) {
        sum += *tmp * vert_kernel[j];
      }
      const auto val = static_cast<int>(sum);
      dst_line[i] = static_cast<uint8_t>(qBound(0, val, 255));
    }

    temp_line += temp_stride;
    dst_line += dst_bpl;
  }
#endif  // if 0

  // Left area between two corners.
  k_origin.setX(0);
  k_origin.setY(k_center.y() + 1);
  for (int x = 0; x < k_left; ++x) {
    src_line = src_data;
    dst_line = dst_data + dst_bpl * k_top;

    kernel.recalcForOrigin(k_origin);
    for (int y = k_top; y < height - k_bottom; ++y) {
      kernel.convolve(dst_line + x, src_line, src_bpl);
      src_line += src_bpl;
      dst_line += dst_bpl;
    }
    k_origin.rx() += 1;
  }
  // Right area between two corners.
  k_origin.setX(k_center.x() + 1);
  k_origin.setY(k_center.y());
  for (int x = width - k_right; x < width; ++x) {
    src_line = src_data + width - kw;
    dst_line = dst_data + dst_bpl * k_top;

    kernel.recalcForOrigin(k_origin);
    for (int y = k_top; y < height - k_bottom; ++y) {
      kernel.convolve(dst_line + x, src_line, src_bpl);
      src_line += src_bpl;
      dst_line += dst_bpl;
    }
    k_origin.rx() += 1;
  }

  // Bottom-left corner.
  k_origin.setY(k_center.y() + 1);
  src_line = src_data + src_bpl * (height - kh);
  dst_line = dst_data + dst_bpl * (height - k_bottom);
  for (int y = height - k_bottom; y < height; ++y, dst_line += dst_bpl) {
    for (int x = 0; x < k_left; ++x) {
      k_origin.setX(x);
      kernel.recalcForOrigin(k_origin);
      kernel.convolve(dst_line + x, src_line, src_bpl);
    }
    k_origin.ry() += 1;
  }

  // Bottom area between two corners.
  k_origin.setX(k_center.x());
  k_origin.setY(k_center.y() + 1);
  src_line = src_data + src_bpl * (height - kh) - k_left;
  dst_line = dst_data + dst_bpl * (height - k_bottom);
  for (int y = height - k_bottom; y < height; ++y, dst_line += dst_bpl) {
    kernel.recalcForOrigin(k_origin);
    for (int x = k_left; x < width - k_right; ++x) {
      kernel.convolve(dst_line + x, src_line + x, src_bpl);
    }
    k_origin.ry() += 1;
  }
  // Bottom-right corner.
  k_origin.setY(k_center.y() + 1);
  src_line = src_data + src_bpl * (height - kh) + (width - kw);
  dst_line = dst_data + dst_bpl * (height - k_bottom);
  for (int y = height - k_bottom; y < height; ++y, dst_line += dst_bpl) {
    k_origin.setX(k_center.x() + 1);
    for (int x = width - k_right; x < width; ++x) {
      kernel.recalcForOrigin(k_origin);
      kernel.convolve(dst_line + x, src_line, src_bpl);
      k_origin.rx() += 1;
    }
    k_origin.ry() += 1;
  }

  return dst;
}  // savGolFilterGrayToGray
}  // namespace

QImage savGolFilter(const QImage& src, const QSize& window_size, const int hor_degree, const int vert_degree) {
  if ((hor_degree < 0) || (vert_degree < 0)) {
    throw std::invalid_argument("savGolFilter: invalid polynomial degree");
  }
  if (window_size.isEmpty()) {
    throw std::invalid_argument("savGolFilter: invalid window size");
  }

  if (calcNumTerms(hor_degree, vert_degree) > window_size.width() * window_size.height()) {
    throw std::invalid_argument("savGolFilter: order is too big for that window");
  }

  return savGolFilterGrayToGray(toGrayscale(src), window_size, hor_degree, vert_degree);
}
}  // namespace imageproc