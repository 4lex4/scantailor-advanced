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
#include <QtPlugin>
#include <QSettings>
#include <QTranslator>
#include <QDir>

#include "CommandLine.h"

int main(int argc, char** argv)
{

    Application app(argc, argv);

#ifdef _WIN32
    app.setLibraryPaths(QStringList(app.applicationDirPath()));
#endif

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

    QString translation("scantailor_" + QLocale::system().name());
    if (cli.hasLanguage()) {
        translation = "scantailor_" + cli.getLanguage();
    }

    QTranslator translator;

    if (!translator.load(translation)) {
        QString path(QString::fromUtf8(TRANSLATIONS_DIR_ABS));
        path += QChar('/');
        path += translation;
        if (!translator.load(path)) {
            path = QString::fromUtf8(TRANSLATIONS_DIR_REL);
            path += QChar('/');
            path += translation;
            translator.load(path);
        }
    }

    app.installTranslator(&translator);

    app.setApplicationName("scantailor");
    app.setOrganizationName("scantailor");

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QDir::currentPath() + "/config");

    QSettings settings;


    PngMetadataLoader::registerMyself();
    TiffMetadataLoader::registerMyself();
    JpegMetadataLoader::registerMyself();

    app.setFusionDarkTheme();

    MainWindow* main_wnd = new MainWindow();
    main_wnd->setAttribute(Qt::WA_DeleteOnClose);
    if (settings.value("mainWindow/maximized") == false) {
        main_wnd->show();
    }
    else {
        main_wnd->showMaximized();
    }

    if (!cli.projectFile().isEmpty()) {
        main_wnd->openProject(cli.projectFile());
    }

    return app.exec();
}
