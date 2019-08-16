// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
