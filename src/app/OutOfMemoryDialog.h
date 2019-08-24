// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_OUTOFMEMORYDIALOG_H_
#define SCANTAILOR_APP_OUTOFMEMORYDIALOG_H_

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

  void setParams(const QString& projectFile,
                 // may be empty
                 intrusive_ptr<StageSequence> stages,
                 intrusive_ptr<ProjectPages> pages,
                 const SelectedPage& selectedPage,
                 const OutputFileNameGenerator& outFileNameGen);

 private slots:

  void saveProject();

  void saveProjectAs();

 private:
  bool saveProjectWithFeedback(const QString& projectFile);

  void showSaveSuccessScreen();

  Ui::OutOfMemoryDialog ui;
  QString m_projectFile;
  intrusive_ptr<StageSequence> m_stages;
  intrusive_ptr<ProjectPages> m_pages;
  SelectedPage m_selectedPage;
  OutputFileNameGenerator m_outFileNameGen;
};


#endif  // ifndef SCANTAILOR_APP_OUTOFMEMORYDIALOG_H_
