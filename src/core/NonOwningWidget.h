// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef NON_OWNING_WIDGET_H_
#define NON_OWNING_WIDGET_H_

#include <QWidget>

/**
 * \brief Your normal QWidget, except it doesn't delete its children with itself,
 *        rather it calls setParent(0) on them.
 */
class NonOwningWidget : public QWidget {
 public:
  explicit NonOwningWidget(QWidget* parent = nullptr);

  ~NonOwningWidget() override;
};


#endif
