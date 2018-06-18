
#ifndef SCANTAILOR_COLORSCHEMEMANAGER_H
#define SCANTAILOR_COLORSCHEMEMANAGER_H


#include <memory>
#include "ColorScheme.h"

class ColorSchemeManager {
 private:
  static std::unique_ptr<ColorSchemeManager> m_instance;
  std::unique_ptr<ColorParams> m_colorParams;

  ColorSchemeManager() = default;

 public:
  static ColorSchemeManager* instance();

  void setColorScheme(const ColorScheme& colorScheme);

  QBrush getColorParam(const std::string& colorParam, const QBrush& defaultColor) const;
};


#endif  // SCANTAILOR_COLORSCHEMEMANAGER_H
