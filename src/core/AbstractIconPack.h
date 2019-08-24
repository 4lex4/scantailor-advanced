// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_ABSTRACTICONPACK_H_
#define SCANTAILOR_CORE_ABSTRACTICONPACK_H_

#include "IconPack.h"

class AbstractIconPack : public IconPack {
 public:
  QIcon getIcon(const QString& iconKey) const override;

  void mergeWith(std::unique_ptr<IconPack> pack) override;

 protected:
  static QIcon::Mode iconModeFromString(const QString& mode);

  static QIcon::State iconStateFromString(const QString& state);

 private:
  std::unique_ptr<IconPack> m_parentIconPack;
};


#endif  // SCANTAILOR_CORE_ABSTRACTICONPACK_H_
