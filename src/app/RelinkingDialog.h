// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_RELINKINGDIALOG_H_
#define SCANTAILOR_APP_RELINKINGDIALOG_H_

#include <QDialog>

#include "AbstractRelinker.h"
#include "RelinkablePath.h"
#include "RelinkingModel.h"
#include "intrusive_ptr.h"
#include "ui_RelinkingDialog.h"

class RelinkingSortingModel;
class QAbstractButton;
class QItemSelection;
class QString;

class RelinkingDialog : public QDialog {
  Q_OBJECT
 public:
  explicit RelinkingDialog(const QString& projectFilePath, QWidget* parent = nullptr);

  ProxyFunction<RelinkingModel&, void, const RelinkablePath&> pathCollector() {
    return ProxyFunction<RelinkingModel&, void, const RelinkablePath&>(m_model);
  }

  /**
   * This method guarantees that
   * \code
   * dialog->relinker().release() == dialog->relinker().release()
   * \endcode
   * will hold true for the lifetime of the dialog.
   * This allows you to take the relinker right after construction
   * and then use it when accepted() signal is emitted.
   */
  intrusive_ptr<AbstractRelinker> relinker() const { return m_model.relinker(); }

 private slots:

  void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

  /** \p type is either RelinkablePath::File or RelinkablePath::Dir */
  void pathButtonClicked(const QString& prefixPath, const QString& suffixPath, int type);

  void undoButtonClicked();

  void commitChanges();

 private:
  Ui::RelinkingDialog ui;
  RelinkingModel m_model;
  RelinkingSortingModel* m_sortingModel;
  QString m_projectFileDir;
};


#endif  // ifndef SCANTAILOR_APP_RELINKINGDIALOG_H_
