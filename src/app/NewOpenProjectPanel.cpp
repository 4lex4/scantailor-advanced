// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "NewOpenProjectPanel.h"

#include <QFileInfo>
#include <QPainter>

#include "ColorSchemeManager.h"
#include "RecentProjects.h"
#include "Utils.h"

using namespace core;

NewOpenProjectPanel::NewOpenProjectPanel(QWidget* parent) : QWidget(parent) {
  setupUi(this);

  recentProjectsGroup->setLayout(new QVBoxLayout);
  newProjectLabel->setText(Utils::richTextForLink(newProjectLabel->text()));
  openProjectLabel->setText(Utils::richTextForLink(openProjectLabel->text()));

  RecentProjects rp;
  rp.read();
  if (!rp.validate()) {
    // Some project files weren't found.
    // Write the list without them.
    rp.write();
  }
  if (rp.isEmpty()) {
    recentProjectsGroup->setVisible(false);
  } else {
    rp.enumerate([this](const QString& filePath) { addRecentProject(filePath); });
  }

  connect(newProjectLabel, SIGNAL(linkActivated(const QString&)), this, SIGNAL(newProject()));
  connect(openProjectLabel, SIGNAL(linkActivated(const QString&)), this, SIGNAL(openProject()));
}

void NewOpenProjectPanel::addRecentProject(const QString& filePath) {
  const QFileInfo fileInfo(filePath);
  QString baseName(fileInfo.completeBaseName());
  if (baseName.isEmpty()) {
    baseName = QChar('_');
  }
  auto* label = new QLabel(recentProjectsGroup);
  label->setWordWrap(true);
  label->setTextFormat(Qt::RichText);
  label->setText(Utils::richTextForLink(baseName, filePath));
  label->setToolTip(filePath);

  int fontSize = recentProjectsGroup->font().pointSize();
  QFont widgetFont = label->font();
  widgetFont.setPointSize(fontSize);
  label->setFont(widgetFont);

  recentProjectsGroup->layout()->addWidget(label);

  connect(label, SIGNAL(linkActivated(const QString&)), this, SIGNAL(openRecentProject(const QString&)));
}

void NewOpenProjectPanel::paintEvent(QPaintEvent*) {
  // In fact Qt doesn't draw QWidget's background, unless
  // autoFillBackground property is set, so we can safely
  // draw our borders and shadows in the margins area.

  int left = 0, top = 0, right = 0, bottom = 0;
  layout()->getContentsMargins(&left, &top, &right, &bottom);

  const QRect widgetRect(rect());
  const QRect exceptMargins(widgetRect.adjusted(left, top, -right, -bottom));

  const int border = 1;  // Solid line border width.

  QPainter painter(this);

  const QBrush border_brush
      = ColorSchemeManager::instance().getColorParam(ColorScheme::OpenNewProjectBorder, palette().windowText());
  painter.setPen(QPen(border_brush, border));

  painter.drawRect(exceptMargins);
}  // NewOpenProjectPanel::paintEvent
