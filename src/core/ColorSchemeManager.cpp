
#include "ColorSchemeManager.h"
#include <QApplication>

ColorSchemeManager& ColorSchemeManager::instance() {
  static ColorSchemeManager instance;
  return instance;
}

void ColorSchemeManager::setColorScheme(const ColorScheme& colorScheme) {
  if (QStyle* style = colorScheme.getStyle()) {
    qApp->setStyle(style);
  }
  if (const QPalette* palette = colorScheme.getPalette()) {
    qApp->setPalette(*palette);
  }
  if (const QString* styleSheet = colorScheme.getStyleSheet()) {
    qApp->setStyleSheet(*styleSheet);
  }
  if (const ColorScheme::ColorParams* colorParams = colorScheme.getColorParams()) {
    m_colorParams = std::make_unique<ColorScheme::ColorParams>(*colorParams);
  }
}

QBrush ColorSchemeManager::getColorParam(const ColorScheme::ColorParam colorParam, const QBrush& defaultBrush) const {
  if (!m_colorParams) {
    return defaultBrush;
  }

  const auto it = m_colorParams->find(colorParam);
  if (it != m_colorParams->end()) {
    return QBrush(it->second, defaultBrush.style());
  } else {
    return defaultBrush;
  }
}

QColor ColorSchemeManager::getColorParam(const ColorScheme::ColorParam colorParam, const QColor& defaultColor) const {
  if (!m_colorParams) {
    return defaultColor;
  }

  const auto it = m_colorParams->find(colorParam);
  if (it != m_colorParams->end()) {
    return it->second;
  } else {
    return defaultColor;
  }
}
