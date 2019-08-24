// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RelinkingDialog.h"
#include <core/IconProvider.h>
#include <QDir>
#include <QFileDialog>
#include <cassert>
#include "RelinkingSortingModel.h"

RelinkingDialog::RelinkingDialog(const QString& projectFilePath, QWidget* parent)
    : QDialog(parent), m_sortingModel(new RelinkingSortingModel), m_projectFileDir(QFileInfo(projectFilePath).path()) {
  ui.setupUi(this);
  ui.undoButton->setIcon(IconProvider::getInstance().getIcon("undo"));
  m_sortingModel->setSourceModel(&m_model);
  ui.listView->setModel(m_sortingModel);
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

void RelinkingDialog::pathButtonClicked(const QString& prefixPath, const QString& suffixPath, const int type) {
  assert(!prefixPath.endsWith(QChar('/')) && !prefixPath.endsWith(QChar('\\')));
  assert(!suffixPath.startsWith(QChar('/')) && !suffixPath.startsWith(QChar('\\')));

  QString replacementPath;

  if (type == RelinkablePath::File) {
    const QDir dir(QFileInfo(prefixPath).dir());
    replacementPath = QFileDialog::getOpenFileName(
        this, tr("Substitution File for %1").arg(QDir::toNativeSeparators(prefixPath)),
        dir.exists() ? dir.path() : m_projectFileDir, QString(), nullptr, QFileDialog::DontUseNativeDialog);
  } else {
    const QDir dir(prefixPath);
    replacementPath = QFileDialog::getExistingDirectory(
        this, tr("Substitution Directory for %1").arg(QDir::toNativeSeparators(prefixPath)),
        dir.exists() ? prefixPath : m_projectFileDir, QFileDialog::DontUseNativeDialog);
  }
  // So what's wrong with native dialogs? The one for directory selection won't show files
  // at all (if you ask it to, the non-native dialog will appear), which is inconvenient
  // in this situation. So, if one of them has to be non-native, the other was made
  // non-native as well, for consistency reasons.
  replacementPath = RelinkablePath::normalize(replacementPath);

  if (replacementPath.isEmpty()) {
    return;
  }

  if (prefixPath == replacementPath) {
    return;
  }

  QString newPath(replacementPath);
  newPath += QChar('/');
  newPath += suffixPath;

  m_model.replacePrefix(prefixPath, replacementPath, (RelinkablePath::Type) type);

  if (m_model.checkForMerges()) {
    ui.errorLabel->setText(tr("This change would merge several files into one."));
    ui.errorLabel->setVisible(true);
    ui.pathVisualization->clear();
    ui.pathVisualization->setVisible(false);
  } else {
    ui.pathVisualization->setPath(RelinkablePath(newPath, (RelinkablePath::Type) type), /*clickable=*/false);
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