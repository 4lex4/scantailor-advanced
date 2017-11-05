
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QApplication>
#include <QtGui/QFont>
#include "ColorSchemeManager.h"

std::unique_ptr<ColorSchemeManager> ColorSchemeManager::m_ptrInstance = nullptr;

ColorSchemeManager* ColorSchemeManager::instance() {
    if (m_ptrInstance == nullptr) {
        m_ptrInstance.reset(new ColorSchemeManager());
    }

    return m_ptrInstance.get();
}

void ColorSchemeManager::setColorScheme(const ColorScheme& colorScheme) {
    qApp->setStyle(QStyleFactory::create("Fusion"));

    qApp->setPalette(*colorScheme.getPalette().release());

    if (colorScheme.getStyleSheet() != nullptr) {
        qApp->setStyleSheet(*colorScheme.getStyleSheet().release());
    }

    m_ptrColorParams = colorScheme.getColorParams();
}

QBrush ColorSchemeManager::getColorParam(const std::string& colorParam, const QBrush& defaultColor) {
    if (m_ptrColorParams->count(colorParam) == 1) {
        return m_ptrColorParams->at(colorParam);
    } else {
        return defaultColor;
    }
}
