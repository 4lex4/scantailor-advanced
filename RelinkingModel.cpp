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

#include "RelinkingModel.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QWaitCondition>
#include <boost/foreach.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include "OutOfMemoryHandler.h"
#include "PayloadEvent.h"

class RelinkingModel::StatusUpdateResponse {
 public:
  StatusUpdateResponse(const QString& path, int row, Status status) : m_path(path), m_row(row), m_status(status) {}

  const QString& path() const { return m_path; }

  int row() const { return m_row; }

  Status status() const { return m_status; }

 private:
  QString m_path;
  int m_row;
  Status m_status;
};


class RelinkingModel::StatusUpdateThread : private QThread {
 public:
  explicit StatusUpdateThread(RelinkingModel* owner);

  /** This will signal the thread to stop and wait for it to happen. */
  ~StatusUpdateThread() override;

  /**
   * Requests are served from last to first.
   * Requesting the same item multiple times will just move the existing
   * record to the top of the stack.
   */
  void requestStatusUpdate(const QString& path, int row);

 private:
  struct Task {
    QString path;
    int row;

    Task(const QString& p, int r) : path(p), row(r) {}
  };

  class OrderedByPathTag;
  class OrderedByPriorityTag;

  typedef boost::multi_index_container<
      Task,
      boost::multi_index::indexed_by<
          boost::multi_index::ordered_unique<boost::multi_index::tag<OrderedByPathTag>,
                                             boost::multi_index::member<Task, QString, &Task::path>>,
          boost::multi_index::sequenced<boost::multi_index::tag<OrderedByPriorityTag>>>>
      TaskList;

  typedef TaskList::index<OrderedByPathTag>::type TasksByPath;
  typedef TaskList::index<OrderedByPriorityTag>::type TasksByPriority;

  void run() override;

  RelinkingModel* m_pOwner;
  TaskList m_tasks;
  TasksByPath& m_rTasksByPath;
  TasksByPriority& m_rTasksByPriority;
  QString m_pathBeingProcessed;
  QMutex m_mutex;
  QWaitCondition m_cond;
  bool m_exiting;
};


/*============================ RelinkingModel =============================*/

RelinkingModel::RelinkingModel()
    : m_fileIcon(":/icons/file-16.png"),
      m_folderIcon(":/icons/folder-16.png"),
      m_ptrRelinker(new Relinker),
      m_ptrStatusUpdateThread(new StatusUpdateThread(this)),
      m_haveUncommittedChanges(true) {}

RelinkingModel::~RelinkingModel() = default;

int RelinkingModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return static_cast<int>(m_items.size());
  } else {
    return 0;
  }
}

QVariant RelinkingModel::data(const QModelIndex& index, int role) const {
  const Item& item = m_items[index.row()];

  switch (role) {
    case TypeRole:
      return item.type;
    case UncommittedStatusRole:
      return item.uncommittedStatus;
    case UncommittedPathRole:
      return item.uncommittedPath;
    case Qt::DisplayRole:
      if (item.uncommittedPath.startsWith(QChar('/')) && !item.uncommittedPath.startsWith(QLatin1String("//"))) {
        // "//" indicates a network path
        return item.uncommittedPath;
      } else {
        return QDir::toNativeSeparators(item.uncommittedPath);
      }
    case Qt::DecorationRole:
      return (item.type == RelinkablePath::Dir) ? m_folderIcon : m_fileIcon;
    case Qt::BackgroundColorRole:
      return QColor(Qt::transparent);
    default:
      break;
  }

  return QVariant();
}

void RelinkingModel::addPath(const RelinkablePath& path) {
  const QString& normalized_path(path.normalizedPath());

  const std::pair<std::set<QString>::iterator, bool> ins(m_origPathSet.insert(path.normalizedPath()));
  if (!ins.second) {
    // Not inserted because identical path is already there.
    return;
  }

  beginInsertRows(QModelIndex(), static_cast<int>(m_items.size()), static_cast<int>(m_items.size()));
  m_items.emplace_back(path);
  endInsertRows();

  requestStatusUpdate(index(static_cast<int>(m_items.size() - 1)));
}

