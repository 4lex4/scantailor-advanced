#ifndef SCANTAILOR_FONTICONPACK_H
#define SCANTAILOR_FONTICONPACK_H

#include <foundation/Hashes.h>
#include <QString>
#include <unordered_map>
#include "AbstractIconPack.h"

class QDomElement;

class FontIconPack : public AbstractIconPack {
 public:
  static std::unique_ptr<FontIconPack> createDefault();

  explicit FontIconPack(const QString& iconSheetPath);

  QIcon getIcon(const QString& iconKey) const override;

 private:
  void addIconFromElement(const QDomElement& iconElement);

  std::unordered_map<QString, QIcon, hashes::hash<QString>> m_iconMap;
};


#endif //SCANTAILOR_FONTICONPACK_H
