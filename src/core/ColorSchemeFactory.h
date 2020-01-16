// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_COLORSCHEMEFACTORY_H_
#define SCANTAILOR_CORE_COLORSCHEMEFACTORY_H_

#include <foundation/Hashes.h>
#include <foundation/NonCopyable.h>

#include <QString>
#include <functional>
#include <memory>
#include <unordered_map>

class ColorScheme;

class ColorSchemeFactory {
  DECLARE_NON_COPYABLE(ColorSchemeFactory)
 public:
  ColorSchemeFactory();

  using ColorSchemeConstructor = std::function<std::unique_ptr<ColorScheme>()>;

  void registerColorScheme(const QString& scheme, const ColorSchemeConstructor& constructor);

  std::unique_ptr<ColorScheme> create(const QString& scheme) const;

 private:
  using Registry = std::unordered_map<QString, ColorSchemeConstructor, hashes::hash<QString>>;

  Registry m_registry;
};


#endif  // SCANTAILOR_CORE_COLORSCHEMEFACTORY_H_
