/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef FIXDPIDIALOG_H_
#define FIXDPIDIALOG_H_

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

  void decorateDpiInputField(QLineEdit* field, ImageMetadata::DpiStatus dpi_status) const;

  std::unique_ptr<TreeModel> m_ptrPages;
  std::unique_ptr<FilterModel> m_ptrUndefinedDpiPages;
  QString m_xDpiInitialValue;
  QString m_yDpiInitialValue;
  QSize m_selectedItemPixelSize;
  QPalette m_normalPalette;
  QPalette m_errorPalette;
};


#endif  // ifndef FIXDPIDIALOG_H_
