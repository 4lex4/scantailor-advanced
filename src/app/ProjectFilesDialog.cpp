// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ProjectFilesDialog.h"

#include <core/IconProvider.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <deque>

#include "ImageMetadataLoader.h"
#include "NonCopyable.h"
#include "SmartFilenameOrdering.h"

class ProjectFilesDialog::Item {
 public:
  enum Status { STATUS_DEFAULT, STATUS_LOAD_OK, STATUS_LOAD_FAILED };

  Item(const QFileInfo& fileInfo, Qt::ItemFlags flags)
      : m_fileInfo(fileInfo), m_flags(flags), m_status(STATUS_DEFAULT) {}

  const QFileInfo& fileInfo() const { return m_fileInfo; }

  Qt::ItemFlags flags() const { return m_flags; }

  Status status() const { return m_status; }

  void setStatus(Status status) { m_status = status; }

  const std::vector<ImageMetadata>& perPageMetadata() const { return m_perPageMetadata; }

  std::vector<ImageMetadata>& perPageMetadata() { return m_perPageMetadata; }

 private:
  QFileInfo m_fileInfo;
  Qt::ItemFlags m_flags;
  std::vector<ImageMetadata> m_perPageMetadata;
  Status m_status;
};


class ProjectFilesDialog::FileList : private QAbstractListModel {
  DECLARE_NON_COPYABLE(FileList)

 public:
  enum LoadStatus { LOAD_OK, LOAD_FAILED, NO_MORE_FILES };

  FileList();

  ~FileList() override;

  QAbstractItemModel* model() { return this; }

  template <typename OutFunc>
  void files(OutFunc out) const;

  const Item& item(const QModelIndex& index) { return m_items[index.row()]; }

  template <typename OutFunc>
  void items(OutFunc out) const;

  template <typename OutFunc>
  void items(const QItemSelection& selection, OutFunc out) const;

  size_t count() const { return m_items.size(); }

  void clear();

  template <typename It>
  void append(It begin, It end);

  template <typename It>
  void assign(It begin, It end);

  void remove(const QItemSelection& selection);

  void prepareForLoadingFiles();

  LoadStatus loadNextFile();

 private:
  int rowCount(const QModelIndex& parent) const override;

  QVariant data(const QModelIndex& index, int role) const override;

  Qt::ItemFlags flags(const QModelIndex& index) const override;

  std::vector<Item> m_items;
  std::deque<int> m_itemsToLoad;
};


class ProjectFilesDialog::SortedFileList : private QSortFilterProxyModel {
  DECLARE_NON_COPYABLE(SortedFileList)

 public:
  explicit SortedFileList(FileList& delegate);

  QAbstractProxyModel* model() { return this; }

 private:
  bool lessThan(const QModelIndex& lhs, const QModelIndex& rhs) const override;

  FileList& m_delegate;
};


class ProjectFilesDialog::ItemVisualOrdering {
 public:
  bool operator()(const Item& lhs, const Item& rhs) const;
};


template <typename OutFunc>
void ProjectFilesDialog::FileList::files(OutFunc out) const {
  auto it(m_items.begin());
  const auto end(m_items.end());
  for (; it != end; ++it) {
    out(it->fileInfo());
  }
}

template <typename OutFunc>
void ProjectFilesDialog::FileList::items(OutFunc out) const {
  std::for_each(m_items.begin(), m_items.end(), out);
}

template <typename OutFunc>
void ProjectFilesDialog::FileList::items(const QItemSelection& selection, OutFunc out) const {
  QListIterator<QItemSelectionRange> it(selection);
  while (it.hasNext()) {
    const QItemSelectionRange& range = it.next();
    for (int row = range.top(); row <= range.bottom(); ++row) {
      out(m_items[row]);
    }
  }
}

template <typename It>
void ProjectFilesDialog::FileList::append(It begin, It end) {
  if (begin == end) {
    return;
  }
  const size_t count = std::distance(begin, end);
  beginInsertRows(QModelIndex(), static_cast<int>(m_items.size()), static_cast<int>(m_items.size() + count - 1));
  m_items.insert(m_items.end(), begin, end);
  endInsertRows();
}

template <typename It>
void ProjectFilesDialog::FileList::assign(It begin, It end) {
  clear();
  append(begin, end);
}

