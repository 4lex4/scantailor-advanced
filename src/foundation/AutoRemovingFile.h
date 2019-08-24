// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_AUTOREMOVINGFILE_H_
#define SCANTAILOR_FOUNDATION_AUTOREMOVINGFILE_H_

#include <QString>

/**
 * \brief Provides unique_ptr semantics for files.
 *
 * Just like std::unique_ptr deleting its object when it goes out of scope,
 * this class deletes a file.  unique_ptr's copying semantics is also preserved.
 */
class AutoRemovingFile {
 private:
  struct CopyHelper {
    AutoRemovingFile* obj;

    explicit CopyHelper(AutoRemovingFile* o) : obj(o) {}
  };

 public:
  AutoRemovingFile();

  explicit AutoRemovingFile(const QString& filePath);

  AutoRemovingFile(AutoRemovingFile& other);

  AutoRemovingFile(CopyHelper other);

  ~AutoRemovingFile();

  AutoRemovingFile& operator=(AutoRemovingFile& other);

  AutoRemovingFile& operator=(CopyHelper other);

  operator CopyHelper() { return CopyHelper(this); }

  const QString& get() const { return m_file; }

  void reset(const QString& file);

  QString release();

 private:
  QString m_file;
};


#endif  // ifndef SCANTAILOR_FOUNDATION_AUTOREMOVINGFILE_H_
