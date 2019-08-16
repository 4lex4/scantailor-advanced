// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OUT_OF_MEMORY_DIALOG_H_
#define OUT_OF_MEMORY_DIALOG_H_

#include <QDialog>
#include <QString>
#include "OutputFileNameGenerator.h"
#include "ProjectPages.h"
#include "SelectedPage.h"
#include "StageSequence.h"
#include "intrusive_ptr.h"
#include "ui_OutOfMemoryDialog.h"

class OutOfMemoryDialog : public QDialog {
  Q_OBJECT
 public:
  explicit OutOfMemoryDialog(QWidget* parent = nullptr);

  void setParams(const QString& project_file,
                 // may be empty
                 intrusive_ptr<StageSequence> stages,
                 intrusive_ptr<ProjectPages> pages,
                 const SelectedPage& selected_page,
                 const OutputFileNameGenerator& out_file_name_gen);

 private slots:

  void saveProject();

  void saveProjectAs();

 private:
  bool saveProjectWithFeedback(const QString& project_file);

  void showSaveSuccessScreen();

  Ui::OutOfMemoryDialog ui;
  QString m_projectFile;
  intrusive_ptr<StageSequence> m_stages;
  intrusive_ptr<ProjectPages> m_pages;
  SelectedPage m_selectedPage;
  OutputFileNameGenerator m_outFileNameGen;
};


#endif  // ifndef OUT_OF_MEMORY_DIALOG_H_
