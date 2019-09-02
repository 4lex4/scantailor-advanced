/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "AtomicFileOverwriter.h"
#include <QFile>
#include <QTemporaryFile>
#include "Utils.h"

using namespace core;

AtomicFileOverwriter::AtomicFileOverwriter() = default;

AtomicFileOverwriter::~AtomicFileOverwriter() {
  abort();
}

QIODevice* AtomicFileOverwriter::startWriting(const QString& file_path) {
  abort();

  m_tempFile = std::make_unique<QTemporaryFile>(file_path);
  m_tempFile->setAutoRemove(false);
  if (!m_tempFile->open()) {
    m_tempFile.reset();
  }

  return m_tempFile.get();
}

bool AtomicFileOverwriter::commit() {
  if (!m_tempFile) {
    return false;
  }

  const QString temp_file_path(m_tempFile->fileName());
  const QString target_path(m_tempFile->fileTemplate());

  // Yes, we have to destroy this object here, because:
  // 1. Under Windows, open files can't be renamed or deleted.
  // 2. QTemporaryFile::close() doesn't really close it.
  m_tempFile.reset();

  if (!Utils::overwritingRename(temp_file_path, target_path)) {
    QFile::remove(temp_file_path);

    return false;
  }

  return true;
}

void AtomicFileOverwriter::abort() {
  if (!m_tempFile) {
    return;
  }

  const QString temp_file_path(m_tempFile->fileName());
  m_tempFile.reset();  // See comments in commit()
  QFile::remove(temp_file_path);
}
