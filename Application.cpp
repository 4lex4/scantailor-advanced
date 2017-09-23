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

#include "Application.h"
#include "OutOfMemoryHandler.h"
#include <QStyleFactory>
#include <QPalette>

Application::Application(int& argc, char** argv)
        : QApplication(argc, argv) {
}

bool Application::notify(QObject* receiver, QEvent* e) {
    try {
        return QApplication::notify(receiver, e);
    } catch (std::bad_alloc const&) {
        OutOfMemoryHandler::instance().handleOutOfMemorySituation();

        return false;
    }
}

void Application::setFusionDarkTheme() {
    QApplication::setStyle(QStyleFactory::create("fusion"));

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0x53, 0x53, 0x53));
    palette.setColor(QPalette::WindowText, QColor(0xe1, 0xe1, 0xe1));
    palette.setColor(QPalette::Base, QColor(0x53, 0x53, 0x53));
    palette.setColor(QPalette::AlternateBase, QColor(0x53, 0x53, 0x53));
    palette.setColor(QPalette::ToolTipBase, QColor(0x53, 0x53, 0x53));
    palette.setColor(QPalette::ToolTipText, QColor(0xe1, 0xe1, 0xe1));
    palette.setColor(QPalette::Text, QColor(0xe1, 0xe1, 0xe1));
    palette.setColor(QPalette::Button, QColor(0x53, 0x53, 0x53));
    palette.setColor(QPalette::ButtonText, QColor(0xe1, 0xe1, 0xe1));
    palette.setColor(QPalette::Link, QColor(0x58, 0xad, 0xf6));

    palette.setColor(QPalette::BrightText, QColor(0xe1, 0x00, 0x00));
    palette.setColor(QPalette::Highlight, QColor(0x42, 0x42, 0x42));
    palette.setColor(QPalette::HighlightedText, QColor(0xee, 0xee, 0xee));

    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(0xb1, 0xb1, 0xb1));
    palette.setColor(QPalette::Disabled, QPalette::Base, QColor(0x73, 0x73, 0x73));
    palette.setColor(QPalette::Disabled, QPalette::AlternateBase, QColor(0x73, 0x73, 0x73));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0xb1, 0xb1, 0xb1));
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor(0x73, 0x73, 0x73));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0xb1, 0xb1, 0xb1));

    QApplication::setPalette(palette);
}

