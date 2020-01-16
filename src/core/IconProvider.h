// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_ICONPROVIDER_H_
#define SCANTAILOR_CORE_ICONPROVIDER_H_


#include <foundation/NonCopyable.h>

#include <QtGui/QIcon>
#include <memory>

class IconPack;

class IconProvider {
  DECLARE_NON_COPYABLE(IconProvider)
 private:
  IconProvider();

  ~IconProvider();

 public:
  static IconProvider& getInstance();

  void setIconPack(std::unique_ptr<IconPack> pack);

  void addIconPack(std::unique_ptr<IconPack> pack);

  QIcon getIcon(const QString& iconKey) const;

 private:
  std::unique_ptr<IconPack> m_iconPack;
};


#endif  // SCANTAILOR_CORE_ICONPROVIDER_H_
