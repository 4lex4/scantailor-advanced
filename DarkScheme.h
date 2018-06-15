
#ifndef SCANTAILOR_DARKSCHEME_H
#define SCANTAILOR_DARKSCHEME_H

#include <QStyleFactory>
#include <QtCore>
#include <QtGui>
#include <memory>
#include "ColorScheme.h"

class DarkScheme : public ColorScheme {
 public:
  std::unique_ptr<QPalette> getPalette() const override;

  std::unique_ptr<QString> getStyleSheet() const override;

  std::unique_ptr<ColorParams> getColorParams() const override;
};


#endif  // SCANTAILOR_DARKSCHEME_H
