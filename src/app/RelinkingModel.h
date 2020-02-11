// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_RELINKINGMODEL_H_
#define SCANTAILOR_APP_RELINKINGMODEL_H_

#include <QAbstractListModel>
#include <QIcon>
#include <QModelIndex>
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
  std::shared_ptr<AbstractRelinker> relinker() const { return m_relinker; }

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
  const std::shared_ptr<Relinker> m_relinker;
  std::unique_ptr<StatusUpdateThread> m_statusUpdateThread;
  bool m_haveUncommittedChanges;
};


#endif  // ifndef SCANTAILOR_APP_RELINKINGMODEL_H_
