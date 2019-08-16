// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_ICONPACK_H
#define SCANTAILOR_ICONPACK_H

#include <QIcon>
#include <memory>

class QString;

class IconPack {
 public:
  virtual ~IconPack() = default;

  virtual QIcon getIcon(const QString& iconKey) const = 0;

  virtual void mergeWith(std::unique_ptr<IconPack> pack) = 0;
};

#endif  // SCANTAILOR_ICONPACK_H
