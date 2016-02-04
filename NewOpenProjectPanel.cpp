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

#include "NewOpenProjectPanel.h"
#include "RecentProjects.h"
#include "Utils.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <QPainter>
#include <QSettings>
#include <QFileInfo>

NewOpenProjectPanel::NewOpenProjectPanel(QWidget* parent)
        : QWidget(parent)
{
    setupUi(this);

    recentProjectsGroup->setLayout(new QVBoxLayout);
    newProjectLabel->setText(
            Utils::richTextForLink(newProjectLabel->text())
    );
    openProjectLabel->setText(
            Utils::richTextForLink(openProjectLabel->text())
    );

    RecentProjects rp;
    rp.read();
    if (!rp.validate()) {
        rp.write();
    }
    if (rp.isEmpty()) {
        recentProjectsGroup->setVisible(false);
    }
    else {
        rp.enumerate(
                boost::lambda::bind(
                        &NewOpenProjectPanel::addRecentProject,
                        this, boost::lambda::_1
                )
        );
    }

    connect(
            newProjectLabel, SIGNAL(linkActivated(QString const&)),
            this, SIGNAL(newProject())
    );
    connect(
            openProjectLabel, SIGNAL(linkActivated(QString const&)),
            this, SIGNAL(openProject())
    );
}

void
NewOpenProjectPanel::addRecentProject(QString const& file_path)
{
    QFileInfo const file_info(file_path);
    QString base_name(file_info.baseName());
    if (base_name.isEmpty()) {
        base_name = QChar('_');
    }
    QLabel* label = new QLabel(recentProjectsGroup);
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    label->setText(Utils::richTextForLink(base_name, file_path));
    label->setToolTip(file_path);
    recentProjectsGroup->layout()->addWidget(label);

    connect(
            label, SIGNAL(linkActivated(QString const&)),
            this, SIGNAL(openRecentProject(QString const&))
    );
}

void
NewOpenProjectPanel::paintEvent(QPaintEvent* event)
{

    int left = 0, top = 0, right = 0, bottom = 0;
    layout()->getContentsMargins(&left, &top, &right, &bottom);

    QRect const widget_rect(rect());
    QRect const except_margins(
            widget_rect.adjusted(left, top, -right, -bottom)
    );

    int const border = 1;
    int const shadow = std::min(right, bottom) - border;

    QPainter painter(this);

    painter.setPen(QPen(palette().windowText(), border));

    painter.drawRect(except_margins);

    QColor const dark(Qt::darkGray);
    QColor const light(Qt::transparent);

    if (shadow <= 0) {
        return;
    }

    left -= border;
    right -= border;
    top -= border;
    bottom -= border;

    QRect const extended(
            except_margins.adjusted(
                    -border - 1, -border - 1, border + 1, border + 1
            )
    );

    {
        QRect rect(widget_rect);
        rect.setWidth(shadow);
        rect.moveLeft(extended.right());
        rect.adjust(0, top + shadow, 0, -bottom);
        QLinearGradient grad(rect.topLeft(), rect.topRight());
        grad.setColorAt(0, dark);
        grad.setColorAt(1, light);
        painter.fillRect(rect, grad);
    }

    {
        QRect rect(widget_rect);
        rect.setHeight(shadow);
        rect.moveTop(extended.bottom());
        rect.adjust(left + shadow, 0, -right, 0);
        QLinearGradient grad(rect.topLeft(), rect.bottomLeft());
        grad.setColorAt(0, dark);
        grad.setColorAt(1, light);
        painter.fillRect(rect, grad);
    }

    {
        QRect rect(0, 0, shadow, shadow);
        rect.moveTopLeft(extended.bottomRight());
        QRadialGradient grad(rect.topLeft(), shadow);
        grad.setColorAt(0, dark);
        grad.setColorAt(1, light);
        painter.fillRect(rect, grad);
    }

    {
        QRect rect(0, 0, shadow, shadow);
        rect.moveTopLeft(extended.topRight() + QPoint(0, border));
        QRadialGradient grad(rect.bottomLeft(), shadow);
        grad.setColorAt(0, dark);
        grad.setColorAt(1, light);
        painter.fillRect(rect, grad);
    }

    {
        QRect rect(0, 0, shadow, shadow);
        rect.moveTopLeft(extended.bottomLeft() + QPoint(border, 0));
        QRadialGradient grad(rect.topRight(), shadow);
        grad.setColorAt(0, dark);
        grad.setColorAt(1, light);
        painter.fillRect(rect, grad);
    }
}
