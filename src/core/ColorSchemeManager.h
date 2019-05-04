
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
  static ColorSchemeManager& instance();

  void setColorScheme(const ColorScheme& colorScheme);

  QBrush getColorParam(ColorScheme::ColorParam colorParam, const QBrush& defaultBrush) const;

  QColor getColorParam(ColorScheme::ColorParam colorParam, const QColor& defaultColor) const;

 private:
  ColorScheme::ColorParams m_colorParams;
};


#endif  // SCANTAILOR_COLORSCHEMEMANAGER_H
