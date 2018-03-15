
#ifndef SCANTAILOR_LIGHTSCHEME_H
#define SCANTAILOR_LIGHTSCHEME_H

#include <QtCore>
#include <QtGui>
#include <QStyleFactory>
#include <memory>
#include "ColorScheme.h"

class LightScheme : public ColorScheme {
public:
    std::unique_ptr<QPalette> getPalette() const override;

    std::unique_ptr<QString> getStyleSheet() const override;

    std::unique_ptr<ColorParams> getColorParams() const override;
};


#endif  // SCANTAILOR_LIGHTSCHEME_H
