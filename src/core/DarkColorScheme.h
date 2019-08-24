// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_DARKCOLORSCHEME_H_
#define SCANTAILOR_CORE_DARKCOLORSCHEME_H_

#include <QPalette>
#include <QString>
#include "ColorScheme.h"

class DarkColorScheme : public ColorScheme {
 public:
  DarkColorScheme();

  QStyle* getStyle() const override;

  const QPalette* getPalette() const override;

  const QString* getStyleSheet() const override;

  const ColorParams* getColorParams() const override;

 private:
  void loadPalette();

  void loadStyleSheet();

  void loadColorParams();

  QPalette m_palette;
  QString m_styleSheet;
  ColorParams m_customColors;
};


#endif  // SCANTAILOR_CORE_DARKCOLORSCHEME_H_
