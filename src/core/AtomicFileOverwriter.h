// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_ATOMICFILEOVERWRITER_H_
#define SCANTAILOR_CORE_ATOMICFILEOVERWRITER_H_

#include <memory>

#include "NonCopyable.h"

class QString;
class QIODevice;
class QTemporaryFile;

/**
 * \brief Overwrites files by writing to a temporary file and then replacing
 *        the target file with it.
 *
 * Because renaming across volumes doesn't work, we create a temporary file
 * in the same directory as the target file.
 */
class AtomicFileOverwriter {
  DECLARE_NON_COPYABLE(AtomicFileOverwriter)

 public:
  AtomicFileOverwriter();

  /**
   * \brief Destroys the object and calls abort() if necessary.
   */
  ~AtomicFileOverwriter();

  /**
   * \brief Start writing to a temporary file.
   *
   * \returns A temporary file as QIODevice, or null of temporary file
   *          could not be opened.  In latter case, calling abort()
   *          is not necessary.
   *
   * If a file is already being written, it calles abort() and then
   * proceeds as usual.
   */
  QIODevice* startWriting(const QString& filePath);

  /**
   * \brief Replaces the target file with the temporary one.
   *
   * If replacing failed, false is returned and the temporary file
   * is removed.
   */
  bool commit();

  /**
   * \brief Removes the temporary file without touching the target one.
   */
  void abort();

 private:
  std::unique_ptr<QTemporaryFile> m_tempFile;
};


#endif  // ifndef SCANTAILOR_CORE_ATOMICFILEOVERWRITER_H_
