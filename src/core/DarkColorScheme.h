
#ifndef SCANTAILOR_DARKCOLORSCHEME_H
#define SCANTAILOR_DARKCOLORSCHEME_H

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


#endif  //SCANTAILOR_DARKCOLORSCHEME_H
