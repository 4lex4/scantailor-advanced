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

#include "ErrorWidget.h"
#include <QIcon>
#include <QStyle>

ErrorWidget::ErrorWidget(const QString& text, Qt::TextFormat fmt) {
  setupUi(this);
  textLabel->setTextFormat(fmt);
  textLabel->setText(text);
  QIcon icon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning));
  imageLabel->setPixmap(icon.pixmap(48, 48));

  connect(textLabel, SIGNAL(linkActivated(const QString&)), SLOT(linkActivated(const QString&)));
}
