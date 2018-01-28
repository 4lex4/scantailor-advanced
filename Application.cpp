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

#include "RelinkingDialog.h"
#include "LoadFilesStatusDialog.h"
#include "FixDpiDialog.h"
#include "ImageMetadataLoader.h"
#include "ProcessingIndicationWidget.h"
#include "ProjectOpeningContext.h"
#include "DebugImages.h"
#include "ErrorWidget.h"
#include "Utils.h"
#include "StageSequence.h"
#include "NewOpenProjectPanel.h"
#include "MainWindow.h"
#include "Application.h"
#include "OutOfMemoryHandler.h"
#include <config.h>
#include <QtCore/QDir>

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

void Application::installLanguage(const QString& locale) {
    if (m_currentLocale == locale) {
        return;
    }

    QString const translation("scantailor_" + locale);
    QStringList const translation_dirs(
            QString::fromUtf8(TRANSLATION_DIRS).split(QChar(':'), QString::SkipEmptyParts)
    );
    bool loaded = false;
    for (QString const& path : translation_dirs) {
        QString absolute_path;
        if (QDir::isAbsolutePath(path)) {
            absolute_path = path;
        } else {
            absolute_path = this->applicationDirPath();
            absolute_path += QChar('/');
            absolute_path += path;
        }
        absolute_path += QChar('/');
        absolute_path += translation;

        loaded = m_translator.load(absolute_path);
        if (loaded) {
            break;
        }
    }

    this->removeTranslator(&m_translator);
    this->installTranslator(&m_translator);

    m_currentLocale = (loaded) ? locale : "en";
}

const QString& Application::getCurrentLocale() const {
    return m_currentLocale;
}