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

#include "config.h"
#include "Application.h"
#include "MainWindow.h"
#include "PngMetadataLoader.h"
#include "TiffMetadataLoader.h"
#include "JpegMetadataLoader.h"
#include "DarkScheme.h"

#include "CommandLine.h"
#include "ColorSchemeManager.h"
#include "LightScheme.h"

int main(int argc, char** argv) {
    Application app(argc, argv);

#ifdef _WIN32
    // Get rid of all references to Qt's installation directory.
    app.setLibraryPaths(QStringList(app.applicationDirPath()));
#endif

    // parse command line arguments
    CommandLine cli(app.arguments());
    CommandLine::set(cli);

    if (cli.isError()) {
        cli.printHelp();

        return 1;
    }

    if (cli.hasHelp()) {
        cli.printHelp();

        return 0;
    }

    QString const translation("scantailor_" + QLocale::system().name());
    QTranslator translator;


    QStringList const translation_dirs(
            // Now try loading from where it's supposed to be.
            QString::fromUtf8(TRANSLATION_DIRS).split(QChar(':'), QString::SkipEmptyParts)
    );
    for (QString const& path : translation_dirs) {
        QString absolute_path;
        if (QDir::isAbsolutePath(path)) {
            absolute_path = path;
        } else {
            absolute_path = app.applicationDirPath();
            absolute_path += QChar('/');
            absolute_path += path;
        }
        absolute_path += QChar('/');
        absolute_path += translation;

        if (translator.load(absolute_path)) {
            break;
        }
    }

    app.installTranslator(&translator);

    // This information is used by QSettings.
    app.setApplicationName("scantailor");
    app.setOrganizationName("scantailor");

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::currentPath() + "/config");

    QSettings settings;

    PngMetadataLoader::registerMyself();
    TiffMetadataLoader::registerMyself();
    JpegMetadataLoader::registerMyself();

    {
        QString val = settings.value("settings/color_scheme", "dark").toString();
        std::unique_ptr<ColorScheme> scheme;
        if (val == "light") {
            scheme.reset(new LightScheme);
        } else {
            scheme.reset(new DarkScheme);
        }

        ColorSchemeManager::instance()->setColorScheme(*scheme.release());
    }

    MainWindow* main_wnd = new MainWindow();
    main_wnd->setAttribute(Qt::WA_DeleteOnClose);
    if (settings.value("mainWindow/maximized") == false) {
        main_wnd->show();
    } else {
#ifdef _WIN32
        main_wnd->show();
        main_wnd->showMaximized();
        main_wnd->showNormal();
#endif
        main_wnd->showMaximized();
    }

    if (!cli.projectFile().isEmpty()) {
        main_wnd->openProject(cli.projectFile());
    }

    return app.exec();
} // main

