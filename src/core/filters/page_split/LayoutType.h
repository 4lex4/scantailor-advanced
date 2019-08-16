// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PAGE_SPLIT_LAYOUT_TYPE_H_
#define PAGE_SPLIT_LAYOUT_TYPE_H_

#include <QString>

namespace page_split {
enum LayoutType { AUTO_LAYOUT_TYPE, SINGLE_PAGE_UNCUT, PAGE_PLUS_OFFCUT, TWO_PAGES };

QString layoutTypeToString(LayoutType type);

LayoutType layoutTypeFromString(const QString& layout_type);
}  // namespace page_split
#endif
