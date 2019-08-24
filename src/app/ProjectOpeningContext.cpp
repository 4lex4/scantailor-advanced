// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ProjectOpeningContext.h"
#include <QMessageBox>
#include <cassert>
#include "FixDpiDialog.h"
#include "ProjectPages.h"
#include "version.h"

ProjectOpeningContext::ProjectOpeningContext(QWidget* parent, const QString& projectFile, const QDomDocument& doc)
    : m_projectFile(projectFile), m_reader(doc), m_parent(parent) {}

ProjectOpeningContext::~ProjectOpeningContext() {
  // Deleting a null pointer is OK.
  delete m_fixDpiDialog;
}

void ProjectOpeningContext::proceed() {
  if (!m_reader.success()) {
    deleteLater();
    if (!m_reader.getVersion().isNull() && (m_reader.getVersion().toInt() != PROJECT_VERSION)) {
      QMessageBox::warning(m_parent, tr("Error"),
                           tr("The project file is not compatible with the current application version."));

      return;
    }

    QMessageBox::critical(m_parent, tr("Error"), tr("Unable to interpret the project file."));

    return;
  }

  if (m_reader.pages()->validateDpis()) {
    deleteLater();
    emit done(this);

    return;
  }

  showFixDpiDialog();
}

void ProjectOpeningContext::fixedDpiSubmitted() {
  m_reader.pages()->updateMetadataFrom(m_fixDpiDialog->files());
  emit done(this);
}

void ProjectOpeningContext::fixDpiDialogDestroyed() {
  deleteLater();
}

void ProjectOpeningContext::showFixDpiDialog() {
  assert(!m_fixDpiDialog);
  m_fixDpiDialog = new FixDpiDialog(m_reader.pages()->toImageFileInfo(), m_parent);
  m_fixDpiDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_fixDpiDialog->setAttribute(Qt::WA_QuitOnClose, false);
  if (m_parent) {
    m_fixDpiDialog->setWindowModality(Qt::WindowModal);
  }
  connect(m_fixDpiDialog, SIGNAL(accepted()), this, SLOT(fixedDpiSubmitted()));
  connect(m_fixDpiDialog, SIGNAL(destroyed(QObject*)), this, SLOT(fixDpiDialogDestroyed()));
  m_fixDpiDialog->show();
}
