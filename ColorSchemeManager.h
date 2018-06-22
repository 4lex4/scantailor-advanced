
#ifndef SCANTAILOR_COLORSCHEMEMANAGER_H
#define SCANTAILOR_COLORSCHEMEMANAGER_H


#include <foundation/NonCopyable.h>
#include <memory>
#include "ColorScheme.h"

class ColorSchemeManager {
  DECLARE_NON_COPYABLE(ColorSchemeManager)
 private:
  ColorSchemeManager() = default;

 public:
  static ColorSchemeManager* instance();

  void setColorScheme(const ColorScheme& colorScheme);

  QBrush getColorParam(ColorScheme::ColorParam colorParam, const QBrush& defaultColor) const;

 private:
  static std::unique_ptr<ColorSchemeManager> m_instance;

  std::unique_ptr<ColorScheme::ColorParams> m_colorParams;
};


#endif  // SCANTAILOR_COLORSCHEMEMANAGER_H
