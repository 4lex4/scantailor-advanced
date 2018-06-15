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
#include "ColorSchemeManager.h"
#include "CommandLine.h"
#include "DarkScheme.h"
#include "JpegMetadataLoader.h"
#include "LightScheme.h"
#include "MainWindow.h"
#include "PngMetadataLoader.h"
#include "TiffMetadataLoader.h"
#include "config.h"

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
  app.setApplicationName(APPLICATION_NAME);
  app.setOrganizationName(ORGANIZATION_NAME);

  PngMetadataLoader::registerMyself();
  TiffMetadataLoader::registerMyself();
  JpegMetadataLoader::registerMyself();

  QSettings::setDefaultFormat(QSettings::IniFormat);
  if (app.isPortableVersion()) {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, app.getPortableConfigPath());
  }

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

    ColorSchemeManager::instance()->setColorScheme(*scheme);
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
