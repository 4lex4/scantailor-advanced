/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef IMAGEPROC_COLOR_MIXER_H_
#define IMAGEPROC_COLOR_MIXER_H_

#include <cassert>
#include <cstdint>
#include <limits>

namespace imageproc {
namespace color_mixer_impl {
template <typename Mixer, bool IntegerAccum>
struct Switcher {
  typedef typename Mixer::accum_type accum_type;
  typedef typename Mixer::result_type result_type;

  static result_type mix(const Mixer* mixer, accum_type total_weight) { return mixer->nonIntegerMix(total_weight); }
};

template <typename Mixer>
struct Switcher<Mixer, true> {
  typedef typename Mixer::accum_type accum_type;
  typedef typename Mixer::result_type result_type;

  static result_type mix(const Mixer* mixer, accum_type total_weight) { return mixer->integerMix(total_weight); }
};
}  // namespace color_mixer_impl

/**
 * @brief Interpolates between weighted pixel values.
 *
 * @tparam AccumType An integer or a floating type to be used for pixel weights
 *         and internally for accumulating weighted pixel values.
 */
template <typename AccumType>
class GrayColorMixer {
  template <typename Mixer, bool IntegerAccum>
  friend struct color_mixer_impl::Switcher;

 public:
  typedef AccumType accum_type;
  typedef uint8_t result_type;

  GrayColorMixer() : m_accum() {}

  /**
   * @brief Adds a weighted pixel into the mix.
   *
   * The weight must be non-negative.
   */
  void add(uint8_t gray_level, AccumType weight) { m_accum += AccumType(gray_level) * weight; }

  /**
   * @brief Returns a color intepolated from previously added ones.
   *
   * @param total_weight The sum of individual weights passed to add().
   *        While an individual weight can be zero, the sum has to be above zero.
   * @return Interpolated color.
   */
  result_type mix(AccumType total_weight) const {
    assert(total_weight > 0);

    using namespace color_mixer_impl;
    typedef std::numeric_limits<AccumType> traits;
    return Switcher<GrayColorMixer<AccumType>, traits::is_integer>::mix(this, total_weight);
  }

 private:
  uint8_t nonIntegerMix(AccumType total_weight) const {
    assert(total_weight > 0);
    return static_cast<uint8_t>(m_accum / total_weight + AccumType(0.5));
  }

  uint8_t integerMix(AccumType total_weight) const {
    assert(total_weight > 0);
    const AccumType half_weight = total_weight >> 1;
    const AccumType mixed = (m_accum + half_weight) / total_weight;
    return static_cast<uint8_t>(mixed);
  }

  AccumType m_accum;
};


/**
 * @brief Interpolates between weighted pixel values.
 *
 * @tparam AccumType An integer or a floating type to be used for pixel weights
 *         and internally for accumulating weighted pixel values.
 */
template <typename AccumType>
class RgbColorMixer {
  template <typename Mixer, bool IntegerAccum>
  friend struct color_mixer_impl::Switcher;

 public:
  typedef AccumType accum_type;
  typedef uint32_t result_type;

  RgbColorMixer() : m_redAccum(), m_greenAccum(), m_blueAccum() {}

  /** @see GrayColorMixer::add() */
  void add(uint32_t rgb, AccumType weight) {
    m_redAccum += AccumType((rgb >> 16) & 0xFF) * weight;
    m_greenAccum += AccumType((rgb >> 8) & 0xFF) * weight;
    m_blueAccum += AccumType(rgb & 0xFF) * weight;
  }

  /** @see GrayColorMixer::mix() */
  result_type mix(AccumType total_weight) const {
    assert(total_weight > 0);

    using namespace color_mixer_impl;
    typedef std::numeric_limits<AccumType> traits;
    return Switcher<RgbColorMixer<AccumType>, traits::is_integer>::mix(this, total_weight);
  }

 private:
  uint32_t nonIntegerMix(AccumType total_weight) const {
    assert(total_weight > 0);
    const AccumType scale = AccumType(1) / total_weight;
    const uint32_t r = uint32_t(AccumType(0.5) + m_redAccum * scale);
    const uint32_t g = uint32_t(AccumType(0.5) + m_greenAccum * scale);
    const uint32_t b = uint32_t(AccumType(0.5) + m_blueAccum * scale);
    return uint32_t(0xff000000) | (r << 16) | (g << 8) | b;
  }

