
#include "ColorSchemeManager.h"
#include <QtGui/QFont>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleFactory>

std::unique_ptr<ColorSchemeManager> ColorSchemeManager::m_instance = nullptr;

ColorSchemeManager* ColorSchemeManager::instance() {
  if (m_instance == nullptr) {
    m_instance.reset(new ColorSchemeManager());
  }

  return m_instance.get();
}

void ColorSchemeManager::setColorScheme(const ColorScheme& colorScheme) {
  qApp->setStyle(QStyleFactory::create("Fusion"));

  qApp->setPalette(*colorScheme.getPalette());

  std::unique_ptr<QString> styleSheet = colorScheme.getStyleSheet();
  if (styleSheet != nullptr) {
    qApp->setStyleSheet(*styleSheet);
  }

  m_colorParams = colorScheme.getColorParams();
}

QBrush ColorSchemeManager::getColorParam(const std::string& colorParam, const QBrush& defaultColor) const {
  if (m_colorParams->find(colorParam) != m_colorParams->end()) {
    return m_colorParams->at(colorParam);
  } else {
    return defaultColor;
  }
}
