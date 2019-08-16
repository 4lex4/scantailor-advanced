// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef NEW_OPEN_PROJECT_PANEL_H_
#define NEW_OPEN_PROJECT_PANEL_H_

#include <QWidget>
#include <memory>
#include "ui_NewOpenProjectPanel.h"

class QString;

class NewOpenProjectPanel : public QWidget, private Ui::NewOpenProjectPanel {
  Q_OBJECT
 public:
  explicit NewOpenProjectPanel(QWidget* parent = nullptr);

 signals:

  void newProject();

  void openProject();

  void openRecentProject(const QString& project_file);

 protected:
  void paintEvent(QPaintEvent*) override;

 private:
  void addRecentProject(const QString& file_path);
};


#endif  // ifndef NEW_OPEN_PROJECT_PANEL_H_
