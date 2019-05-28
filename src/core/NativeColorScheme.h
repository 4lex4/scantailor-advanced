#ifndef SCANTAILOR_ADVANCED_NATIVESCHEME_H
#define SCANTAILOR_ADVANCED_NATIVESCHEME_H

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

#endif  // SCANTAILOR_ADVANCED_NATIVESCHEME_H
