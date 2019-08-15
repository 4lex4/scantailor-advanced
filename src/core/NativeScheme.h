#ifndef SCANTAILOR_ADVANCED_NATIVESCHEME_H
#define SCANTAILOR_ADVANCED_NATIVESCHEME_H

#include <QtCore>
#include <QtGui>
#include <memory>
#include "ColorScheme.h"

class NativeScheme : public ColorScheme {
 public:
  QStyle* getStyle() const override;

  QPalette getPalette() const override;

  std::unique_ptr<QString> getStyleSheet() const override;

  ColorParams getColorParams() const override;
};

#endif //SCANTAILOR_ADVANCED_NATIVESCHEME_H
