// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_UTILS_H_
#define SCANTAILOR_CORE_UTILS_H_

#include <QString>
#include <map>
#include <unordered_map>

#include "ThumbnailPixmapCache.h"

namespace core {
class Utils {
 public:
  template <typename K, typename V, typename Comp, typename Alloc>
  static typename std::map<K, V, Comp, Alloc>::iterator mapSetValue(std::map<K, V, Comp, Alloc>& map,
                                                                    const K& key,
                                                                    const V& val);

  template <typename K, typename V, typename Hash, typename Pred, typename Alloc>
  static typename std::unordered_map<K, V, Hash, Pred, Alloc>::iterator
  mapSetValue(std::unordered_map<K, V, Hash, Pred, Alloc>& map, const K& key, const V& val);

  template <typename T>
  static T castOrFindChild(QObject* object);

  /**
   * \brief If \p outputDir exists, creates a "cache" subdirectory under it.
   *
   * The idea is to prevent creating a bogus directory structure when loading
   * a project created on another machine.
   */
  static void maybeCreateCacheDir(const QString& outputDir);

  static QString outputDirToThumbDir(const QString& outputDir);

  static intrusive_ptr<ThumbnailPixmapCache> createThumbnailCache(const QString& outputDir);

  /**
   * Unlike QFile::rename(), this one overwrites existing files.
   */
  static bool overwritingRename(const QString& from, const QString& to);

  /**
   * \brief Generate rich text, complete with headers and stuff,
   *        for a clickable link.
   *
   * \param label The text to show as a link.
   * \param target A URL or something else.  If the link will
   *        be used in a QLable, this string will be passed
   *        to QLabel::linkActivated(const QString&).
   * \return The resulting reach text.
   */
  static QString richTextForLink(const QString& label, const QString& target = QString(QChar('#')));

  static QString qssConvertPxToEm(const QString& stylesheet, double base, int precise);

  Utils() = delete;
};


template <typename K, typename V, typename Comp, typename Alloc>
typename std::map<K, V, Comp, Alloc>::iterator Utils::mapSetValue(std::map<K, V, Comp, Alloc>& map,
                                                                  const K& key,
                                                                  const V& val) {
  const auto it(map.lower_bound(key));
  if ((it == map.end()) || map.key_comp()(key, it->first)) {
    return map.insert(it, typename std::map<K, V, Comp, Alloc>::value_type(key, val));
  } else {
    it->second = val;
    return it;
  }
}

template <typename K, typename V, typename Hash, typename Pred, typename Alloc>
typename std::unordered_map<K, V, Hash, Pred, Alloc>::iterator
Utils::mapSetValue(std::unordered_map<K, V, Hash, Pred, Alloc>& map, const K& key, const V& val) {
  const auto it(map.find(key));
  if (it == map.end()) {
    return map.insert(it, typename std::unordered_map<K, V, Hash, Pred, Alloc>::value_type(key, val));
  } else {
    it->second = val;
    return it;
  }
}

template <typename T>
T Utils::castOrFindChild(QObject* object) {
  if (!object) {
    return nullptr;
  }

  if (auto result = dynamic_cast<T>(object)) {
    return result;
  } else {
    for (QObject* child : object->children()) {
      if ((result = castOrFindChild<T>(child))) {
        return result;
      }
    }
  }
  return nullptr;
}
}  // namespace core

#endif  // ifndef SCANTAILOR_CORE_UTILS_H_
