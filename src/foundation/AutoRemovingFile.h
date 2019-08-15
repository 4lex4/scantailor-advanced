/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AUTO_REMOVING_FILE_H_
#define AUTO_REMOVING_FILE_H_

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

  explicit AutoRemovingFile(const QString& file_path);

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


#endif  // ifndef AUTO_REMOVING_FILE_H_
