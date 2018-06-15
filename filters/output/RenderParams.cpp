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

#include "RenderParams.h"
#include "ColorParams.h"

namespace output {
RenderParams::RenderParams(const ColorParams& colorParams, const SplittingOptions& splittingOptions) : m_mask(0) {
  const BlackWhiteOptions blackWhiteOptions(colorParams.blackWhiteOptions());
  const ColorCommonOptions colorCommonOptions(colorParams.colorCommonOptions());
  const ColorMode colorMode = colorParams.colorMode();

  if ((colorMode == BLACK_AND_WHITE) || (colorMode == MIXED)) {
    m_mask |= NEED_BINARIZATION;
    if ((colorMode == MIXED) && splittingOptions.isSplitOutput()) {
      m_mask |= SPLIT_OUTPUT;
      if (splittingOptions.getSplittingMode() == COLOR_FOREGROUND) {
        m_mask ^= NEED_BINARIZATION;
      }
      if ((splittingOptions.getSplittingMode() == BLACK_AND_WHITE_FOREGROUND)
          && (splittingOptions.isOriginalBackground())) {
        m_mask |= ORIGINAL_BACKGROUND;
      }
    }
    if (colorMode == MIXED) {
      m_mask |= MIXED_OUTPUT;
    }
    if (blackWhiteOptions.isSavitzkyGolaySmoothingEnabled()) {
      m_mask |= SAVITZKY_GOLAY_SMOOTHING;
    }
    if (blackWhiteOptions.isMorphologicalSmoothingEnabled()) {
      m_mask |= MORPHOLOGICAL_SMOOTHING;
    }
    if (blackWhiteOptions.normalizeIllumination()) {
      m_mask |= NORMALIZE_ILLUMINATION;
    }
    if (blackWhiteOptions.getColorSegmenterOptions().isEnabled()) {
      m_mask |= COLOR_SEGMENTATION;
      if (colorCommonOptions.getPosterizationOptions().isEnabled()) {
        m_mask |= POSTERIZE;
      }
    }
  } else {
    if (colorCommonOptions.getPosterizationOptions().isEnabled()) {
      m_mask |= POSTERIZE;
    }
  }
  if (colorCommonOptions.fillMargins()) {
    m_mask |= FILL_MARGINS;
  }
  if (colorCommonOptions.fillOffcut()) {
    m_mask |= FILL_OFFCUT;
  }
  if (colorCommonOptions.normalizeIllumination()) {
    m_mask |= NORMALIZE_ILLUMINATION_COLOR;
  }
}

bool RenderParams::fillMargins() const {
  return (m_mask & FILL_MARGINS) != 0;
}

bool RenderParams::normalizeIllumination() const {
  return (m_mask & NORMALIZE_ILLUMINATION) != 0;
}

bool RenderParams::normalizeIlluminationColor() const {
  return (m_mask & NORMALIZE_ILLUMINATION_COLOR) != 0;
}

bool RenderParams::needBinarization() const {
  return (m_mask & NEED_BINARIZATION) != 0;
}

bool RenderParams::mixedOutput() const {
  return (m_mask & MIXED_OUTPUT) != 0;
}

bool RenderParams::binaryOutput() const {
  return (m_mask & (NEED_BINARIZATION | MIXED_OUTPUT)) == NEED_BINARIZATION;
}

bool RenderParams::needSavitzkyGolaySmoothing() const {
  return (m_mask & SAVITZKY_GOLAY_SMOOTHING) != 0;
}

bool RenderParams::needMorphologicalSmoothing() const {
  return (m_mask & MORPHOLOGICAL_SMOOTHING) != 0;
}

bool RenderParams::splitOutput() const {
  return (m_mask & SPLIT_OUTPUT) != 0;
}

bool RenderParams::originalBackground() const {
  return (m_mask & ORIGINAL_BACKGROUND) != 0;
}

bool RenderParams::needColorSegmentation() const {
  return (m_mask & COLOR_SEGMENTATION) != 0;
}

bool RenderParams::posterize() const {
  return (m_mask & POSTERIZE) != 0;
}

bool RenderParams::fillOffcut() const {
  return (m_mask & FILL_OFFCUT) != 0;
}
}  // namespace output
