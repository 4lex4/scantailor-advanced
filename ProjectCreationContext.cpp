/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "ProjectCreationContext.h"
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <cassert>
#include "FixDpiDialog.h"
#include "ProjectFilesDialog.h"

ProjectCreationContext::ProjectCreationContext(QWidget* parent) : m_layoutDirection(Qt::LeftToRight), m_parent(parent) {
  showProjectFilesDialog();
}

ProjectCreationContext::~ProjectCreationContext() {
  // Deleting a null pointer is OK.
  delete m_projectFilesDialog;
  delete m_fixDpiDialog;
}

namespace {
template <typename T>
bool allDpisOK(const T& container) {
  using namespace boost::lambda;

  return std::find_if(container.begin(), container.end(), !bind(&ImageFileInfo::isDpiOK, _1)) == container.end();
}
}  // anonymous namespace

void ProjectCreationContext::projectFilesSubmitted() {
  m_files = m_projectFilesDialog->inProjectFiles();
  m_outDir = m_projectFilesDialog->outputDirectory();
  m_layoutDirection = Qt::LeftToRight;
  if (m_projectFilesDialog->isRtlLayout()) {
    m_layoutDirection = Qt::RightToLeft;
  }

  if (!m_projectFilesDialog->isDpiFixingForced() && allDpisOK(m_files)) {
    emit done(this);
  } else {
    showFixDpiDialog();
  }
}

void ProjectCreationContext::projectFilesDialogDestroyed() {
  if (!m_fixDpiDialog) {
    deleteLater();
  }
}

void ProjectCreationContext::fixedDpiSubmitted() {
  m_files = m_fixDpiDialog->files();
  emit done(this);
}

void ProjectCreationContext::fixDpiDialogDestroyed() {
  deleteLater();
}

void ProjectCreationContext::showProjectFilesDialog() {
  assert(!m_projectFilesDialog);
  m_projectFilesDialog = new ProjectFilesDialog(m_parent);
  m_projectFilesDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_projectFilesDialog->setAttribute(Qt::WA_QuitOnClose, false);
  if (m_parent) {
    m_projectFilesDialog->setWindowModality(Qt::WindowModal);
  }
  connect(m_projectFilesDialog, SIGNAL(accepted()), this, SLOT(projectFilesSubmitted()));
  connect(m_projectFilesDialog, SIGNAL(destroyed(QObject*)), this, SLOT(projectFilesDialogDestroyed()));
  m_projectFilesDialog->show();
}

void ProjectCreationContext::showFixDpiDialog() {
  assert(!m_fixDpiDialog);
  m_fixDpiDialog = new FixDpiDialog(m_files, m_parent);
  m_fixDpiDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_fixDpiDialog->setAttribute(Qt::WA_QuitOnClose, false);
  if (m_parent) {
    m_fixDpiDialog->setWindowModality(Qt::WindowModal);
  }
  connect(m_fixDpiDialog, SIGNAL(accepted()), this, SLOT(fixedDpiSubmitted()));
  connect(m_fixDpiDialog, SIGNAL(destroyed(QObject*)), this, SLOT(fixDpiDialogDestroyed()));
  m_fixDpiDialog->show();
}
