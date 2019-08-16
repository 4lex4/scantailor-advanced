// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
