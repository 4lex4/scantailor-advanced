
#ifndef SCANTAILOR_LIGHTSCHEME_H
#define SCANTAILOR_LIGHTSCHEME_H

#include <QStyleFactory>
#include <QtCore>
#include <QtGui>
#include <memory>
#include "ColorScheme.h"

class LightScheme : public ColorScheme {
 public:
  QPalette getPalette() const override;

  std::unique_ptr<QString> getStyleSheet() const override;

  ColorParams getColorParams() const override;
};


#endif  // SCANTAILOR_LIGHTSCHEME_H
