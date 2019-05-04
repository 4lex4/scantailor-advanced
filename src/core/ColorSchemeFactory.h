#ifndef SCANTAILOR_COLORSCHEMEFACTORY_H
#define SCANTAILOR_COLORSCHEMEFACTORY_H


#include <foundation/Hashes.h>
#include <foundation/NonCopyable.h>
#include <QString>
#include <memory>
#include <unordered_map>
#include <functional>

class ColorScheme;

class ColorSchemeFactory {
  DECLARE_NON_COPYABLE(ColorSchemeFactory)
 public:
  ColorSchemeFactory();

  using ColorSchemeConstructor = std::function<std::unique_ptr<ColorScheme>()>;

  void registerColorScheme(const QString& scheme, const ColorSchemeConstructor& constructor);

  std::unique_ptr<ColorScheme> create(const QString& scheme) const;

 private:
  typedef std::unordered_map<QString, ColorSchemeConstructor, hashes::hash<QString>> Registry;

  Registry m_registry;
};


#endif  // SCANTAILOR_COLORSCHEMEFACTORY_H
