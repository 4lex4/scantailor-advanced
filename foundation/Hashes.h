
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
