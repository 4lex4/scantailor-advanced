// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_FIXDPIDIALOG_H_
#define SCANTAILOR_APP_FIXDPIDIALOG_H_

#include <QDialog>
#include <QItemSelection>
#include <QPalette>
#include <QSize>
#include <QString>
#include <memory>
#include <vector>
#include "Dpi.h"
#include "ImageFileInfo.h"
#include "ImageMetadata.h"
#include "ui_FixDpiDialog.h"

class QItemSelection;

class FixDpiDialog : public QDialog, private Ui::FixDpiDialog {
  Q_OBJECT
 public:
  explicit FixDpiDialog(const std::vector<ImageFileInfo>& files, QWidget* parent = nullptr);

  ~FixDpiDialog() override;

  const std::vector<ImageFileInfo>& files() const;

 private slots:

  void tabChanged(int tab);

  void selectionChanged(const QItemSelection& selection);

  void dpiComboChangedByUser(int index);

  void dpiValueChanged();

  void applyClicked();

 private:
  class DpiCounts;
  class SizeGroup;
  class TreeModel;
  class FilterModel;

  enum Scope { ALL, NOT_OK };

  void enableDisableOkButton();

  void updateDpiFromSelection(const QItemSelection& selection);

  void resetDpiForm();

  void setDpiForm(const ImageMetadata& metadata);

  void updateDpiCombo();

  void decorateDpiInputField(QLineEdit* field, ImageMetadata::DpiStatus dpiStatus) const;

  std::unique_ptr<TreeModel> m_pages;
  std::unique_ptr<FilterModel> m_undefinedDpiPages;
  QString m_xDpiInitialValue;
  QString m_yDpiInitialValue;
  QSize m_selectedItemPixelSize;
  QPalette m_normalPalette;
  QPalette m_errorPalette;
};


#endif  // ifndef SCANTAILOR_APP_FIXDPIDIALOG_H_
