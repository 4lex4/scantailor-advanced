#include "ColorSchemeFactory.h"
#include "ColorScheme.h"
#include "NativeScheme.h"
#include "LightScheme.h"
#include "DarkScheme.h"

ColorSchemeFactory::ColorSchemeFactory() {
  registerColorScheme("native", []() { return std::make_unique<NativeScheme>(); });
  registerColorScheme("light", []() { return std::make_unique<LightScheme>(); });
  registerColorScheme("dark", []() { return std::make_unique<DarkScheme>(); });
}

void ColorSchemeFactory::registerColorScheme(const QString& scheme,
                                             const ColorSchemeConstructor& constructor) {
  m_registry[scheme] = constructor;
}

std::unique_ptr<ColorScheme> ColorSchemeFactory::create(const QString& scheme) const {
  return m_registry.at(scheme)();
}
