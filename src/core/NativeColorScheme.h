// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_NATIVECOLORSCHEME_H_
#define SCANTAILOR_CORE_NATIVECOLORSCHEME_H_

#include <QString>

#include "ColorScheme.h"

class NativeColorScheme : public ColorScheme {
 public:
  NativeColorScheme();

  QStyle* getStyle() const override;

  const QPalette* getPalette() const override;

  const QString* getStyleSheet() const override;

  const ColorParams* getColorParams() const override;

 private:
  void loadStyleSheet();

  void loadColorParams();

  QString m_styleSheet;
  ColorParams m_customColors;
};

#endif  // SCANTAILOR_CORE_NATIVECOLORSCHEME_H_
