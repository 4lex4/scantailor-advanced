// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
