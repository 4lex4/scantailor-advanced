// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef RECENT_PROJECTS_H_
#define RECENT_PROJECTS_H_

#include <QString>
#include <limits>
#include <list>

class RecentProjects {
 public:
  /**
   * \brief The default value for max_items parameters of
   *        write() and enumerate().
   */
  enum { DEFAULT_MAX_ITEMS = 7 };

  /**
   * \brief Reads the list of recent projects from QSettings
   *        without validating them.
   *
   * The current list will be overwritten.
   */
  void read();

  /**
   * \brief Removes non-existing project files.
   *
   * \return true if no projects were removed, false otherwise.
   */
  bool validate();

  /**
   * \brief Appends a project to the list or moves it to the
   *        top of the list, if it was already there.
   */
  void setMostRecent(const QString& file_path);

  void write(int max_items = DEFAULT_MAX_ITEMS) const;

  bool isEmpty() const { return m_projectFiles.empty(); }

  /**
   * \brief Calls out((const QString&)file_path) for every entry.
   *
   * Modifying this object from the callback is not allowed.
   */
  template <typename Out>
  void enumerate(Out out, int max_items = DEFAULT_MAX_ITEMS) const;

 private:
  std::list<QString> m_projectFiles;
};


template <typename Out>
void RecentProjects::enumerate(Out out, int max_items) const {
  std::list<QString>::const_iterator it(m_projectFiles.begin());
  const std::list<QString>::const_iterator end(m_projectFiles.end());
  for (; it != end && max_items > 0; ++it, --max_items) {
    out(*it);
  }
}

#endif  // ifndef RECENT_PROJECTS_H_
