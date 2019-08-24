// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutOfMemoryDialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <utility>
#include "ProjectWriter.h"
#include "RecentProjects.h"

OutOfMemoryDialog::OutOfMemoryDialog(QWidget* parent) : QDialog(parent) {
  ui.setupUi(this);
  if (sizeof(void*) > 4) {
    ui.only_32bit_1->hide();
    ui.only_32bit_2->hide();
  }

  ui.topLevelStack->setCurrentWidget(ui.mainPage);

  connect(ui.saveProjectBtn, SIGNAL(clicked()), SLOT(saveProject()));
  connect(ui.saveProjectAsBtn, SIGNAL(clicked()), SLOT(saveProjectAs()));
  connect(ui.dontSaveBtn, SIGNAL(clicked()), SLOT(reject()));
}

void OutOfMemoryDialog::setParams(const QString& projectFile,
                                  intrusive_ptr<StageSequence> stages,
                                  intrusive_ptr<ProjectPages> pages,
                                  const SelectedPage& selectedPage,
                                  const OutputFileNameGenerator& outFileNameGen) {
  m_projectFile = projectFile;
  m_stages = std::move(stages);
  m_pages = std::move(pages);
  m_selectedPage = selectedPage;
  m_outFileNameGen = outFileNameGen;

  ui.saveProjectBtn->setVisible(!projectFile.isEmpty());
}

void OutOfMemoryDialog::saveProject() {
  if (m_projectFile.isEmpty()) {
    saveProjectAs();
  } else if (saveProjectWithFeedback(m_projectFile)) {
    showSaveSuccessScreen();
  }
}

void OutOfMemoryDialog::saveProjectAs() {
  // XXX: this function is duplicated MainWindow

  QString projectDir;
  if (!m_projectFile.isEmpty()) {
    projectDir = QFileInfo(m_projectFile).absolutePath();
  } else {
    QSettings settings;
    projectDir = settings.value("project/lastDir").toString();
  }

  QString projectFile(
      QFileDialog::getSaveFileName(this, QString(), projectDir, tr("Scan Tailor Projects") + " (*.ScanTailor)"));
  if (projectFile.isEmpty()) {
    return;
  }

  if (!projectFile.endsWith(".ScanTailor", Qt::CaseInsensitive)) {
    projectFile += ".ScanTailor";
  }

  if (saveProjectWithFeedback(projectFile)) {
    m_projectFile = projectFile;
    showSaveSuccessScreen();

    QSettings settings;
    settings.setValue("project/lastDir", QFileInfo(m_projectFile).absolutePath());

    RecentProjects rp;
    rp.read();
    rp.setMostRecent(m_projectFile);
    rp.write();
  }
}  // OutOfMemoryDialog::saveProjectAs

bool OutOfMemoryDialog::saveProjectWithFeedback(const QString& projectFile) {
  ProjectWriter writer(m_pages, m_selectedPage, m_outFileNameGen);

  if (!writer.write(projectFile, m_stages->filters())) {
    QMessageBox::warning(this, tr("Error"), tr("Error saving the project file!"));

    return false;
  }

  return true;
}

void OutOfMemoryDialog::showSaveSuccessScreen() {
  ui.topLevelStack->setCurrentWidget(ui.saveSuccessPage);
}
