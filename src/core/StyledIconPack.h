#ifndef SCANTAILOR_STYLEDICONPACK_H
#define SCANTAILOR_STYLEDICONPACK_H


#include <foundation/Hashes.h>
#include <QString>
#include <unordered_map>
#include "AbstractIconPack.h"

class QDomElement;

class StyledIconPack : public AbstractIconPack {
 public:
  static std::unique_ptr<StyledIconPack> createDefault();

  explicit StyledIconPack(const QString& iconSheetPath);

  QIcon getIcon(const QString& iconKey) const override;

 private:
  void addIconFromElement(const QDomElement& iconElement);

  std::unordered_map<QString, QIcon, hashes::hash<QString>> m_iconMap;
};


#endif  // SCANTAILOR_STYLEDICONPACK_H
