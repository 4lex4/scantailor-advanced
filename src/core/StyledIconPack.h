// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_STYLEDICONPACK_H_
#define SCANTAILOR_CORE_STYLEDICONPACK_H_

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


#endif  // SCANTAILOR_CORE_STYLEDICONPACK_H_
