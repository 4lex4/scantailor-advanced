// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "IconProvider.h"

#include "IconPack.h"

IconProvider::IconProvider() = default;

IconProvider::~IconProvider() = default;

IconProvider& IconProvider::getInstance() {
  static IconProvider instance;
  return instance;
}

void IconProvider::setIconPack(std::unique_ptr<IconPack> pack) {
  m_iconPack = std::move(pack);
}

void IconProvider::addIconPack(std::unique_ptr<IconPack> pack) {
  if (pack) {
    pack->mergeWith(std::move(m_iconPack));
    m_iconPack = std::move(pack);
  }
}

QIcon IconProvider::getIcon(const QString& iconKey) const {
  return m_iconPack ? m_iconPack->getIcon(iconKey) : QIcon();
}
