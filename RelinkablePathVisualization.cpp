
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

#include "RelinkablePathVisualization.h"
#include "RelinkablePath.h"
#include "QtSignalForwarder.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QPaintEvent>
#include <QStyle>
#include <QStylePainter>
#include <QStyleOption>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

struct RelinkablePathVisualization::PathComponent {
    QString label;
    QString prefixPath;
    QString suffixPath;
    RelinkablePath::Type type;
    bool exists;

    PathComponent(QString const& lbl, QString const& prefix_path, QString const& suffix_path, RelinkablePath::Type t)
        : label(lbl),
          prefixPath(prefix_path),
          suffixPath(suffix_path),
          type(t),
          exists(false)
    { }
};


class RelinkablePathVisualization::ComponentButton
    : public QPushButton
{
public:
    ComponentButton(QWidget* parent = 0)
        : QPushButton(parent)
    { }

protected:
    virtual void paintEvent(QPaintEvent* evt);
};


RelinkablePathVisualization::RelinkablePathVisualization(QWidget* parent)
    : QWidget(parent),
      m_pLayout(new QHBoxLayout(this))
{
    m_pLayout->setSpacing(0);
    m_pLayout->setMargin(0);
}

void
RelinkablePathVisualization::clear()
{
    for (int count; (count = m_pLayout->count());) {
        QLayoutItem* item = m_pLayout->takeAt(count - 1);
        if (QWidget* wgt = item->widget()) {
            wgt->deleteLater();
        }
        delete item;
    }
}

void
RelinkablePathVisualization::setPath(RelinkablePath const& path, bool clickable)
{
    clear();

    QStringList components(path.normalizedPath().split(QChar('/'), QString::SkipEmptyParts));
    if (components.empty()) {
        return;
    }

    QString prefix_path;

    if (path.normalizedPath().startsWith(QLatin1String("//"))) {
        components.front().prepend(QLatin1String("//"));
    }
    else if (path.normalizedPath().startsWith(QChar('/'))) {
        prefix_path += QChar('/');
    }

    std::vector<PathComponent> path_components;
    path_components.reserve(components.size());

    for (QStringList::const_iterator it(components.begin()); it != components.end(); ++it) {
        QString const& component = *it;

        if (!prefix_path.isEmpty() && !prefix_path.endsWith('/')) {
            prefix_path += QChar('/');
        }
        prefix_path += component;

        QString suffix_path;
        for (QStringList::const_iterator it2(it); ++it2 != components.end();) {
            if (!suffix_path.isEmpty()) {
                suffix_path += QChar('/');
            }
            suffix_path += *it2;
        }

        path_components.push_back(PathComponent(component, prefix_path, suffix_path, RelinkablePath::Dir));
    }

    path_components.back().type = path.type();

    checkForExistence(path_components);

    int component_idx = -1;
    for (PathComponent& path_component : path_components) {
        ++component_idx;
        ComponentButton* btn = new ComponentButton(this);
        m_pLayout->addWidget(btn);
        btn->setText(path_component.label.replace(QChar('/'), QChar('\\')));
        btn->setEnabled(clickable);
        if (clickable) {
            btn->setCursor(QCursor(Qt::PointingHandCursor));
        }
        stylePathComponentButton(btn, path_component.exists);

        new QtSignalForwarder(
            btn, SIGNAL(clicked()), boost::bind(
                &RelinkablePathVisualization::onClicked, this,
                component_idx, path_component.prefixPath,
                path_component.suffixPath, path_component.type
            )
        );
    }

    m_pLayout->addStretch();
}  // RelinkablePathVisualization::setPath

