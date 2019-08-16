// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef ERRORWIDGET_H_
#define ERRORWIDGET_H_

#include <QWidget>
#include <Qt>
#include <memory>

class QString;
namespace Ui {
class ErrorWidget;
}

class ErrorWidget : public QWidget {
  Q_OBJECT
 public:
  explicit ErrorWidget(const QString& text, Qt::TextFormat fmt = Qt::AutoText);

  virtual ~ErrorWidget();

 private slots:

  /**
   * \see QLabel::linkActivated()
   */
  virtual void linkActivated(const QString& link) {}

 private:
  std::unique_ptr<Ui::ErrorWidget> ui;
};


#endif
