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
    RenderParams::RenderParams(ColorParams const& colorParams, SplittingOptions const& splittingOptions)
            : m_mask(0) {
        const BlackWhiteOptions blackWhiteOptions(colorParams.blackWhiteOptions());
        const ColorCommonOptions colorCommonOptions(colorParams.colorCommonOptions());
        const ColorParams::ColorMode colorMode = colorParams.colorMode();

        if ((colorMode == ColorParams::BLACK_AND_WHITE) || (colorMode == ColorParams::MIXED)) {
            m_mask |= NEED_BINARIZATION;
            if ((colorMode == ColorParams::MIXED) && splittingOptions.isSplitOutput()) {
                m_mask |= SPLIT_OUTPUT;
                if (splittingOptions.getForegroundType() == SplittingOptions::COLOR_FOREGROUND) {
                    m_mask ^= NEED_BINARIZATION;
                }
                if ((splittingOptions.getForegroundType() == SplittingOptions::BLACK_AND_WHITE_FOREGROUND)
                    && (splittingOptions.isOriginalBackground())) {
                    m_mask |= ORIGINAL_BACKGROUND;
                }
            }
            if (colorMode == ColorParams::MIXED) {
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
        }
        if (colorCommonOptions.cutMargins()) {
            m_mask |= CUT_MARGINS;
        }
        if (colorCommonOptions.normalizeIllumination()) {
            m_mask |= NORMALIZE_ILLUMINATION_COLOR;
        }
    }
}  // namespace output
