// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_PROJECTOPENINGCONTEXT_H_
#define SCANTAILOR_APP_PROJECTOPENINGCONTEXT_H_

#include <QObject>
#include <QPointer>
#include <QString>
#include <Qt>
#include <vector>
#include "ImageFileInfo.h"
#include "NonCopyable.h"
#include "ProjectReader.h"

class FixDpiDialog;
class QWidget;
class QDomDocument;

class ProjectOpeningContext : public QObject {
  Q_OBJECT
  DECLARE_NON_COPYABLE(ProjectOpeningContext)

 public:
  ProjectOpeningContext(QWidget* parent, const QString& projectFile, const QDomDocument& doc);

  ~ProjectOpeningContext() override;

  void proceed();

  const QString& projectFile() const { return m_projectFile; }

  ProjectReader* projectReader() { return &m_reader; }

 signals:

  void done(ProjectOpeningContext* context);

 private slots:

  void fixedDpiSubmitted();

  void fixDpiDialogDestroyed();

 private:
  void showFixDpiDialog();

  QString m_projectFile;
  ProjectReader m_reader;
  QPointer<FixDpiDialog> m_fixDpiDialog;
  QWidget* m_parent;
};


#endif  // ifndef SCANTAILOR_APP_PROJECTOPENINGCONTEXT_H_
