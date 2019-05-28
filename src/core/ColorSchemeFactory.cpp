#include "ColorSchemeFactory.h"
#include "ColorScheme.h"
#include "DarkColorScheme.h"
#include "LightColorScheme.h"
#include "NativeColorScheme.h"

ColorSchemeFactory::ColorSchemeFactory() {
  registerColorScheme("native", []() { return std::make_unique<NativeColorScheme>(); });
  registerColorScheme("light", []() { return std::make_unique<LightColorScheme>(); });
  registerColorScheme("dark", []() { return std::make_unique<DarkColorScheme>(); });
}

void ColorSchemeFactory::registerColorScheme(const QString& scheme, const ColorSchemeConstructor& constructor) {
  m_registry[scheme] = constructor;
}

std::unique_ptr<ColorScheme> ColorSchemeFactory::create(const QString& scheme) const {
  return m_registry.at(scheme)();
}
