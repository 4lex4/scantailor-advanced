// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_APPLICATION_H_
#define SCANTAILOR_CORE_APPLICATION_H_

#include <QApplication>
#include <QTimer>
#include <QtCore/QTranslator>

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

  void loadFonts();

  QTranslator m_translator;
  QString m_currentLocale;
  std::map<QString, QString> m_translationsMap;
  QString m_portableConfigPath;
};


#endif
