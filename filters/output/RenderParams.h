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
        RenderParams()
                : m_mask(0) {
        }

        explicit RenderParams(ColorParams const& colorParams, SplittingOptions const& splittingOptions);

        bool cutMargins() const;

        bool normalizeIllumination() const;

        bool normalizeIlluminationColor() const;

        bool needBinarization() const;

        bool mixedOutput() const;

        bool binaryOutput() const;

        bool needSavitzkyGolaySmoothing() const;

        bool needMorphologicalSmoothing() const;

        bool splitOutput() const;

        bool originalBackground() const;

    private:
        enum {
            CUT_MARGINS = 1,
            NORMALIZE_ILLUMINATION = 2,
            NEED_BINARIZATION = 4,
            MIXED_OUTPUT = 8,
            NORMALIZE_ILLUMINATION_COLOR = 16,
            SAVITZKY_GOLAY_SMOOTHING = 32,
            MORPHOLOGICAL_SMOOTHING = 64,
            SPLIT_OUTPUT = 128,
            ORIGINAL_BACKGROUND = 256
        };

        int m_mask;
    };
}  // namespace output
#endif  // ifndef OUTPUT_RENDER_PARAMS_H_
