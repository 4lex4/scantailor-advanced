// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef RELINKABLE_PATH_VISUALIZATION_H_
#define RELINKABLE_PATH_VISUALIZATION_H_

#include <QString>
#include <QWidget>

class RelinkablePath;
class QHBoxLayout;
class QAbstractButton;

class RelinkablePathVisualization : public QWidget {
  Q_OBJECT
 public:
  explicit RelinkablePathVisualization(QWidget* parent = nullptr);

  void clear();

  void setPath(const RelinkablePath& path, bool clickable);

 signals:

  /** \p type is either RelinkablePath::File or RelinkablePath::Dir */
  void clicked(const QString& prefix_path, const QString& suffix_path, int type);

 protected:
  void paintEvent(QPaintEvent* evt) override;

 private:
  struct PathComponent;

  class ComponentButton;

  void onClicked(int component_idx, const QString& prefix_path, const QString& suffix_path, int type);

  void stylePathComponentButton(QAbstractButton* btn, bool exists);

  static void checkForExistence(std::vector<PathComponent>& components);

  QHBoxLayout* m_layout;
};


#endif  // ifndef RELINKABLE_PATH_VISUALIZATION_H_
