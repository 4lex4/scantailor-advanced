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

#ifndef LOAD_FILES_STATUS_DIALOG_H_
#define LOAD_FILES_STATUS_DIALOG_H_

#include <QString>
#include <vector>
#include "ui_LoadFilesStatusDialog.h"

class LoadFilesStatusDialog : public QDialog {
 public:
  explicit LoadFilesStatusDialog(QWidget* parent = nullptr);

  void setLoadedFiles(const std::vector<QString>& files);

  void setFailedFiles(const std::vector<QString>& failed);

  void setOkButtonName(const QString& name);

 private:
  Ui::LoadFilesStatusDialog ui;
  QString m_loadedTabNameTemplate;
  QString m_failedTabNameTemplate;
};


#endif
