// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_RELINKABLEPATH_H_
#define SCANTAILOR_CORE_RELINKABLEPATH_H_

#include <QString>

/**
 * \brief Represents a file or directory.
 */
class RelinkablePath {
  // Member-wise copying is OK.
 public:
  enum Type { File, Dir };

  RelinkablePath(const QString& path, Type type);

  const QString& normalizedPath() const { return m_normalizedPath; }

  Type type() const { return m_type; }

  /**
   * Performs the following operations on the path:
   * \li Converts backwards slashes to forward slashes.
   * \li Eliminates redundant slashes.
   * \li Eliminates "/./" and resolves "/../" components.
   * \li Removes trailing slashes.
   *
   * \return The normalized string on success or an empty string on failure.
   *         Failure can happen because of unresolvable "/../" components.
   */
  static QString normalize(const QString& path);

 private:
  QString m_normalizedPath;
  Type m_type;
};


#endif  // ifndef SCANTAILOR_CORE_RELINKABLEPATH_H_