void RelinkingModel::replacePrefix(const QString& prefix, const QString& replacement, RelinkablePath::Type type) {
  QString slash_terminated_prefix(prefix);
  ensureEndsWithSlash(slash_terminated_prefix);

  int modified_rowspan_begin = -1;

  int row = -1;
  for (Item& item : m_items) {
    ++row;
    bool modified = false;

    if (type == RelinkablePath::File) {
      if ((item.type == RelinkablePath::File) && (item.uncommittedPath == prefix)) {
        item.uncommittedPath = replacement;
        modified = true;
      }
    } else {
      assert(type == RelinkablePath::Dir);
      if (item.uncommittedPath.startsWith(slash_terminated_prefix)) {
        const int suffix_len = item.uncommittedPath.length() - slash_terminated_prefix.length() + 1;
        item.uncommittedPath = replacement + item.uncommittedPath.right(suffix_len);
        modified = true;
      } else if (item.uncommittedPath == prefix) {
        item.uncommittedPath = replacement;
        modified = true;
      }
    }

    if (modified) {
      m_haveUncommittedChanges = true;
      if (modified_rowspan_begin == -1) {
        modified_rowspan_begin = row;
      }
      emit dataChanged(index(modified_rowspan_begin), index(row));
      requestStatusUpdate(index(row));  // This sets item.changedStatus to StatusUpdatePending.
    } else {
      if (modified_rowspan_begin != -1) {
        emit dataChanged(index(modified_rowspan_begin), index(row));
        modified_rowspan_begin = -1;
      }
    }
  }

  if (modified_rowspan_begin != -1) {
    emit dataChanged(index(modified_rowspan_begin), index(row));
  }
}  // RelinkingModel::replacePrefix

bool RelinkingModel::checkForMerges() const {
  std::vector<QString> new_paths;
  new_paths.reserve(m_items.size());

  for (const Item& item : m_items) {
    new_paths.push_back(item.uncommittedPath);
  }

  std::sort(new_paths.begin(), new_paths.end());

  return std::adjacent_find(new_paths.begin(), new_paths.end()) != new_paths.end();
}

void RelinkingModel::commitChanges() {
  if (!m_haveUncommittedChanges) {
    return;
  }

  Relinker new_relinker;
  int modified_rowspan_begin = -1;

  int row = -1;
  for (Item& item : m_items) {
    ++row;

    if (item.committedPath != item.uncommittedPath) {
      item.committedPath = item.uncommittedPath;
      item.committedStatus = item.uncommittedStatus;
      new_relinker.addMapping(item.origPath, item.committedPath);
      if (modified_rowspan_begin == -1) {
        modified_rowspan_begin = row;
      }
    } else {
      if (modified_rowspan_begin != -1) {
        emit dataChanged(index(modified_rowspan_begin), index(row));
        modified_rowspan_begin = -1;
      }
    }
  }

  if (modified_rowspan_begin != -1) {
    emit dataChanged(index(modified_rowspan_begin), index(row));
  }

  m_ptrRelinker->swap(new_relinker);
  m_haveUncommittedChanges = false;
}  // RelinkingModel::commitChanges

void RelinkingModel::rollbackChanges() {
  if (!m_haveUncommittedChanges) {
    return;
  }

  int modified_rowspan_begin = -1;

  int row = -1;
  for (Item& item : m_items) {
    ++row;

    if (item.uncommittedPath != item.committedPath) {
      item.uncommittedPath = item.committedPath;
      item.uncommittedStatus = item.committedStatus;
      if (modified_rowspan_begin == -1) {
        modified_rowspan_begin = row;
      }
    } else {
      if (modified_rowspan_begin != -1) {
        emit dataChanged(index(modified_rowspan_begin), index(row));
        modified_rowspan_begin = -1;
      }
    }
  }

  if (modified_rowspan_begin != -1) {
    emit dataChanged(index(modified_rowspan_begin), index(row));
  }

  m_haveUncommittedChanges = false;
}  // RelinkingModel::rollbackChanges

void RelinkingModel::ensureEndsWithSlash(QString& str) {
  if (!str.endsWith(QChar('/'))) {
    str += QChar('/');
  }
}

void RelinkingModel::requestStatusUpdate(const QModelIndex& index) {
  assert(index.isValid());

  Item& item = m_items[index.row()];
  item.uncommittedStatus = StatusUpdatePending;

  m_ptrStatusUpdateThread->requestStatusUpdate(item.uncommittedPath, index.row());
}

