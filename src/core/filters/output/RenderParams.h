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
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_RENDERPARAMS_H_
