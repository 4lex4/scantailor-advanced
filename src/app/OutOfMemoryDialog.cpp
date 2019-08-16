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

void OutOfMemoryDialog::setParams(const QString& project_file,
                                  intrusive_ptr<StageSequence> stages,
                                  intrusive_ptr<ProjectPages> pages,
                                  const SelectedPage& selected_page,
                                  const OutputFileNameGenerator& out_file_name_gen) {
  m_projectFile = project_file;
  m_stages = std::move(stages);
  m_pages = std::move(pages);
  m_selectedPage = selected_page;
  m_outFileNameGen = out_file_name_gen;

  ui.saveProjectBtn->setVisible(!project_file.isEmpty());
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

  QString project_dir;
  if (!m_projectFile.isEmpty()) {
    project_dir = QFileInfo(m_projectFile).absolutePath();
  } else {
    QSettings settings;
    project_dir = settings.value("project/lastDir").toString();
  }

  QString project_file(
      QFileDialog::getSaveFileName(this, QString(), project_dir, tr("Scan Tailor Projects") + " (*.ScanTailor)"));
  if (project_file.isEmpty()) {
    return;
  }

  if (!project_file.endsWith(".ScanTailor", Qt::CaseInsensitive)) {
    project_file += ".ScanTailor";
  }

  if (saveProjectWithFeedback(project_file)) {
    m_projectFile = project_file;
    showSaveSuccessScreen();

    QSettings settings;
    settings.setValue("project/lastDir", QFileInfo(m_projectFile).absolutePath());

    RecentProjects rp;
    rp.read();
    rp.setMostRecent(m_projectFile);
    rp.write();
  }
}  // OutOfMemoryDialog::saveProjectAs

bool OutOfMemoryDialog::saveProjectWithFeedback(const QString& project_file) {
  ProjectWriter writer(m_pages, m_selectedPage, m_outFileNameGen);

  if (!writer.write(project_file, m_stages->filters())) {
    QMessageBox::warning(this, tr("Error"), tr("Error saving the project file!"));

    return false;
  }

  return true;
}

void OutOfMemoryDialog::showSaveSuccessScreen() {
  ui.topLevelStack->setCurrentWidget(ui.saveSuccessPage);
}
