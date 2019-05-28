
#ifndef SCANTAILOR_LIGHTCOLORSCHEME_H
#define SCANTAILOR_LIGHTCOLORSCHEME_H

#include <QPalette>
#include <QString>
#include <memory>
#include "ColorScheme.h"

class LightColorScheme : public ColorScheme {
 public:
  LightColorScheme();

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


#endif  //SCANTAILOR_LIGHTCOLORSCHEME_H
