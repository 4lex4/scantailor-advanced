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

#include <config.h>
#include <core/Application.h>
#include <core/ColorSchemeFactory.h>
#include <core/ColorSchemeManager.h>
#include <QSettings>
#include <QStringList>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
  // Rescaling for high DPI displays.
  QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

  Application app(argc, argv);

#ifdef _WIN32
  // Get rid of all references to Qt's installation directory.
  Application::setLibraryPaths(QStringList(Application::applicationDirPath()));
#endif

  QStringList args = Application::arguments();

  // This information is used by QSettings.
  Application::setApplicationName(APPLICATION_NAME);
  Application::setOrganizationName(ORGANIZATION_NAME);

  QSettings::setDefaultFormat(QSettings::IniFormat);
  if (app.isPortableVersion()) {
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, app.getPortableConfigPath());
  }
  QSettings settings;

  app.installLanguage(settings.value("settings/language", QLocale::system().name()).toString());

  {
    QString scheme_name = settings.value("settings/color_scheme", "dark").toString();
    std::unique_ptr<ColorScheme> scheme = ColorSchemeFactory().create(scheme_name);
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

  if (args.size() > 1) {
    main_wnd->openProject(args.at(1));
  }

  return Application::exec();
}  // main
