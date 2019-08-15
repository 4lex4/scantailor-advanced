/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    Based on code from the GIMP project,
    Copyright (C) 1995 Spencer Kimball and Peter Mattis

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

#include "GaussBlur.h"
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <cmath>
#include "Constants.h"
#include "GrayImage.h"

namespace imageproc {
namespace gauss_blur_impl {
void find_iir_constants(float* n_p, float* n_m, float* d_p, float* d_m, float* bd_p, float* bd_m, float std_dev) {
  /*  The constants used in the implemenation of a casual sequence
   *  using a 4th order approximation of the gaussian operator
   */

  const auto div = static_cast<const float>(std::sqrt(2.0 * constants::PI) * std_dev);
  const auto x0 = static_cast<const float>(-1.783 / std_dev);
  const auto x1 = static_cast<const float>(-1.723 / std_dev);
  const auto x2 = static_cast<const float>(0.6318 / std_dev);
  const auto x3 = static_cast<const float>(1.997 / std_dev);
  const auto x4 = static_cast<const float>(1.6803 / div);
  const auto x5 = static_cast<const float>(3.735 / div);
  const auto x6 = static_cast<const float>(-0.6803 / div);
  const auto x7 = static_cast<const float>(-0.2598 / div);

  n_p[0] = x4 + x6;
  n_p[1] = std::exp(x1) * (x7 * std::sin(x3) - (x6 + 2 * x4) * std::cos(x3))
           + std::exp(x0) * (x5 * std::sin(x2) - (2 * x6 + x4) * std::cos(x2));
  n_p[2] = 2 * std::exp(x0 + x1)
               * ((x4 + x6) * std::cos(x3) * std::cos(x2) - x5 * std::cos(x3) * std::sin(x2)
                  - x7 * std::cos(x2) * std::sin(x3))
           + x6 * std::exp(2 * x0) + x4 * std::exp(2 * x1);
  n_p[3] = std::exp(x1 + 2 * x0) * (x7 * std::sin(x3) - x6 * std::cos(x3))
           + std::exp(x0 + 2 * x1) * (x5 * std::sin(x2) - x4 * std::cos(x2));
  n_p[4] = 0.0;

  d_p[0] = 0.0;
  d_p[1] = -2 * std::exp(x1) * std::cos(x3) - 2 * std::exp(x0) * std::cos(x2);
  d_p[2] = 4 * std::cos(x3) * std::cos(x2) * std::exp(x0 + x1) + std::exp(2 * x1) + std::exp(2 * x0);
  d_p[3] = -2 * std::cos(x2) * std::exp(x0 + 2 * x1) - 2 * std::cos(x3) * std::exp(x1 + 2 * x0);
  d_p[4] = std::exp(2 * x0 + 2 * x1);

  for (int i = 0; i <= 4; i++) {
    d_m[i] = d_p[i];
  }

  n_m[0] = 0.0;

  for (int i = 1; i <= 4; i++) {
    n_m[i] = n_p[i] - d_p[i] * n_p[0];
  }

  float sum_n_p = 0.0;
  float sum_n_m = 0.0;
  float sum_d = 0.0;

  for (int i = 0; i <= 4; i++) {
    sum_n_p += n_p[i];
    sum_n_m += n_m[i];
    sum_d += d_p[i];
  }

  const auto a = static_cast<const float>(sum_n_p / (1.0 + sum_d));
  const auto b = static_cast<const float>(sum_n_m / (1.0 + sum_d));

  for (int i = 0; i <= 4; i++) {
    bd_p[i] = d_p[i] * a;
    bd_m[i] = d_m[i] * b;
  }
}  // find_iir_constants
}  // namespace gauss_blur_impl

GrayImage gaussBlur(const GrayImage& src, float h_sigma, float v_sigma) {
  using namespace boost::lambda;

  if (src.isNull()) {
    return src;
  }

  GrayImage dst(src.size());
  gaussBlurGeneric(src.size(), h_sigma, v_sigma, src.data(), src.stride(), StaticCastValueConv<float>(), dst.data(),
                   dst.stride(), _1 = bind<uint8_t>(RoundAndClipValueConv<uint8_t>(), _2));

  return dst;
}
}  // namespace imageproc