void
RelinkablePathVisualization::stylePathComponentButton(QAbstractButton* btn, bool exists)
{
    QColor const border_color(palette().color(QPalette::Window).darker(150));

    QString style
        = "QAbstractButton {\n"
          "	border: 2px solid " + border_color.name() + ";\n"
          "	border-radius: 0.5em;\n"
          "	padding: 0.2em;\n"
          "	margin-left: 1px;\n"
          "	margin-right: 1px;\n"
          "	min-width: 2em;\n"
          "	font-weight: bold;\n";
    if (exists) {
        style
            += "	color: #3a5827;\n"
               "	background: qradialgradient(cx: 0.3, cy: -0.4, fx: 0.3, fy: -0.4, radius: 1.35, stop: 0 #fff, stop: 1 #89e74a);\n";
    }
    else {
        style
            += "	color: #6f2719;\n"
               "	background: qradialgradient(cx: 0.3, cy: -0.4, fx: 0.3, fy: -0.4, radius: 1.35, stop: 0 #fff, stop: 1 #ff674b);\n";
    }
    style
        += "}\n"
           "QAbstractButton:hover {\n"
           "	color: #333;\n"
           "	background: qradialgradient(cx: 0.3, cy: -0.4, fx: 0.3, fy: -0.4, radius: 1.35, stop: 0 #fff, stop: 1 #bbb);\n"
           "}\n"
           "QAbstractButton:pressed {\n"
           "	color: #333;\n"
           "	background: qradialgradient(cx: 0.4, cy: -0.1, fx: 0.4, fy: -0.1, radius: 1.35, stop: 0 #fff, stop: 1 #ddd);\n"
           "}\n";

    btn->setStyleSheet(style);
}  // RelinkablePathVisualization::stylePathComponentButton

void
RelinkablePathVisualization::paintEvent(QPaintEvent* evt)
{
    int const total_items = m_pLayout->count();
    for (int i = 0; i < total_items; ++i) {
        QWidget* widget = m_pLayout->itemAt(i)->widget();
        if (!widget) {
            continue;
        }

        QStyleOption option;
        option.initFrom(widget);

        if (option.state & QStyle::State_MouseOver) {
            widget->setProperty("highlightEnforcer", true);

            for (int j = 0; j < total_items; ++j) {
                widget = m_pLayout->itemAt(j)->widget();
                if (widget) {
                    bool const highlight = j <= i;
                    if (widget->property("forceHighlight").toBool() != highlight) {
                        widget->setProperty("forceHighlight", highlight);
                        widget->update();
                    }
                }
            }
            break;
        }
        else if (widget->property("highlightEnforcer").toBool()) {
            widget->setProperty("highlightEnforcer", false);

            for (int j = 0; j < total_items; ++j) {
                widget = m_pLayout->itemAt(j)->widget();
                if (widget) {
                    bool const highlight = false;
                    if (widget->property("forceHighlight").toBool() != highlight) {
                        widget->setProperty("forceHighlight", highlight);
                        widget->update();
                    }
                }
            }
            break;
        }
    }
}  // RelinkablePathVisualization::paintEvent

void
RelinkablePathVisualization::onClicked(int component_idx,
                                       QString const& prefix_path,
                                       QString const& suffix_path,
                                       int type)
{
    for (int i = 0; i <= component_idx; ++i) {
        QWidget* widget = m_pLayout->itemAt(i)->widget();
        if (widget) {
            widget->setProperty("stickHighlight", true);
        }
    }

    emit clicked(prefix_path, suffix_path, type);

    int const total_items = m_pLayout->count();
    for (int i = 0; i <= component_idx && i < total_items; ++i) {
        QWidget* widget = m_pLayout->itemAt(i)->widget();
        if (widget) {
            widget->setProperty("stickHighlight", false);
        }
    }
}

void
RelinkablePathVisualization::checkForExistence(std::vector<PathComponent>& components)
{
    if (components.empty()) {
        return;
    }

    if (QFile::exists(components.back().prefixPath)) {
        for (PathComponent& comp : components) {
            comp.exists = true;
        }

        return;
    }

    int left = -1;
    int right = components.size() - 1;
    while (right - left > 1) {
        int const mid = (left + right + 1) >> 1;
        if (QFile::exists(components[mid].prefixPath)) {
            left = mid;
        }
        else {
            right = mid;
        }
    }

    for (int i = components.size() - 1; i >= 0; --i) {
        components[i].exists = (i < right);
    }
}

/*============================ ComponentButton ============================*/

void
RelinkablePathVisualization::ComponentButton::paintEvent(QPaintEvent* evt)
{
    QStyleOptionButton option;
    option.initFrom(this);
    option.text = text();
    if (property("forceHighlight").toBool() || property("stickHighlight").toBool()) {
        option.state |= QStyle::State_MouseOver;
    }

    option.state |= QStyle::State_Enabled;

    QStylePainter painter(this);
    painter.drawControl(QStyle::CE_PushButton, option);
}