ProjectFilesDialog::ProjectFilesDialog(QWidget* parent)
    : QDialog(parent),
      m_offProjectFiles(new FileList),
      m_offProjectFilesSorted(new SortedFileList(*m_offProjectFiles)),
      m_inProjectFiles(new FileList),
      m_inProjectFilesSorted(new SortedFileList(*m_inProjectFiles)),
      m_loadTimerId(0),
      m_metadataLoadFailed(false),
      m_autoOutDir(true) {
  m_supportedExtensions.insert("png");
  m_supportedExtensions.insert("jpg");
  m_supportedExtensions.insert("jpeg");
  m_supportedExtensions.insert("tif");
  m_supportedExtensions.insert("tiff");

  setupUi(this);

  setupIcons();

  offProjectList->setModel(m_offProjectFilesSorted->model());
  inProjectList->setModel(m_inProjectFilesSorted->model());

  connect(inpDirBrowseBtn, SIGNAL(clicked()), this, SLOT(inpDirBrowse()));
  connect(outDirBrowseBtn, SIGNAL(clicked()), this, SLOT(outDirBrowse()));
  connect(inpDirLine, SIGNAL(textEdited(const QString&)), this, SLOT(inpDirEdited(const QString&)));
  connect(outDirLine, SIGNAL(textEdited(const QString&)), this, SLOT(outDirEdited(const QString&)));
  connect(addToProjectBtn, SIGNAL(clicked()), this, SLOT(addToProject()));
  connect(removeFromProjectBtn, SIGNAL(clicked()), this, SLOT(removeFromProject()));
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(onOK()));
}

ProjectFilesDialog::~ProjectFilesDialog() = default;

QString ProjectFilesDialog::inputDirectory() const {
  return inpDirLine->text();
}

QString ProjectFilesDialog::outputDirectory() const {
  return outDirLine->text();
}


std::vector<ImageFileInfo> ProjectFilesDialog::inProjectFiles() const {
  std::vector<ImageFileInfo> files;
  m_inProjectFiles->items([&](const Item& item) { files.emplace_back(item.fileInfo(), item.perPageMetadata()); });

  std::sort(files.begin(), files.end(), [](const ImageFileInfo& lhs, const ImageFileInfo& rhs) {
    return SmartFilenameOrdering()(lhs.fileInfo(), rhs.fileInfo());
  });
  return files;
}

bool ProjectFilesDialog::isRtlLayout() const {
  return rtlLayoutCB->isChecked();
}

bool ProjectFilesDialog::isDpiFixingForced() const {
  return forceFixDpi->isChecked();
}

QString ProjectFilesDialog::sanitizePath(const QString& path) {
  QString trimmed(path.trimmed());
  if (trimmed.startsWith(QChar('"')) && trimmed.endsWith(QChar('"'))) {
    trimmed.chop(1);
    if (!trimmed.isEmpty()) {
      trimmed.remove(0, 1);
    }
  }
  return trimmed;
}

void ProjectFilesDialog::inpDirBrowse() {
  QSettings settings;

  QString initialDir(inpDirLine->text());
  if (initialDir.isEmpty() || !QDir(initialDir).exists()) {
    initialDir = settings.value("lastInputDir").toString();
  }
  if (initialDir.isEmpty() || !QDir(initialDir).exists()) {
    initialDir = QDir::home().absolutePath();
  } else {
    initialDir = QDir(initialDir).absolutePath();
  }

  const QString dir(QFileDialog::getExistingDirectory(this, tr("Input Directory"), initialDir));

  if (!dir.isEmpty()) {
    setInputDir(dir);
    settings.setValue("lastInputDir", dir);
  }
}

void ProjectFilesDialog::outDirBrowse() {
  QString initialDir(outDirLine->text());
  if (initialDir.isEmpty() || !QDir(initialDir).exists()) {
    initialDir = QDir::home().absolutePath();
  }

  const QString dir(QFileDialog::getExistingDirectory(this, tr("Output Directory"), initialDir));

  if (!dir.isEmpty()) {
    setOutputDir(dir);
  }
}

void ProjectFilesDialog::inpDirEdited(const QString& text) {
  setInputDir(sanitizePath(text), /* autoAddFiles= */ false);
}

