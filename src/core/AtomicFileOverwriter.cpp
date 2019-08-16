// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
