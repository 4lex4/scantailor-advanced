/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "ProjectOpeningContext.h"
#include <QMessageBox>
#include <cassert>
#include "FixDpiDialog.h"
#include "ProjectPages.h"
#include "version.h"

ProjectOpeningContext::ProjectOpeningContext(QWidget* parent, const QString& project_file, const QDomDocument& doc)
    : m_projectFile(project_file), m_reader(doc), m_parent(parent) {}

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
