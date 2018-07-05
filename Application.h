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

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <QApplication>
#include <QTimer>
#include <QtCore/QTranslator>
#include "BackgroundTask.h"
#include "FilterUiInterface.h"
#include "OutputFileNameGenerator.h"

class Application : public QApplication {
  Q_OBJECT
 public:
  Application(int& argc, char** argv);

  bool notify(QObject* receiver, QEvent* e) override;

  const QString& getCurrentLocale() const;

  void installLanguage(const QString& locale);

  std::list<QString> getLanguagesList() const;

  bool isPortableVersion() const;

  const QString& getPortableConfigPath() const;

 private:
  void initTranslations();

  void initPortableVersion();


  QTranslator m_translator;
  QString m_currentLocale;
  std::map<QString, QString> m_translationsMap;
  QString m_portableConfigPath;
};


#endif
