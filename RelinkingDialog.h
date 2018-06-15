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

#ifndef RELINKING_DIALOG_H_
#define RELINKING_DIALOG_H_

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
  explicit RelinkingDialog(const QString& project_file_path, QWidget* parent = nullptr);

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
  void pathButtonClicked(const QString& prefix_path, const QString& suffix_path, int type);

  void undoButtonClicked();

  void commitChanges();

 private:
  Ui::RelinkingDialog ui;
  RelinkingModel m_model;
  RelinkingSortingModel* m_pSortingModel;
  QString m_projectFileDir;
};


#endif  // ifndef RELINKING_DIALOG_H_