void ProjectFilesDialog::outDirEdited(const QString& text) {
  m_autoOutDir = false;
}

namespace {
struct FileInfoLess {
  bool operator()(const QFileInfo& lhs, const QFileInfo& rhs) const {
    if (lhs == rhs) {
      // This takes into account filesystem's case sensitivity.
      return false;
    }
    return lhs.absoluteFilePath() < rhs.absoluteFilePath();
  }
};

}  // namespace

void ProjectFilesDialog::setInputDir(const QString& dir, const bool autoAddFiles) {
  inpDirLine->setText(QDir::toNativeSeparators(dir));
  if (m_autoOutDir) {
    setOutputDir(QDir::cleanPath(QDir(dir).filePath("out")));
  }

  QFileInfoList files(QDir(dir).entryInfoList(QDir::Files));

  {
    // Filter out files already in project.
    // Here we use simple ordering, which is OK.

    std::vector<QFileInfo> newFiles(files.begin(), files.end());
    std::vector<QFileInfo> existingFiles;
    m_inProjectFiles->files([&](const QFileInfo& fileInfo) { existingFiles.push_back(fileInfo); });
    std::sort(newFiles.begin(), newFiles.end(), FileInfoLess());
    std::sort(existingFiles.begin(), existingFiles.end(), FileInfoLess());

    files.clear();
    std::set_difference(newFiles.begin(), newFiles.end(), existingFiles.begin(), existingFiles.end(),
                        std::back_inserter(files), FileInfoLess());
  }

  using ItemList = std::vector<Item>;
  ItemList items;
  for (const QFileInfo& file : files) {
    Qt::ItemFlags flags;
    if (m_supportedExtensions.contains(file.suffix().toLower())) {
      flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
    items.emplace_back(file, flags);
  }

  m_offProjectFiles->assign(items.begin(), items.end());

  if (autoAddFiles && (m_inProjectFiles->count() == 0)) {
    offProjectList->selectAll();
    addToProject();
  }
}  // ProjectFilesDialog::setInputDir

void ProjectFilesDialog::setOutputDir(const QString& dir) {
  outDirLine->setText(QDir::toNativeSeparators(dir));
}

void ProjectFilesDialog::addToProject() {
  const QItemSelection selection(
      m_offProjectFilesSorted->model()->mapSelectionToSource(offProjectList->selectionModel()->selection()));

  using ItemList = std::vector<Item>;
  ItemList items;

  m_offProjectFiles->items(selection, [&](const Item& item) { items.push_back(item); });

  m_inProjectFiles->append(items.begin(), items.end());
  m_offProjectFiles->remove(selection);
}


void ProjectFilesDialog::removeFromProject() {
  const QDir inputDir(inpDirLine->text());

  const QItemSelection selection(
      m_inProjectFilesSorted->model()->mapSelectionToSource(inProjectList->selectionModel()->selection()));

  using ItemList = std::vector<Item>;
  ItemList items;

  m_inProjectFiles->items(selection, [&](const Item& item) {
    if (item.fileInfo().dir() == inputDir) {
      items.push_back(item);
    }
  });

  m_offProjectFiles->append(items.begin(), items.end());
  m_inProjectFiles->remove(selection);
}

void ProjectFilesDialog::onOK() {
  if (m_inProjectFiles->count() == 0) {
    QMessageBox::warning(this, tr("Error"), tr("No files in project!"));
    return;
  }

  const QDir inpDir(inpDirLine->text());
  if (!inpDir.isAbsolute() || !inpDir.exists()) {
    QMessageBox::warning(this, tr("Error"), tr("Input directory is not set or doesn't exist."));
    return;
  }

  const QDir outDir(outDirLine->text());
  if (inpDir == outDir) {
    QMessageBox::warning(this, tr("Error"), tr("Input and output directories can't be the same."));
    return;
  }

  if (outDir.isAbsolute() && !outDir.exists()) {
    // Maybe create it.
    bool create = m_autoOutDir;
    if (!m_autoOutDir) {
      create = QMessageBox::question(this, tr("Create Directory?"), tr("Output directory doesn't exist.  Create it?"),
                                     QMessageBox::Yes | QMessageBox::No)
               == QMessageBox::Yes;
      if (!create) {
        return;
      }
    }
    if (create) {
      if (!outDir.mkpath(outDir.path())) {
        QMessageBox::warning(this, tr("Error"), tr("Unable to create output directory."));
        return;
      }
    }
  }
  if (!outDir.isAbsolute() || !outDir.exists()) {
    QMessageBox::warning(this, tr("Error"), tr("Output directory is not set or doesn't exist."));
    return;
  }

  startLoadingMetadata();
}  // ProjectFilesDialog::onOK

void ProjectFilesDialog::startLoadingMetadata() {
  m_inProjectFiles->prepareForLoadingFiles();

  progressBar->setMaximum(static_cast<int>(m_inProjectFiles->count()));
  inpDirLine->setEnabled(false);
  inpDirBrowseBtn->setEnabled(false);
  outDirLine->setEnabled(false);
  outDirBrowseBtn->setEnabled(false);
  addToProjectBtn->setEnabled(false);
  removeFromProjectBtn->setEnabled(false);
  offProjectSelectAllBtn->setEnabled(false);
  inProjectSelectAllBtn->setEnabled(false);
  rtlLayoutCB->setEnabled(false);
  forceFixDpi->setEnabled(false);
  buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
  offProjectList->clearSelection();
  inProjectList->clearSelection();
  m_loadTimerId = startTimer(0);
  m_metadataLoadFailed = false;
}

void ProjectFilesDialog::timerEvent(QTimerEvent* event) {
  if (event->timerId() != m_loadTimerId) {
    QWidget::timerEvent(event);
    return;
  }

  switch (m_inProjectFiles->loadNextFile()) {
    case FileList::NO_MORE_FILES:
      finishLoadingMetadata();
      break;
    case FileList::LOAD_FAILED:
      m_metadataLoadFailed = true;
      // Fall through.
    case FileList::LOAD_OK:
      progressBar->setValue(progressBar->value() + 1);
      break;
  }
}

void ProjectFilesDialog::finishLoadingMetadata() {
  killTimer(m_loadTimerId);

  inpDirLine->setEnabled(true);
  inpDirBrowseBtn->setEnabled(true);
  outDirLine->setEnabled(true);
  outDirBrowseBtn->setEnabled(true);
  addToProjectBtn->setEnabled(true);
  removeFromProjectBtn->setEnabled(true);
  offProjectSelectAllBtn->setEnabled(true);
  inProjectSelectAllBtn->setEnabled(true);
  rtlLayoutCB->setEnabled(true);
  forceFixDpi->setEnabled(true);
  buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

  if (m_metadataLoadFailed) {
    progressBar->setValue(0);
    QMessageBox::warning(this, tr("Error"),
                         tr("Some of the files failed to load.\n"
                            "Either we don't support their format, or they are broken.\n"
                            "You should remove them from the project."));
    return;
  }

  accept();
}

void ProjectFilesDialog::setupIcons() {
  auto& iconProvider = IconProvider::getInstance();
  addToProjectBtn->setIcon(iconProvider.getIcon("right-arrow-inscribed"));
  removeFromProjectBtn->setIcon(iconProvider.getIcon("left-arrow-inscribed"));
}

/*====================== ProjectFilesDialog::FileList ====================*/

ProjectFilesDialog::FileList::FileList() = default;

ProjectFilesDialog::FileList::~FileList() = default;

void ProjectFilesDialog::FileList::clear() {
  if (m_items.empty()) {
    return;
  }
  beginRemoveRows(QModelIndex(), 0, static_cast<int>(m_items.size() - 1));
  m_items.clear();
  endRemoveRows();
}

void ProjectFilesDialog::FileList::remove(const QItemSelection& selection) {
  if (selection.isEmpty()) {
    return;
  }

  using Range = std::pair<int, int>;
  QVector<Range> sortedRanges;
  for (const auto& range : selection) {
    sortedRanges.push_back(Range(range.top(), range.bottom()));
  }

  std::sort(sortedRanges.begin(), sortedRanges.end(),
            [](const Range& lhs, const Range& rhs) { return lhs.first < rhs.first; });

  QVectorIterator<Range> it(sortedRanges);
  int rowsRemoved = 0;
  while (it.hasNext()) {
    const Range& range = it.next();
    const int first = range.first - rowsRemoved;
    const int last = range.second - rowsRemoved;
    beginRemoveRows(QModelIndex(), first, last);
    m_items.erase(m_items.begin() + first, m_items.begin() + (last + 1));
    endRemoveRows();
    rowsRemoved += last - first + 1;
  }
}  // ProjectFilesDialog::FileList::remove

int ProjectFilesDialog::FileList::rowCount(const QModelIndex&) const {
  return static_cast<int>(m_items.size());
}

QVariant ProjectFilesDialog::FileList::data(const QModelIndex& index, const int role) const {
  const Item& item = m_items[index.row()];
  switch (role) {
    case Qt::DisplayRole:
      return item.fileInfo().fileName();
    case Qt::ForegroundRole:
      switch (item.status()) {
        case Item::STATUS_DEFAULT:
          return QVariant();
        case Item::STATUS_LOAD_OK:
          return QBrush(QColor(0x00, 0xff, 0x00));
        case Item::STATUS_LOAD_FAILED:
          return QBrush(QColor(0xff, 0x00, 0x00));
      }
      break;
    default:
      break;
  }
  return QVariant();
}

Qt::ItemFlags ProjectFilesDialog::FileList::flags(const QModelIndex& index) const {
  return m_items[index.row()].flags();
}

void ProjectFilesDialog::FileList::prepareForLoadingFiles() {
  std::deque<int> itemIndexes;
  const auto numItems = static_cast<int>(m_items.size());
  for (int i = 0; i < numItems; ++i) {
    itemIndexes.push_back(i);
  }

  std::sort(itemIndexes.begin(), itemIndexes.end(),
            [&](int lhs, int rhs) { return ItemVisualOrdering()(m_items[lhs], m_items[rhs]); });

  m_itemsToLoad.swap(itemIndexes);
}

ProjectFilesDialog::FileList::LoadStatus ProjectFilesDialog::FileList::loadNextFile() {
  if (m_itemsToLoad.empty()) {
    return NO_MORE_FILES;
  }

  const int itemIdx = m_itemsToLoad.front();
  Item& item = m_items[itemIdx];
  std::vector<ImageMetadata> perPageMetadata;
  const QString filePath(item.fileInfo().absoluteFilePath());
  const ImageMetadataLoader::Status st = ImageMetadataLoader::load(
      filePath, [&](const ImageMetadata& metadata) { perPageMetadata.push_back(metadata); });

  LoadStatus status;

  if (st == ImageMetadataLoader::LOADED) {
    status = LOAD_OK;
    item.perPageMetadata().swap(perPageMetadata);
    item.setStatus(Item::STATUS_LOAD_OK);
  } else {
    status = LOAD_FAILED;
    item.setStatus(Item::STATUS_LOAD_FAILED);
  }
  const QModelIndex idx(index(itemIdx, 0));
  emit dataChanged(idx, idx);

  m_itemsToLoad.pop_front();
  return status;
}  // ProjectFilesDialog::FileList::loadNextFile

/*================= ProjectFilesDialog::SortedFileList ===================*/

ProjectFilesDialog::SortedFileList::SortedFileList(FileList& delegate) : m_delegate(delegate) {
  setSourceModel(delegate.model());
  setDynamicSortFilter(true);
  sort(0);
}

bool ProjectFilesDialog::SortedFileList::lessThan(const QModelIndex& lhs, const QModelIndex& rhs) const {
  const Item& lhsItem = m_delegate.item(lhs);
  const Item& rhsItem = m_delegate.item(rhs);
  return ItemVisualOrdering()(lhsItem, rhsItem);
}

/*=============== ProjectFilesDialog::ItemVisualOrdering =================*/

bool ProjectFilesDialog::ItemVisualOrdering::operator()(const Item& lhs, const Item& rhs) const {
  const bool lhsFailed = (lhs.status() == Item::STATUS_LOAD_FAILED);
  const bool rhsFailed = (rhs.status() == Item::STATUS_LOAD_FAILED);
  if (lhsFailed != rhsFailed) {
    // Failed ones go to the top.
    return lhsFailed;
  }
  return SmartFilenameOrdering()(lhs.fileInfo(), rhs.fileInfo());
}
