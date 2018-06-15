/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef OUTPUT_RENDER_PARAMS_H_
#define OUTPUT_RENDER_PARAMS_H_

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
#endif  // ifndef OUTPUT_RENDER_PARAMS_H_
