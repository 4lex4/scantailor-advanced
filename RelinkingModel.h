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

#ifndef RELINKING_MODEL_H_
#define RELINKING_MODEL_H_

#include <QAbstractListModel>
#include <QModelIndex>
#include <QPixmap>
#include <QString>
#include <QThread>
#include <Qt>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>
#include "AbstractRelinker.h"
#include "Hashes.h"
#include "NonCopyable.h"
#include "RelinkablePath.h"
#include "VirtualFunction.h"
#include "intrusive_ptr.h"

class RelinkingModel : public QAbstractListModel {
  DECLARE_NON_COPYABLE(RelinkingModel)

 public:
  enum Status { Exists, Missing, StatusUpdatePending };

  enum { TypeRole = Qt::UserRole, UncommittedPathRole, UncommittedStatusRole };

  RelinkingModel();

  ~RelinkingModel() override;

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  /**
   * This method guarantees that
   * \code
   * model.relinker().release() == model.relinker().release()
   * \endcode
   * will hold true for the lifetime of RelinkingModel object.
   * This allows you to take the relinker right after construction
   * and then use it when accepted() signal is emitted.
   */
  intrusive_ptr<AbstractRelinker> relinker() const { return m_ptrRelinker; }

  void operator()(const RelinkablePath& path) { addPath(path); }

  void addPath(const RelinkablePath& path);

  void replacePrefix(const QString& prefix, const QString& replacement, RelinkablePath::Type type);

  /**
   * Returns true if we have different original paths remapped to the same one.
   * Checking is done on uncommitted paths.
   */
  bool checkForMerges() const;

  void commitChanges();

  void rollbackChanges();

  void requestStatusUpdate(const QModelIndex& index);

 protected:
  void customEvent(QEvent* event) override;

 private:
  class StatusUpdateThread;
  class StatusUpdateResponse;

  /** Stands for File System Object (file or directory). */
  struct Item {
    QString origPath;

    /**< That's the path passed through addPath(). It never changes. */
    QString committedPath;
    QString uncommittedPath;

    /**< Same as committedPath when m_haveUncommittedChanges == false. */
    RelinkablePath::Type type;
    Status committedStatus;
    Status uncommittedStatus;

    /**< Same as committedStatus when m_haveUncommittedChanges == false. */

    explicit Item(const RelinkablePath& path);
  };

  class Relinker : public AbstractRelinker {
   public:
    void addMapping(const QString& from, const QString& to);

    /** Returns path.normalizedPath() if there is no mapping for it. */
    QString substitutionPathFor(const RelinkablePath& path) const override;

    void swap(Relinker& other);

   private:
    std::unordered_map<QString, QString, hashes::hash<QString>> m_mappings;
  };


  static void ensureEndsWithSlash(QString& str);

  QPixmap m_fileIcon;
  QPixmap m_folderIcon;
  std::vector<Item> m_items;
  std::set<QString> m_origPathSet;
  const intrusive_ptr<Relinker> m_ptrRelinker;
  std::unique_ptr<StatusUpdateThread> m_ptrStatusUpdateThread;
  bool m_haveUncommittedChanges;
};


#endif  // ifndef RELINKING_MODEL_H_
