// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_PICTUREZONEPROPDIALOG_H_
#define SCANTAILOR_OUTPUT_PICTUREZONEPROPDIALOG_H_

#include <QDialog>
#include "PropertySet.h"
#include "intrusive_ptr.h"
#include "ui_PictureZonePropDialog.h"

namespace output {
class PictureZonePropDialog : public QDialog {
  Q_OBJECT
 public:
  explicit PictureZonePropDialog(intrusive_ptr<PropertySet> props, QWidget* parent = nullptr);

 signals:
  void updated();

 private slots:
  void itemToggled(bool selected);

 private:
  Ui::PictureZonePropDialog ui;
  intrusive_ptr<PropertySet> m_props;
};
}  // namespace output
#endif
