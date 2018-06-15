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
  intrusive_ptr<StageSequence> m_ptrStages;
  intrusive_ptr<ProjectPages> m_ptrPages;
  SelectedPage m_selectedPage;
  OutputFileNameGenerator m_outFileNameGen;
};


#endif  // ifndef OUT_OF_MEMORY_DIALOG_H_