void RelinkingModel::customEvent(QEvent* event) {
  typedef PayloadEvent<StatusUpdateResponse> ResponseEvent;
  auto* evt = dynamic_cast<ResponseEvent*>(event);
  assert(evt);

  const StatusUpdateResponse& response = evt->payload();
  if ((response.row() < 0) || (response.row() >= int(m_items.size()))) {
    return;
  }

  Item& item = m_items[response.row()];
  if (item.uncommittedPath == response.path()) {
    item.uncommittedStatus = response.status();
  }
  if (item.committedPath == response.path()) {
    item.committedStatus = response.status();
  }

  emit dataChanged(index(response.row()), index(response.row()));
}

/*========================== StatusUpdateThread =========================*/

RelinkingModel::StatusUpdateThread::StatusUpdateThread(RelinkingModel* owner)
    : QThread(owner),
      m_pOwner(owner),
      m_tasks(),
      m_rTasksByPath(m_tasks.get<OrderedByPathTag>()),
      m_rTasksByPriority(m_tasks.get<OrderedByPriorityTag>()),
      m_exiting(false) {}

RelinkingModel::StatusUpdateThread::~StatusUpdateThread() {
  {
    QMutexLocker locker(&m_mutex);
    m_exiting = true;
  }

  m_cond.wakeAll();
  wait();
}

void RelinkingModel::StatusUpdateThread::requestStatusUpdate(const QString& path, int row) {
  const QMutexLocker locker(&m_mutex);
  if (m_exiting) {
    return;
  }

  if (path == m_pathBeingProcessed) {
    // This task is currently in progress.
    return;
  }

  const std::pair<TasksByPath::iterator, bool> ins(m_rTasksByPath.insert(Task(path, row)));

  // Whether inserted or being already there, move it to the front of priority queue.
  m_rTasksByPriority.relocate(m_rTasksByPriority.end(), m_tasks.project<OrderedByPriorityTag>(ins.first));

  if (!isRunning()) {
    start();
  }

  m_cond.wakeOne();
}

void RelinkingModel::StatusUpdateThread::run() try {
  const QMutexLocker locker(&m_mutex);

  class MutexUnlocker {
   public:
    explicit MutexUnlocker(QMutex* mutex) : m_pMutex(mutex) { mutex->unlock(); }

    ~MutexUnlocker() { m_pMutex->lock(); }

   private:
    QMutex* const m_pMutex;
  };


  for (;;) {
    if (m_exiting) {
      break;
    }

    if (m_tasks.empty()) {
      m_cond.wait(&m_mutex);
    }

    if (m_tasks.empty()) {
      continue;
    }

    const Task task(m_rTasksByPriority.front());
    m_pathBeingProcessed = task.path;
    m_rTasksByPriority.pop_front();

    {
      const MutexUnlocker unlocker(&m_mutex);

      const bool exists = QFile::exists(task.path);
      const StatusUpdateResponse response(task.path, task.row, exists ? Exists : Missing);
      QCoreApplication::postEvent(m_pOwner, new PayloadEvent<StatusUpdateResponse>(response));
    }

    m_pathBeingProcessed.clear();
  }
}  // RelinkingModel::StatusUpdateThread::run

catch (const std::bad_alloc&) {
  OutOfMemoryHandler::instance().handleOutOfMemorySituation();
}

/*================================ Item =================================*/

RelinkingModel::Item::Item(const RelinkablePath& path)
    : origPath(path.normalizedPath()),
      committedPath(path.normalizedPath()),
      uncommittedPath(path.normalizedPath()),
      type(path.type()),
      committedStatus(StatusUpdatePending),
      uncommittedStatus(StatusUpdatePending) {}

/*============================== Relinker ================================*/

void RelinkingModel::Relinker::addMapping(const QString& from, const QString& to) {
  m_mappings[from] = to;
}

QString RelinkingModel::Relinker::substitutionPathFor(const RelinkablePath& path) const {
  const auto it(m_mappings.find(path.normalizedPath()));
  if (it != m_mappings.end()) {
    return it->second;
  } else {
    return path.normalizedPath();
  }
}

void RelinkingModel::Relinker::swap(Relinker& other) {
  m_mappings.swap(other.m_mappings);
}
