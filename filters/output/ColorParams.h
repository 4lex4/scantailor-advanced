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

#ifndef OUTPUT_COLORPARAMS_H_
#define OUTPUT_COLORPARAMS_H_

#include "BlackWhiteOptions.h"
#include "ColorCommonOptions.h"
#include "SplittingOptions.h"

class QDomDocument;
class QDomElement;

namespace output {
enum ColorMode { BLACK_AND_WHITE, COLOR_GRAYSCALE, MIXED };

class ColorParams {
 public:
  ColorParams();

  explicit ColorParams(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  ColorMode colorMode() const;

  void setColorMode(ColorMode mode);

  const ColorCommonOptions& colorCommonOptions() const;

  void setColorCommonOptions(const ColorCommonOptions& opt);

  const BlackWhiteOptions& blackWhiteOptions() const;

  void setBlackWhiteOptions(const BlackWhiteOptions& opt);

 private:
  static ColorMode parseColorMode(const QString& str);

  static QString formatColorMode(ColorMode mode);


  ColorMode m_colorMode;
  ColorCommonOptions m_colorCommonOptions;
  BlackWhiteOptions m_bwOptions;
};
}  // namespace output
#endif  // ifndef OUTPUT_COLORPARAMS_H_
