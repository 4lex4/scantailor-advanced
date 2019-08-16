// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
