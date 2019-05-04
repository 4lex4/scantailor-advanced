
#include "ColorSchemeManager.h"
#include <QtGui/QFont>
#include <QtWidgets/QApplication>

ColorSchemeManager& ColorSchemeManager::instance() {
  static ColorSchemeManager instance;
  return instance;
}

void ColorSchemeManager::setColorScheme(const ColorScheme& colorScheme) {
  if (QStyle* style = colorScheme.getStyle()) {
    qApp->setStyle(style);
  }
  qApp->setPalette(colorScheme.getPalette());
  if (std::unique_ptr<QString> styleSheet = colorScheme.getStyleSheet()) {
    qApp->setStyleSheet(*styleSheet);
  }
  m_colorParams = colorScheme.getColorParams();
}

QBrush ColorSchemeManager::getColorParam(const ColorScheme::ColorParam colorParam, const QBrush& defaultBrush) const {
  const auto it = m_colorParams.find(colorParam);
  if (it != m_colorParams.end()) {
    return QBrush(it->second, defaultBrush.style());
  } else {
    return defaultBrush;
  }
}

QColor ColorSchemeManager::getColorParam(ColorScheme::ColorParam colorParam, const QColor& defaultColor) const {
  const auto it = m_colorParams.find(colorParam);
  if (it != m_colorParams.end()) {
    return it->second;
  } else {
    return defaultColor;
  }
}