  uint32_t integerMix(AccumType total_weight) const {
    assert(total_weight > 0);
    const AccumType half_weight = total_weight >> 1;
    const uint32_t r = uint32_t((m_redAccum + half_weight) / total_weight);
    const uint32_t g = uint32_t((m_greenAccum + half_weight) / total_weight);
    const uint32_t b = uint32_t((m_blueAccum + half_weight) / total_weight);
    return uint32_t(0xff000000) | (r << 16) | (g << 8) | b;
  }

  AccumType m_redAccum;
  AccumType m_greenAccum;
  AccumType m_blueAccum;
};


/**
 * @brief Interpolates between weighted pixel values.
 *
 * @tparam AccumType An integer or a floating type to be used for pixel weights
 *         and internally for accumulating weighted pixel values.
 *
 * @note Because additional scaling by alpha, uint32_t is not going to be enough for
 *       AccumType if we plan to do significant downscaling. uint64_t or a floating
 *       point type would be fine.
 */
template <typename AccumType>
class ArgbColorMixer {
  template <typename Mixer, bool IntegerAccum>
  friend struct color_mixer_impl::Switcher;

 public:
  typedef AccumType accum_type;
  typedef uint32_t result_type;

  ArgbColorMixer() : m_alphaAccum(), m_redAccum(), m_greenAccum(), m_blueAccum() {}

  /** @see GrayColorMixer:add() */
  void add(const uint32_t argb, const AccumType weight) {
    const AccumType alpha = AccumType((argb >> 24) & 0xFF);
    const AccumType alpha_weight = alpha * weight;
    m_alphaAccum += alpha_weight;
    m_redAccum += AccumType((argb >> 16) & 0xFF) * alpha_weight;
    m_greenAccum += AccumType((argb >> 8) & 0xFF) * alpha_weight;
    m_blueAccum += AccumType(argb & 0xFF) * alpha_weight;
  }

  /** @see GrayColorMixer::mix() */
  result_type mix(AccumType total_weight) const {
    assert(total_weight > 0);
    if (m_alphaAccum == AccumType(0)) {
      // A totally transparent color. This can happen when mixing
      // a bunch of (possibly different) colors with alpha == 0.
      // This branch prevents a division by zero in *IntegerMix().
      return 0;
    }

    using namespace color_mixer_impl;
    typedef std::numeric_limits<AccumType> traits;
    return Switcher<ArgbColorMixer<AccumType>, traits::is_integer>::mix(this, total_weight);
  }

 private:
  uint32_t nonIntegerMix(AccumType total_weight) const {
    assert(total_weight > 0);
    assert(m_alphaAccum > 0);
    const AccumType scale1 = AccumType(1) / total_weight;
    const AccumType scale2 = AccumType(1) / m_alphaAccum;
    const uint32_t a = uint32_t(AccumType(0.5) + m_alphaAccum * scale1);
    const uint32_t r = uint32_t(AccumType(0.5) + m_redAccum * scale2);
    const uint32_t g = uint32_t(AccumType(0.5) + m_greenAccum * scale2);
    const uint32_t b = uint32_t(AccumType(0.5) + m_blueAccum * scale2);
    return (a << 24) | (r << 16) | (g << 8) | b;
  }

  uint32_t integerMix(AccumType total_weight) const {
    assert(total_weight > 0);
    assert(m_alphaAccum > 0);
    const AccumType half_weight1 = total_weight >> 1;
    const AccumType half_weight2 = m_alphaAccum >> 1;
    const uint32_t a = uint32_t((m_alphaAccum + half_weight1) / total_weight);
    const uint32_t r = uint32_t((m_redAccum + half_weight2) / m_alphaAccum);
    const uint32_t g = uint32_t((m_greenAccum + half_weight2) / m_alphaAccum);
    const uint32_t b = uint32_t((m_blueAccum + half_weight2) / m_alphaAccum);
    return (a << 24) | (r << 16) | (g << 8) | b;
  }

  AccumType m_alphaAccum;
  AccumType m_redAccum;
  AccumType m_greenAccum;
  AccumType m_blueAccum;
};
}  // namespace imageproc

#endif  // ifndef IMAGEPROC_COLOR_MIXER_H_
