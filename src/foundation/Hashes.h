// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_HASHES_H
#define SCANTAILOR_HASHES_H

#include <QtCore/QString>

namespace hashes {
template <typename>
struct hash;

template <>
struct hash<QString> {
  std::size_t operator()(const QString& str) const noexcept {
    const QChar* data = str.constData();
    std::size_t hash = 5381;
    for (int i = 0; i < str.size(); ++i) {
      hash = ((hash << 5) + hash) ^ ((data[i].row() << 8) | data[i].cell());
    }

    return hash;
  }
};
}  // namespace hashes

#endif  // SCANTAILOR_HASHES_H
