// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_BINARIZATIONOPTIONSWIDGET_H
#define SCANTAILOR_BINARIZATIONOPTIONSWIDGET_H

#include <PageId.h>
#include <QWidget>

namespace output {
class BinarizationOptionsWidget : public QWidget {
  Q_OBJECT
 public:
  virtual void updateUi(const PageId& m_pageId) = 0;

 signals:

  /**
   * \brief To be emitted by subclasses when their state has changed.
   */
  void stateChanged();
};
}  // namespace output


#endif  // SCANTAILOR_BINARIZATIONOPTIONSWIDGET_H
