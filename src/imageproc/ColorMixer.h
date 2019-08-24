// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_COLORMIXER_H_
#define SCANTAILOR_IMAGEPROC_COLORMIXER_H_

#include <cassert>
#include <cstdint>
#include <limits>

namespace imageproc {
namespace color_mixer_impl {
template <typename Mixer, bool IntegerAccum>
struct Switcher {
  using accumType = typename Mixer::accumType;
  using resultType = typename Mixer::resultType;

  static resultType mix(const Mixer* mixer, accumType totalWeight) { return mixer->nonIntegerMix(totalWeight); }
};

template <typename Mixer>
struct Switcher<Mixer, true> {
  using accumType = typename Mixer::accumType;
  using resultType = typename Mixer::resultType;

  static resultType mix(const Mixer* mixer, accumType totalWeight) { return mixer->integerMix(totalWeight); }
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
  using accumType = AccumType;
  using resultType = uint8_t;

  GrayColorMixer() : m_accum() {}

  /**
   * @brief Adds a weighted pixel into the mix.
   *
   * The weight must be non-negative.
   */
  void add(uint8_t grayLevel, AccumType weight) { m_accum += AccumType(grayLevel) * weight; }

  /**
   * @brief Returns a color intepolated from previously added ones.
   *
   * @param totalWeight The sum of individual weights passed to add().
   *        While an individual weight can be zero, the sum has to be above zero.
   * @return Interpolated color.
   */
  resultType mix(AccumType totalWeight) const {
    assert(totalWeight > 0);

    using namespace color_mixer_impl;
    using traits = std::numeric_limits<AccumType>;
    return Switcher<GrayColorMixer<AccumType>, traits::is_integer>::mix(this, totalWeight);
  }

 private:
  uint8_t nonIntegerMix(AccumType totalWeight) const {
    assert(totalWeight > 0);
    return static_cast<uint8_t>(m_accum / totalWeight + AccumType(0.5));
  }

  uint8_t integerMix(AccumType totalWeight) const {
    assert(totalWeight > 0);
    const AccumType halfWeight = totalWeight >> 1;
    const AccumType mixed = (m_accum + halfWeight) / totalWeight;
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
  using accumType = AccumType;
  using resultType = uint32_t;

  RgbColorMixer() : m_redAccum(), m_greenAccum(), m_blueAccum() {}

  /** @see GrayColorMixer::add() */
  void add(uint32_t rgb, AccumType weight) {
    m_redAccum += AccumType((rgb >> 16) & 0xFF) * weight;
    m_greenAccum += AccumType((rgb >> 8) & 0xFF) * weight;
    m_blueAccum += AccumType(rgb & 0xFF) * weight;
  }

  /** @see GrayColorMixer::mix() */
  resultType mix(AccumType totalWeight) const {
    assert(totalWeight > 0);

    using namespace color_mixer_impl;
    using traits = std::numeric_limits<AccumType>;
    return Switcher<RgbColorMixer<AccumType>, traits::is_integer>::mix(this, totalWeight);
  }

 private:
  uint32_t nonIntegerMix(AccumType totalWeight) const {
    assert(totalWeight > 0);
    const AccumType scale = AccumType(1) / totalWeight;
    const uint32_t r = uint32_t(AccumType(0.5) + m_redAccum * scale);
    const uint32_t g = uint32_t(AccumType(0.5) + m_greenAccum * scale);
    const uint32_t b = uint32_t(AccumType(0.5) + m_blueAccum * scale);
    return uint32_t(0xff000000) | (r << 16) | (g << 8) | b;
  }

  uint32_t integerMix(AccumType totalWeight) const {
    assert(totalWeight > 0);
    const AccumType halfWeight = totalWeight >> 1;
    const uint32_t r = uint32_t((m_redAccum + halfWeight) / totalWeight);
    const uint32_t g = uint32_t((m_greenAccum + halfWeight) / totalWeight);
    const uint32_t b = uint32_t((m_blueAccum + halfWeight) / totalWeight);
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
  using accumType = AccumType;
  using resultType = uint32_t;

  ArgbColorMixer() : m_alphaAccum(), m_redAccum(), m_greenAccum(), m_blueAccum() {}

  /** @see GrayColorMixer:add() */
  void add(const uint32_t argb, const AccumType weight) {
    const AccumType alpha = AccumType((argb >> 24) & 0xFF);
    const AccumType alphaWeight = alpha * weight;
    m_alphaAccum += alphaWeight;
    m_redAccum += AccumType((argb >> 16) & 0xFF) * alphaWeight;
    m_greenAccum += AccumType((argb >> 8) & 0xFF) * alphaWeight;
    m_blueAccum += AccumType(argb & 0xFF) * alphaWeight;
  }

  /** @see GrayColorMixer::mix() */
  resultType mix(AccumType totalWeight) const {
    assert(totalWeight > 0);
    if (m_alphaAccum == AccumType(0)) {
      // A totally transparent color. This can happen when mixing
      // a bunch of (possibly different) colors with alpha == 0.
      // This branch prevents a division by zero in *IntegerMix().
      return 0;
    }

    using namespace color_mixer_impl;
    using traits = std::numeric_limits<AccumType>;
    return Switcher<ArgbColorMixer<AccumType>, traits::is_integer>::mix(this, totalWeight);
  }

 private:
  uint32_t nonIntegerMix(AccumType totalWeight) const {
    assert(totalWeight > 0);
    assert(m_alphaAccum > 0);
    const AccumType scale1 = AccumType(1) / totalWeight;
    const AccumType scale2 = AccumType(1) / m_alphaAccum;
    const uint32_t a = uint32_t(AccumType(0.5) + m_alphaAccum * scale1);
    const uint32_t r = uint32_t(AccumType(0.5) + m_redAccum * scale2);
    const uint32_t g = uint32_t(AccumType(0.5) + m_greenAccum * scale2);
    const uint32_t b = uint32_t(AccumType(0.5) + m_blueAccum * scale2);
    return (a << 24) | (r << 16) | (g << 8) | b;
  }

  uint32_t integerMix(AccumType totalWeight) const {
    assert(totalWeight > 0);
    assert(m_alphaAccum > 0);
    const AccumType halfWeight1 = totalWeight >> 1;
    const AccumType halfWeight2 = m_alphaAccum >> 1;
    const uint32_t a = uint32_t((m_alphaAccum + halfWeight1) / totalWeight);
    const uint32_t r = uint32_t((m_redAccum + halfWeight2) / m_alphaAccum);
    const uint32_t g = uint32_t((m_greenAccum + halfWeight2) / m_alphaAccum);
    const uint32_t b = uint32_t((m_blueAccum + halfWeight2) / m_alphaAccum);
    return (a << 24) | (r << 16) | (g << 8) | b;
  }

  AccumType m_alphaAccum;
  AccumType m_redAccum;
  AccumType m_greenAccum;
  AccumType m_blueAccum;
};
}  // namespace imageproc

#endif  // ifndef SCANTAILOR_IMAGEPROC_COLORMIXER_H_
