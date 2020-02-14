// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_RENDERPARAMS_H_
#define SCANTAILOR_OUTPUT_RENDERPARAMS_H_

#include "SplittingOptions.h"

namespace output {
class ColorParams;

class RenderParams {
 public:
  RenderParams() : m_mask(0) {}

  explicit RenderParams(const ColorParams& colorParams, const SplittingOptions& splittingOptions);

  bool fillOffcut() const;

  bool fillMargins() const;

  bool normalizeIllumination() const;

  bool normalizeIlluminationColor() const;

  bool needBinarization() const;

  bool mixedOutput() const;

  bool binaryOutput() const;

  bool needSavitzkyGolaySmoothing() const;

  bool needMorphologicalSmoothing() const;

  bool splitOutput() const;

  bool originalBackground() const;

  bool needColorSegmentation() const;

  bool posterize() const;

 private:
  enum {
    FILL_MARGINS = 1,
    NORMALIZE_ILLUMINATION = 1 << 1,
    NEED_BINARIZATION = 1 << 2,
    MIXED_OUTPUT = 1 << 3,
    NORMALIZE_ILLUMINATION_COLOR = 1 << 4,
    SAVITZKY_GOLAY_SMOOTHING = 1 << 5,
    MORPHOLOGICAL_SMOOTHING = 1 << 6,
    SPLIT_OUTPUT = 1 << 7,
    ORIGINAL_BACKGROUND = 1 << 8,
    COLOR_SEGMENTATION = 1 << 9,
    POSTERIZE = 1 << 10,
    FILL_OFFCUT = 1 << 11
  };

  int m_mask;
};


inline bool RenderParams::fillMargins() const {
  return (m_mask & FILL_MARGINS) != 0;
}

inline bool RenderParams::normalizeIllumination() const {
  return (m_mask & NORMALIZE_ILLUMINATION) != 0;
}

inline bool RenderParams::normalizeIlluminationColor() const {
  return (m_mask & NORMALIZE_ILLUMINATION_COLOR) != 0;
}

inline bool RenderParams::needBinarization() const {
  return (m_mask & NEED_BINARIZATION) != 0;
}

inline bool RenderParams::mixedOutput() const {
  return (m_mask & MIXED_OUTPUT) != 0;
}

inline bool RenderParams::binaryOutput() const {
  return (m_mask & (NEED_BINARIZATION | MIXED_OUTPUT)) == NEED_BINARIZATION;
}

inline bool RenderParams::needSavitzkyGolaySmoothing() const {
  return (m_mask & SAVITZKY_GOLAY_SMOOTHING) != 0;
}

inline bool RenderParams::needMorphologicalSmoothing() const {
  return (m_mask & MORPHOLOGICAL_SMOOTHING) != 0;
}

inline bool RenderParams::splitOutput() const {
  return (m_mask & SPLIT_OUTPUT) != 0;
}

inline bool RenderParams::originalBackground() const {
  return (m_mask & ORIGINAL_BACKGROUND) != 0;
}

inline bool RenderParams::needColorSegmentation() const {
  return (m_mask & COLOR_SEGMENTATION) != 0;
}

inline bool RenderParams::posterize() const {
  return (m_mask & POSTERIZE) != 0;
}

inline bool RenderParams::fillOffcut() const {
  return (m_mask & FILL_OFFCUT) != 0;
}
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_RENDERPARAMS_H_
