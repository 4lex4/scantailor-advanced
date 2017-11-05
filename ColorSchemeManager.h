//
// Created by Alex on 9/27/2017.
//

#ifndef SCANTAILOR_COLORSCHEMEMANAGER_H
#define SCANTAILOR_COLORSCHEMEMANAGER_H


#include <memory>
#include "ColorScheme.h"

class ColorSchemeManager {
private:
    static std::unique_ptr<ColorSchemeManager> m_ptrInstance;
    std::unique_ptr<ColorParams> m_ptrColorParams;

    ColorSchemeManager() = default;

public:
    static ColorSchemeManager* instance();

    void setColorScheme(const ColorScheme& colorScheme);

    QBrush getColorParam(const std::string& colorParam, const QBrush& defaultColor);
};


#endif //SCANTAILOR_COLORSCHEMEMANAGER_H
