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

#include "RelinkingDialog.h"
#include <QDir>
#include <QFileDialog>
#include <cassert>
#include "RelinkingSortingModel.h"

RelinkingDialog::RelinkingDialog(const QString& project_file_path, QWidget* parent)
    : QDialog(parent),
      m_pSortingModel(new RelinkingSortingModel),
      m_projectFileDir(QFileInfo(project_file_path).path()) {
  ui.setupUi(this);
  m_pSortingModel->setSourceModel(&m_model);
  ui.listView->setModel(m_pSortingModel);
  ui.listView->setTextElideMode(Qt::ElideMiddle);
  ui.errorLabel->setVisible(false);
  ui.undoButton->setVisible(false);

  connect(ui.listView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
          SLOT(selectionChanged(const QItemSelection&, const QItemSelection&)));

  connect(ui.pathVisualization, SIGNAL(clicked(const QString&, const QString&, int)),
          SLOT(pathButtonClicked(const QString&, const QString&, int)));

  connect(ui.undoButton, SIGNAL(clicked()), SLOT(undoButtonClicked()));

  disconnect(ui.buttonBox, SIGNAL(accepted()));
  connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
}

void RelinkingDialog::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
  if (selected.isEmpty()) {
    ui.pathVisualization->clear();
    ui.pathVisualization->setVisible(false);
  } else {
    ui.undoButton->setVisible(false);

    const QModelIndex index(selected.front().topLeft());
    const QString path(index.data(m_model.UncommittedPathRole).toString());
    const int type = index.data(m_model.TypeRole).toInt();
    ui.pathVisualization->setPath(RelinkablePath(path, (RelinkablePath::Type) type), /*clickable=*/true);
    ui.pathVisualization->setVisible(true);

    if (ui.errorLabel->isVisible()) {
      m_model.rollbackChanges();
    } else {
      m_model.commitChanges();
    }
  }

  ui.errorLabel->setVisible(false);
}

void RelinkingDialog::pathButtonClicked(const QString& prefix_path, const QString& suffix_path, const int type) {
  assert(!prefix_path.endsWith(QChar('/')) && !prefix_path.endsWith(QChar('\\')));
  assert(!suffix_path.startsWith(QChar('/')) && !suffix_path.startsWith(QChar('\\')));

  QString replacement_path;

  if (type == RelinkablePath::File) {
    const QDir dir(QFileInfo(prefix_path).dir());
    replacement_path = QFileDialog::getOpenFileName(
        this, tr("Substitution File for %1").arg(QDir::toNativeSeparators(prefix_path)),
        dir.exists() ? dir.path() : m_projectFileDir, QString(), nullptr, QFileDialog::DontUseNativeDialog);
  } else {
    const QDir dir(prefix_path);
    replacement_path = QFileDialog::getExistingDirectory(
        this, tr("Substitution Directory for %1").arg(QDir::toNativeSeparators(prefix_path)),
        dir.exists() ? prefix_path : m_projectFileDir, QFileDialog::DontUseNativeDialog);
  }
  // So what's wrong with native dialogs? The one for directory selection won't show files
  // at all (if you ask it to, the non-native dialog will appear), which is inconvenient
  // in this situation. So, if one of them has to be non-native, the other was made
  // non-native as well, for consistency reasons.
  replacement_path = RelinkablePath::normalize(replacement_path);

  if (replacement_path.isEmpty()) {
    return;
  }

  if (prefix_path == replacement_path) {
    return;
  }

  QString new_path(replacement_path);
  new_path += QChar('/');
  new_path += suffix_path;

  m_model.replacePrefix(prefix_path, replacement_path, (RelinkablePath::Type) type);

  if (m_model.checkForMerges()) {
    ui.errorLabel->setText(tr("This change would merge several files into one."));
    ui.errorLabel->setVisible(true);
    ui.pathVisualization->clear();
    ui.pathVisualization->setVisible(false);
  } else {
    ui.pathVisualization->setPath(RelinkablePath(new_path, (RelinkablePath::Type) type), /*clickable=*/false);
    ui.pathVisualization->setVisible(true);
  }

  ui.undoButton->setVisible(true);
  ui.listView->update();
}  // RelinkingDialog::pathButtonClicked

void RelinkingDialog::undoButtonClicked() {
  m_model.rollbackChanges();  // Has to go before selectionChanged()
  selectionChanged(ui.listView->selectionModel()->selection(), QItemSelection());
}

void RelinkingDialog::commitChanges() {
  m_model.commitChanges();
  accept();
}
