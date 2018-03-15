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
    // rescaling for high DPI displays
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    Application app(argc, argv);

#ifdef _WIN32
    // Get rid of all references to Qt's installation directory.
    app.setLibraryPaths(QStringList(app.applicationDirPath()));
#endif

    // Initialize command line in gui mode.
    CommandLine cli(app.arguments());
    CommandLine::set(cli);

    // This information is used by QSettings.
    app.setApplicationName("scantailor");
    app.setOrganizationName("scantailor");

    PngMetadataLoader::registerMyself();
    TiffMetadataLoader::registerMyself();
    JpegMetadataLoader::registerMyself();

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, app.applicationDirPath() + "/config");

    QSettings settings;

    app.installLanguage(settings.value("settings/language", QLocale::system().name()).toString());

    {
        QString val = settings.value("settings/color_scheme", "dark").toString();
        std::unique_ptr<ColorScheme> scheme;
        if (val == "light") {
            scheme = std::make_unique<LightScheme>();
        } else {
            scheme = std::make_unique<DarkScheme>();
        }

        ColorSchemeManager::instance()->setColorScheme(*scheme.release());
    }

    auto* main_wnd = new MainWindow();
    main_wnd->setAttribute(Qt::WA_DeleteOnClose);
    if (settings.value("mainWindow/maximized") == false) {
        main_wnd->show();
    } else {
        // main_wnd->showMaximized();  // Doesn't work for Windows.
        QTimer::singleShot(0, main_wnd, &QMainWindow::showMaximized);
    }

    if (!cli.projectFile().isEmpty()) {
        main_wnd->openProject(cli.projectFile());
    }

    return app.exec();
}  // main
