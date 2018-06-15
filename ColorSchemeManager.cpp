
#include "ColorSchemeManager.h"
#include <QtGui/QFont>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleFactory>

std::unique_ptr<ColorSchemeManager> ColorSchemeManager::m_ptrInstance = nullptr;

ColorSchemeManager* ColorSchemeManager::instance() {
  if (m_ptrInstance == nullptr) {
    m_ptrInstance.reset(new ColorSchemeManager());
  }

  return m_ptrInstance.get();
}

void ColorSchemeManager::setColorScheme(const ColorScheme& colorScheme) {
  qApp->setStyle(QStyleFactory::create("Fusion"));

  qApp->setPalette(*colorScheme.getPalette());

  std::unique_ptr<QString> styleSheet = colorScheme.getStyleSheet();
  if (styleSheet != nullptr) {
    qApp->setStyleSheet(*styleSheet);
  }

  m_ptrColorParams = colorScheme.getColorParams();
}

QBrush ColorSchemeManager::getColorParam(const std::string& colorParam, const QBrush& defaultColor) const {
  if (m_ptrColorParams->find(colorParam) != m_ptrColorParams->end()) {
    return m_ptrColorParams->at(colorParam);
  } else {
    return defaultColor;
  }
}
