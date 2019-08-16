// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
