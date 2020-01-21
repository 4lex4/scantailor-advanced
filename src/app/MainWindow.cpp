// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "MainWindow.h"

#include <core/ApplicationSettings.h>
#include <core/IconProvider.h>

#include <QDir>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QResource>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <QtWidgets/QInputDialog>
#include <boost/lambda/lambda.hpp>
#include <memory>

#include "AbstractRelinker.h"
#include "Application.h"
#include "AutoRemovingFile.h"
#include "BasicImageView.h"
#include "ContentBoxPropagator.h"
#include "DebugImageView.h"
#include "DebugImages.h"
#include "DefaultParamsDialog.h"
#include "ErrorWidget.h"
#include "FilterOptionsWidget.h"
#include "FixDpiDialog.h"
#include "ImageInfo.h"
#include "ImageMetadataLoader.h"
#include "LoadFileTask.h"
#include "LoadFilesStatusDialog.h"
#include "NewOpenProjectPanel.h"
#include "OutOfMemoryDialog.h"
#include "OutOfMemoryHandler.h"
#include "PageOrientationPropagator.h"
#include "PageSelectionAccessor.h"
#include "PageSequence.h"
#include "ProcessingIndicationWidget.h"
#include "ProcessingTaskQueue.h"
#include "ProjectCreationContext.h"
#include "ProjectOpeningContext.h"
#include "ProjectPages.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "RecentProjects.h"
#include "RelinkingDialog.h"
#include "ScopedIncDec.h"
#include "SettingsDialog.h"
#include "SkinnedButton.h"
#include "SmartFilenameOrdering.h"
#include "StageSequence.h"
#include "SystemLoadWidget.h"
#include "TabbedDebugImages.h"
#include "ThumbnailFactory.h"
#include "UnitsProvider.h"
#include "Utils.h"
#include "WorkerThreadPool.h"
#include "filters/deskew/CacheDrivenTask.h"
#include "filters/deskew/Task.h"
#include "filters/fix_orientation/CacheDrivenTask.h"
#include "filters/fix_orientation/Task.h"
#include "filters/output/CacheDrivenTask.h"
#include "filters/output/TabbedImageView.h"
#include "filters/output/Task.h"
#include "filters/page_layout/CacheDrivenTask.h"
#include "filters/page_layout/Task.h"
#include "filters/page_split/CacheDrivenTask.h"
#include "filters/page_split/Task.h"
#include "filters/select_content/CacheDrivenTask.h"
#include "filters/select_content/Task.h"
#include "ui_AboutDialog.h"
#include "ui_BatchProcessingLowerPanel.h"
#include "ui_RemovePagesDialog.h"
#include "version.h"

using namespace core;

class MainWindow::PageSelectionProviderImpl : public PageSelectionProvider {
 public:
  explicit PageSelectionProviderImpl(MainWindow* wnd) : m_wnd(wnd) {}

  PageSequence allPages() const override { return m_wnd ? m_wnd->allPages() : PageSequence(); }

  std::set<PageId> selectedPages() const override { return m_wnd ? m_wnd->selectedPages() : std::set<PageId>(); }

  std::vector<PageRange> selectedRanges() const override {
    return m_wnd ? m_wnd->selectedRanges() : std::vector<PageRange>();
  }

 private:
  QPointer<MainWindow> m_wnd;
};


MainWindow::MainWindow()
    : m_pages(new ProjectPages),
      m_stages(new StageSequence(m_pages, newPageSelectionAccessor())),
      m_workerThreadPool(new WorkerThreadPool),
      m_interactiveQueue(new ProcessingTaskQueue()),
      m_outOfMemoryDialog(new OutOfMemoryDialog),
      m_curFilter(0),
      m_ignoreSelectionChanges(0),
      m_ignorePageOrderingChanges(0),
      m_debug(false),
      m_closing(false) {
  ApplicationSettings& settings = ApplicationSettings::getInstance();

  m_maxLogicalThumbSize = settings.getMaxLogicalThumbnailSize();
  const ThumbnailSequence::ViewMode viewMode = settings.isSingleColumnThumbnailDisplayEnabled()
                                                   ? ThumbnailSequence::SINGLE_COLUMN
                                                   : ThumbnailSequence::MULTI_COLUMN;
  m_thumbSequence = std::make_unique<ThumbnailSequence>(m_maxLogicalThumbSize, viewMode);

  m_autoSaveTimer.setSingleShot(true);
  connect(&m_autoSaveTimer, SIGNAL(timeout()), SLOT(autoSaveProject()));

  setupUi(this);
  setupIcons();

  sortOptions->setVisible(false);

  createBatchProcessingWidget();
  m_processingIndicationWidget = std::make_unique<ProcessingIndicationWidget>();

  filterList->setStages(m_stages);
  filterList->selectRow(0);

  setupThumbView();  // Expects m_thumbSequence to be initialized.
  m_tabbedDebugImages = std::make_unique<TabbedDebugImages>();

  m_debug = actionDebug->isChecked();

  m_imageFrameLayout = new QStackedLayout(imageViewFrame);
  m_imageFrameLayout->setStackingMode(QStackedLayout::StackAll);

  m_optionsFrameLayout = new QStackedLayout(filterOptions);

  m_statusBarPanel = std::make_unique<StatusBarPanel>();
  QMainWindow::statusBar()->addPermanentWidget(m_statusBarPanel.get());
  connect(m_thumbSequence.get(), &ThumbnailSequence::newSelectionLeader, [this](const PageInfo& pageInfo) {
    PageSequence pageSequence = m_thumbSequence->toPageSequence();
    if (pageSequence.numPages() > 0) {
      m_statusBarPanel->updatePage(pageSequence.pageNo(pageInfo.id()) + 1, pageSequence.numPages(), pageInfo.id());
    } else {
      m_statusBarPanel->clear();
    }
  });

  m_unitsMenuActionGroup = std::make_unique<QActionGroup>(this);
  for (QAction* action : menuUnits->actions()) {
    m_unitsMenuActionGroup->addAction(action);
  }
  switch (unitsFromString(settings.getUnits())) {
    case PIXELS:
      actionPixels->setChecked(true);
      break;
    case MILLIMETRES:
      actionMilimeters->setChecked(true);
      break;
    case CENTIMETRES:
      actionCentimetres->setChecked(true);
      break;
    case INCHES:
      actionInches->setChecked(true);
      break;
  }
  connect(actionPixels, &QAction::toggled, [this, &settings](bool checked) {
    if (checked) {
      UnitsProvider::getInstance().setUnits(PIXELS);
      settings.setUnits(unitsToString(PIXELS));
    }
  });
  connect(actionMilimeters, &QAction::toggled, [this, &settings](bool checked) {
    if (checked) {
      UnitsProvider::getInstance().setUnits(MILLIMETRES);
      settings.setUnits(unitsToString(MILLIMETRES));
    }
  });
  connect(actionCentimetres, &QAction::toggled, [this, &settings](bool checked) {
    if (checked) {
      UnitsProvider::getInstance().setUnits(CENTIMETRES);
      settings.setUnits(unitsToString(CENTIMETRES));
    }
  });
  connect(actionInches, &QAction::toggled, [this, &settings](bool checked) {
    if (checked) {
      UnitsProvider::getInstance().setUnits(INCHES);
      settings.setUnits(unitsToString(INCHES));
    }
  });

  thumbColumnViewBtn->setChecked(settings.isSingleColumnThumbnailDisplayEnabled());

  addAction(actionFirstPage);
  addAction(actionLastPage);
  addAction(actionNextPage);
  addAction(actionPrevPage);
  addAction(actionPrevPageQ);
  addAction(actionNextPageW);
  addAction(actionNextSelectedPage);
  addAction(actionPrevSelectedPage);
  addAction(actionNextSelectedPageW);
  addAction(actionPrevSelectedPageQ);
  addAction(actionGotoPage);

  addAction(actionSwitchFilter1);
  addAction(actionSwitchFilter2);
  addAction(actionSwitchFilter3);
  addAction(actionSwitchFilter4);
  addAction(actionSwitchFilter5);
  addAction(actionSwitchFilter6);
  // Should be enough to save a project.
  OutOfMemoryHandler::instance().allocateEmergencyMemory(3 * 1024 * 1024);

  connect(actionFirstPage, SIGNAL(triggered(bool)), SLOT(goFirstPage()));
  connect(actionLastPage, SIGNAL(triggered(bool)), SLOT(goLastPage()));
  connect(actionPrevPage, SIGNAL(triggered(bool)), SLOT(goPrevPage()));
  connect(actionNextPage, SIGNAL(triggered(bool)), SLOT(goNextPage()));
  connect(actionPrevPageQ, SIGNAL(triggered(bool)), this, SLOT(goPrevPage()));
  connect(actionNextPageW, SIGNAL(triggered(bool)), this, SLOT(goNextPage()));
  connect(actionPrevSelectedPage, SIGNAL(triggered(bool)), SLOT(goPrevSelectedPage()));
  connect(actionNextSelectedPage, SIGNAL(triggered(bool)), SLOT(goNextSelectedPage()));
  connect(actionPrevSelectedPageQ, SIGNAL(triggered(bool)), this, SLOT(goPrevSelectedPage()));
  connect(actionNextSelectedPageW, SIGNAL(triggered(bool)), this, SLOT(goNextSelectedPage()));
  connect(actionGotoPage, SIGNAL(triggered(bool)), this, SLOT(execGotoPageDialog()));
  connect(actionAbout, SIGNAL(triggered(bool)), this, SLOT(showAboutDialog()));
  connect(&OutOfMemoryHandler::instance(), SIGNAL(outOfMemory()), SLOT(handleOutOfMemorySituation()));
  connect(prevPageBtn, &QToolButton::clicked, this, [this]() {
    if (filterSelectedBtn->isChecked()) {
      goPrevSelectedPage();
    } else {
      goPrevPage();
    }
  });
  connect(nextPageBtn, &QToolButton::clicked, this, [this]() {
    if (filterSelectedBtn->isChecked()) {
      goNextSelectedPage();
    } else {
      goNextPage();
    }
  });
  connect(gotoPageBtn, SIGNAL(clicked()), this, SLOT(execGotoPageDialog()));

  connect(actionSwitchFilter1, SIGNAL(triggered(bool)), SLOT(switchFilter1()));
  connect(actionSwitchFilter2, SIGNAL(triggered(bool)), SLOT(switchFilter2()));
  connect(actionSwitchFilter3, SIGNAL(triggered(bool)), SLOT(switchFilter3()));
  connect(actionSwitchFilter4, SIGNAL(triggered(bool)), SLOT(switchFilter4()));
  connect(actionSwitchFilter5, SIGNAL(triggered(bool)), SLOT(switchFilter5()));
  connect(actionSwitchFilter6, SIGNAL(triggered(bool)), SLOT(switchFilter6()));

  connect(filterList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
          SLOT(filterSelectionChanged(const QItemSelection&)));
  connect(filterList, SIGNAL(launchBatchProcessing()), this, SLOT(startBatchProcessing()));

  connect(m_workerThreadPool.get(), SIGNAL(taskResult(const BackgroundTaskPtr&, const FilterResultPtr&)), this,
          SLOT(filterResult(const BackgroundTaskPtr&, const FilterResultPtr&)));

  connect(m_thumbSequence.get(),
          SIGNAL(newSelectionLeader(const PageInfo&, const QRectF&, ThumbnailSequence::SelectionFlags)), this,
          SLOT(currentPageChanged(const PageInfo&, const QRectF&, ThumbnailSequence::SelectionFlags)));
  connect(m_thumbSequence.get(), SIGNAL(pageContextMenuRequested(const PageInfo&, const QPoint&, bool)), this,
          SLOT(pageContextMenuRequested(const PageInfo&, const QPoint&, bool)));
  connect(m_thumbSequence.get(), SIGNAL(pastLastPageContextMenuRequested(const QPoint&)),
          SLOT(pastLastPageContextMenuRequested(const QPoint&)));
  connect(selectionModeBtn, SIGNAL(clicked(bool)), m_thumbSequence.get(), SLOT(setSelectionModeEnabled(bool)));

  connect(thumbView->verticalScrollBar(), SIGNAL(sliderMoved(int)), this, SLOT(thumbViewScrolled()));
  connect(thumbView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(thumbViewScrolled()));
  connect(focusButton, SIGNAL(clicked(bool)), this, SLOT(thumbViewFocusToggled(bool)));
  connect(sortOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(pageOrderingChanged(int)));
  connect(thumbColumnViewBtn, &QToolButton::clicked, this, [this, &settings](bool checked) {
    settings.setSingleColumnThumbnailDisplayEnabled(checked);
    updateThumbnailViewMode();
    m_thumbSequence->updateSceneItemsPos();
  });
  connect(sortingOrderBtn, &QToolButton::clicked, this, [this](bool) {
    pageOrderingChanged(m_stages->filterAt(m_curFilter)->selectedPageOrder());
  });

  connect(actionFixDpi, SIGNAL(triggered(bool)), SLOT(fixDpiDialogRequested()));
  connect(actionRelinking, SIGNAL(triggered(bool)), SLOT(showRelinkingDialog()));
#ifndef NDEBUG
  connect(actionDebug, SIGNAL(toggled(bool)), SLOT(debugToggled(bool)));
#else
  actionDebug->setVisible(false);
#endif

  connect(actionSettings, SIGNAL(triggered(bool)), this, SLOT(openSettingsDialog()));
  connect(actionDefaults, SIGNAL(triggered(bool)), this, SLOT(openDefaultParamsDialog()));

  connect(actionNewProject, SIGNAL(triggered(bool)), this, SLOT(newProject()));
  connect(actionOpenProject, SIGNAL(triggered(bool)), this, SLOT(openProject()));
  connect(actionSaveProject, SIGNAL(triggered(bool)), this, SLOT(saveProjectTriggered()));
  connect(actionSaveProjectAs, SIGNAL(triggered(bool)), this, SLOT(saveProjectAsTriggered()));
  connect(actionCloseProject, SIGNAL(triggered(bool)), this, SLOT(closeProject()));
  connect(actionQuit, SIGNAL(triggered(bool)), this, SLOT(close()));

  updateProjectActions();
  updateWindowTitle();
  updateMainArea();

  QSettings qSettings;
  if (qSettings.value("mainWindow/maximized") == false) {
    const QVariant geom(qSettings.value("mainWindow/nonMaximizedGeometry"));
    if (!restoreGeometry(geom.toByteArray())) {
      resize(1014, 689);  // A sensible value.
    }
  }

  m_maxLogicalThumbSizeUpdater.setSingleShot(true);
  connect(&m_maxLogicalThumbSizeUpdater, &QTimer::timeout, this, &MainWindow::updateMaxLogicalThumbSize);

  m_sceneItemsPosUpdater.setSingleShot(true);
  connect(&m_sceneItemsPosUpdater, &QTimer::timeout, m_thumbSequence.get(), &ThumbnailSequence::updateSceneItemsPos);
}

MainWindow::~MainWindow() {
  m_interactiveQueue->cancelAndClear();
  if (m_batchQueue) {
    m_batchQueue->cancelAndClear();
  }
  m_workerThreadPool->shutdown();

  removeWidgetsFromLayout(m_imageFrameLayout);
  removeWidgetsFromLayout(m_optionsFrameLayout);
  m_tabbedDebugImages->clear();
}

PageSequence MainWindow::allPages() const {
  return m_thumbSequence->toPageSequence();
}

std::set<PageId> MainWindow::selectedPages() const {
  return m_thumbSequence->selectedItems();
}

std::vector<PageRange> MainWindow::selectedRanges() const {
  return m_thumbSequence->selectedRanges();
}

void MainWindow::switchToNewProject(const intrusive_ptr<ProjectPages>& pages,
                                    const QString& outDir,
                                    const QString& projectFilePath,
                                    const ProjectReader* projectReader) {
  stopBatchProcessing(CLEAR_MAIN_AREA);
  m_interactiveQueue->cancelAndClear();

  if (!outDir.isEmpty()) {
    Utils::maybeCreateCacheDir(outDir);
  }
  m_pages = pages;
  m_projectFile = projectFilePath;

  if (projectReader) {
    m_selectedPage = projectReader->selectedPage();
  }

  intrusive_ptr<FileNameDisambiguator> disambiguator;
  if (projectReader) {
    disambiguator = projectReader->namingDisambiguator();
  } else {
    disambiguator.reset(new FileNameDisambiguator);
  }

  m_outFileNameGen = OutputFileNameGenerator(disambiguator, outDir, pages->layoutDirection());
  // These two need to go in this order.
  updateDisambiguationRecords(pages->toPageSequence(IMAGE_VIEW));

  // Recreate the stages and load their state.
  m_stages = make_intrusive<StageSequence>(pages, newPageSelectionAccessor());
  if (projectReader) {
    projectReader->readFilterSettings(m_stages->filters());
  }

  // Connect the filter list model to the view and select
  // the first item.
  {
    ScopedIncDec<int> guard(m_ignoreSelectionChanges);
    filterList->setStages(m_stages);
    filterList->selectRow(0);
    m_curFilter = 0;
    // Setting a data model also implicitly sets a new
    // selection model, so we have to reconnect to it.
    connect(filterList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
            SLOT(filterSelectionChanged(const QItemSelection&)));
  }

  updateSortOptions();

  m_contentBoxPropagator = std::make_unique<ContentBoxPropagator>(
      m_stages->pageLayoutFilter(), createCompositeCacheDrivenTask(m_stages->selectContentFilterIdx()));

  m_pageOrientationPropagator = std::make_unique<PageOrientationPropagator>(
      m_stages->pageSplitFilter(), createCompositeCacheDrivenTask(m_stages->fixOrientationFilterIdx()));

  // Thumbnails are stored relative to the output directory,
  // so recreate the thumbnail cache.
  if (outDir.isEmpty()) {
    m_thumbnailCache.reset();
  } else {
    m_thumbnailCache = Utils::createThumbnailCache(m_outFileNameGen.outDir());
  }
  resetThumbSequence(currentPageOrderProvider());

  removeFilterOptionsWidget();
  updateProjectActions();
  updateWindowTitle();
  updateMainArea();

  if (!QDir(outDir).exists()) {
    showRelinkingDialog();
  }
}  // MainWindow::switchToNewProject

void MainWindow::showNewOpenProjectPanel() {
  auto outerWidget = std::make_unique<QWidget>();
  auto* layout = new QGridLayout(outerWidget.get());
  outerWidget->setLayout(layout);

  auto* nop = new NewOpenProjectPanel(outerWidget.get());
  // We use asynchronous connections because otherwise we
  // would be deleting a widget from its event handler, which
  // Qt doesn't like.
  connect(nop, SIGNAL(newProject()), this, SLOT(newProject()), Qt::QueuedConnection);
  connect(nop, SIGNAL(openProject()), this, SLOT(openProject()), Qt::QueuedConnection);
  connect(nop, SIGNAL(openRecentProject(const QString&)), this, SLOT(openProject(const QString&)),
          Qt::QueuedConnection);

  layout->addWidget(nop, 1, 1);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(2, 1);
  layout->setRowStretch(0, 1);
  layout->setRowStretch(2, 1);
  setImageWidget(outerWidget.release(), TRANSFER_OWNERSHIP);

  filterList->setBatchProcessingPossible(false);
}  // MainWindow::showNewOpenProjectPanel

void MainWindow::createBatchProcessingWidget() {
  m_batchProcessingWidget = std::make_unique<QWidget>();
  auto* layout = new QGridLayout(m_batchProcessingWidget.get());
  m_batchProcessingWidget->setLayout(layout);

  const auto& iconProvider = IconProvider::getInstance();
  auto* stopBtn = new SkinnedButton(iconProvider.getIcon("stop"), iconProvider.getIcon("stop-hovered"),
                                    iconProvider.getIcon("stop-pressed"), m_batchProcessingWidget.get());
  stopBtn->setIconSize({128, 128});
  stopBtn->setStatusTip(tr("Stop batch processing"));

  class LowerPanel : public QWidget {
   public:
    explicit LowerPanel(QWidget* parent = 0) : QWidget(parent) { ui.setupUi(this); }

    Ui::BatchProcessingLowerPanel ui;
  };


  auto* lowerPanel = new LowerPanel(m_batchProcessingWidget.get());
  m_checkBeepWhenFinished = [lowerPanel]() { return lowerPanel->ui.beepWhenFinished->isChecked(); };

  int row = 0;  // Row 0 is reserved.
  layout->addWidget(stopBtn, ++row, 1, Qt::AlignCenter);
  layout->addWidget(lowerPanel, ++row, 0, 1, 3, Qt::AlignHCenter | Qt::AlignTop);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(2, 1);
  layout->setRowStretch(0, 1);
  layout->setRowStretch(row, 1);

  connect(stopBtn, SIGNAL(clicked()), SLOT(stopBatchProcessing()));
}  // MainWindow::createBatchProcessingWidget

void MainWindow::updateThumbViewMinWidth() {
  const int sb = thumbView->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
  int innerWidth = thumbView->maximumViewportSize().width() - sb;
  if (thumbView->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, nullptr, thumbView)) {
    innerWidth -= thumbView->frameWidth() * 2;
  }
  const int deltaX = thumbView->size().width() - innerWidth;
  thumbView->setMinimumWidth((int) std::ceil(m_maxLogicalThumbSize.width() + deltaX));
}

void MainWindow::setupThumbView() {
  updateThumbViewMinWidth();
  m_thumbSequence->attachView(thumbView);
  thumbView->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* ev) {
  if ((obj == thumbView) && (ev->type() == QEvent::Resize)) {
    if (!m_sceneItemsPosUpdater.isActive()) {
      m_sceneItemsPosUpdater.start(150);
    }
  }

  if ((obj == thumbView || obj == thumbView->verticalScrollBar()) && (ev->type() == QEvent::Wheel)) {
    auto* wheelEvent = static_cast<QWheelEvent*>(ev);
    if (wheelEvent->modifiers() == Qt::AltModifier) {
      scaleThumbnails(wheelEvent);
      wheelEvent->accept();
      return true;
    }
  }
  return false;
}

void MainWindow::closeEvent(QCloseEvent* const event) {
  if (m_closing) {
    event->accept();
  } else {
    event->ignore();
    startTimer(0);
  }
}

void MainWindow::timerEvent(QTimerEvent* const event) {
  // We only use the timer event for delayed closing of the window.
  killTimer(event->timerId());

  if (closeProjectInteractive()) {
    m_closing = true;
    QSettings settings;
    settings.setValue("mainWindow/maximized", isMaximized());
    if (!isMaximized()) {
      settings.setValue("mainWindow/nonMaximizedGeometry", saveGeometry());
    }
    close();
  }
}

MainWindow::SavePromptResult MainWindow::promptProjectSave() {
  QMessageBox msgBox(QMessageBox::Question, tr("Save Project"), tr("Save the project?"),
                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
  msgBox.setDefaultButton(QMessageBox::Yes);

  switch (msgBox.exec()) {
    case QMessageBox::Yes:
      return SAVE;
    case QMessageBox::No:
      return DONT_SAVE;
    default:
      return CANCEL;
  }
}

bool MainWindow::compareFiles(const QString& fpath1, const QString& fpath2) {
  QFile file1(fpath1);
  QFile file2(fpath2);

  if (!file1.open(QIODevice::ReadOnly)) {
    return false;
  }
  if (!file2.open(QIODevice::ReadOnly)) {
    return false;
  }

  if (!file1.isSequential() && !file2.isSequential()) {
    if (file1.size() != file2.size()) {
      return false;
    }
  }

  const int chunkSize = 4096;
  while (true) {
    const QByteArray chunk1(file1.read(chunkSize));
    const QByteArray chunk2(file2.read(chunkSize));
    if (chunk1.size() != chunk2.size()) {
      return false;
    } else if (chunk1.size() == 0) {
      return true;
    }
  }
}

intrusive_ptr<const PageOrderProvider> MainWindow::currentPageOrderProvider() const {
  const int idx = sortOptions->currentIndex();
  if (idx < 0) {
    return nullptr;
  }

  const intrusive_ptr<AbstractFilter> filter(m_stages->filterAt(m_curFilter));
  intrusive_ptr<const PageOrderProvider> currentOrderProvider = filter->pageOrderOptions()[idx].provider();
  return (currentOrderProvider && sortingOrderBtn->isChecked()) ? currentOrderProvider->reversed()
                                                                : currentOrderProvider;
}

void MainWindow::updateSortOptions() {
  const ScopedIncDec<int> guard(m_ignorePageOrderingChanges);

  const intrusive_ptr<AbstractFilter> filter(m_stages->filterAt(m_curFilter));

  sortOptions->clear();

  for (const PageOrderOption& opt : filter->pageOrderOptions()) {
    sortOptions->addItem(opt.name());
  }

  sortOptions->setVisible(sortOptions->count() > 0);

  if (sortOptions->count() > 0) {
    sortOptions->setCurrentIndex(filter->selectedPageOrder());
  }
}

void MainWindow::resetThumbSequence(const intrusive_ptr<const PageOrderProvider>& pageOrderProvider,
                                    const ThumbnailSequence::SelectionAction selectionAction) {
  if (m_thumbnailCache) {
    const intrusive_ptr<CompositeCacheDrivenTask> task(createCompositeCacheDrivenTask(m_curFilter));

    m_thumbSequence->setThumbnailFactory(
        make_intrusive<ThumbnailFactory>(m_thumbnailCache, m_maxLogicalThumbSize, task));
  }

  m_thumbSequence->reset(currentPageSequence(), selectionAction, pageOrderProvider);

  if (!m_thumbnailCache) {
    // Empty project.
    assert(m_pages->numImages() == 0);
    m_thumbSequence->setThumbnailFactory(nullptr);
  }

  if (selectionAction != ThumbnailSequence::KEEP_SELECTION) {
    const PageId page(m_selectedPage.get(getCurrentView()));
    if (m_thumbSequence->setSelection(page)) {
      // OK
    } else if (m_thumbSequence->setSelection(PageId(page.imageId(), PageId::LEFT_PAGE))) {
      // OK
    } else if (m_thumbSequence->setSelection(PageId(page.imageId(), PageId::RIGHT_PAGE))) {
      // OK
    } else if (m_thumbSequence->setSelection(PageId(page.imageId(), PageId::SINGLE_PAGE))) {
      // OK
    } else {
      // Last resort.
      m_thumbSequence->setSelection(m_thumbSequence->firstPage().id());
    }
  }
}

void MainWindow::setOptionsWidget(FilterOptionsWidget* widget, const Ownership ownership) {
  if (isBatchProcessingInProgress()) {
    if (ownership == TRANSFER_OWNERSHIP) {
      delete widget;
    }
    return;
  }

  if (m_optionsWidget != widget) {
    removeWidgetsFromLayout(m_optionsFrameLayout);
  }
  // Delete the old widget we were owning, if any.
  m_optionsWidgetCleanup.clear();

  if (ownership == TRANSFER_OWNERSHIP) {
    m_optionsWidgetCleanup.add(widget);
  }

  if (m_optionsWidget == widget) {
    return;
  }

  if (m_optionsWidget) {
    disconnect(m_optionsWidget, SIGNAL(reloadRequested()), this, SLOT(reloadRequested()));
    disconnect(m_optionsWidget, SIGNAL(invalidateThumbnail(const PageId&)), this,
               SLOT(invalidateThumbnail(const PageId&)));
    disconnect(m_optionsWidget, SIGNAL(invalidateThumbnail(const PageInfo&)), this,
               SLOT(invalidateThumbnail(const PageInfo&)));
    disconnect(m_optionsWidget, SIGNAL(invalidateAllThumbnails()), this, SLOT(invalidateAllThumbnails()));
    disconnect(m_optionsWidget, SIGNAL(goToPage(const PageId&)), this, SLOT(goToPage(const PageId&)));
  }

  m_optionsFrameLayout->addWidget(widget);
  m_optionsWidget = widget;

  // We use an asynchronous connection here, because the slot
  // will probably delete the options panel, which could be
  // responsible for the emission of this signal.  Qt doesn't
  // like when we delete an object while it's emitting a signal.
  connect(widget, SIGNAL(reloadRequested()), this, SLOT(reloadRequested()), Qt::QueuedConnection);
  connect(widget, SIGNAL(invalidateThumbnail(const PageId&)), this, SLOT(invalidateThumbnail(const PageId&)));
  connect(widget, SIGNAL(invalidateThumbnail(const PageInfo&)), this, SLOT(invalidateThumbnail(const PageInfo&)));
  connect(widget, SIGNAL(invalidateAllThumbnails()), this, SLOT(invalidateAllThumbnails()));
  connect(widget, SIGNAL(goToPage(const PageId&)), this, SLOT(goToPage(const PageId&)));
}  // MainWindow::setOptionsWidget

void MainWindow::setImageWidget(QWidget* widget, const Ownership ownership, DebugImages* debugImages, bool overlay) {
  if (isBatchProcessingInProgress() && (widget != m_batchProcessingWidget.get())) {
    if (ownership == TRANSFER_OWNERSHIP) {
      delete widget;
    }
    return;
  }

  if (!overlay) {
    removeImageWidget();
  }

  if (ownership == TRANSFER_OWNERSHIP) {
    m_imageWidgetCleanup.add(widget);
  }

  if (!debugImages || debugImages->empty()) {
    if (widget != m_imageFrameLayout->currentWidget()) {
      m_imageFrameLayout->addWidget(widget);
      if (overlay) {
        m_imageFrameLayout->setCurrentWidget(widget);
      }
    }
  } else {
    m_tabbedDebugImages->addTab(widget, "Main");
    AutoRemovingFile file;
    QString label;
    while (!(file = debugImages->retrieveNext(&label)).get().isNull()) {
      QWidget* view = new DebugImageView(file);
      m_imageWidgetCleanup.add(view);
      m_tabbedDebugImages->addTab(view, label);
    }
    m_imageFrameLayout->addWidget(m_tabbedDebugImages.get());
  }
}

void MainWindow::removeImageWidget() {
  removeWidgetsFromLayout(m_imageFrameLayout);

  m_tabbedDebugImages->clear();
  // Delete the old widget we were owning, if any.
  m_imageWidgetCleanup.clear();
}

void MainWindow::invalidateThumbnail(const PageId& pageId) {
  m_thumbSequence->invalidateThumbnail(pageId);
}

void MainWindow::invalidateThumbnail(const PageInfo& pageInfo) {
  m_thumbSequence->invalidateThumbnail(pageInfo);
}

void MainWindow::invalidateAllThumbnails() {
  m_thumbSequence->invalidateAllThumbnails();
}

intrusive_ptr<AbstractCommand<void>> MainWindow::relinkingDialogRequester() {
  class Requester : public AbstractCommand<void> {
   public:
    explicit Requester(MainWindow* wnd) : m_wnd(wnd) {}

    virtual void operator()() {
      if (!m_wnd.isNull()) {
        m_wnd->showRelinkingDialog();
      }
    }

   private:
    QPointer<MainWindow> m_wnd;
  };
  return make_intrusive<Requester>(this);
}

void MainWindow::showRelinkingDialog() {
  if (!isProjectLoaded()) {
    return;
  }

  auto* dialog = new RelinkingDialog(m_projectFile, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);

  m_pages->listRelinkablePaths(dialog->pathCollector());
  dialog->pathCollector()(RelinkablePath(m_outFileNameGen.outDir(), RelinkablePath::Dir));

  connect(dialog, &QDialog::accepted, [this, dialog]() { this->performRelinking(dialog->relinker()); });

  dialog->show();
}

void MainWindow::performRelinking(const intrusive_ptr<AbstractRelinker>& relinker) {
  assert(relinker);

  if (!isProjectLoaded()) {
    return;
  }

  m_pages->performRelinking(*relinker);
  m_stages->performRelinking(*relinker);
  m_outFileNameGen.performRelinking(*relinker);

  Utils::maybeCreateCacheDir(m_outFileNameGen.outDir());

  m_thumbnailCache->setThumbDir(Utils::outputDirToThumbDir(m_outFileNameGen.outDir()));
  resetThumbSequence(currentPageOrderProvider());
  m_selectedPage.set(m_thumbSequence->selectionLeader().id(), getCurrentView());

  reloadRequested();
}

void MainWindow::goFirstPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo firstPage(m_thumbSequence->firstPage());
  if (!firstPage.isNull()) {
    goToPage(firstPage.id());
  }
}

void MainWindow::goLastPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo lastPage(m_thumbSequence->lastPage());
  if (!lastPage.isNull()) {
    goToPage(lastPage.id());
  }
}

void MainWindow::goNextPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo nextPage(m_thumbSequence->nextPage(m_thumbSequence->selectionLeader().id()));
  if (!nextPage.isNull()) {
    goToPage(nextPage.id());
  }
}

void MainWindow::goPrevPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo prevPage(m_thumbSequence->prevPage(m_thumbSequence->selectionLeader().id()));
  if (!prevPage.isNull()) {
    goToPage(prevPage.id());
  }
}

void MainWindow::goNextSelectedPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo nextSelectedPage(m_thumbSequence->nextSelectedPage(m_thumbSequence->selectionLeader().id()));
  if (!nextSelectedPage.isNull()) {
    goToPage(nextSelectedPage.id(), ThumbnailSequence::KEEP_SELECTION);
  }
}

void MainWindow::goPrevSelectedPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo prevSelectedPage(m_thumbSequence->prevSelectedPage(m_thumbSequence->selectionLeader().id()));
  if (!prevSelectedPage.isNull()) {
    goToPage(prevSelectedPage.id(), ThumbnailSequence::KEEP_SELECTION);
  }
}

void MainWindow::goToPage(const PageId& pageId, const ThumbnailSequence::SelectionAction selectionAction) {
  focusButton->setChecked(true);

  m_thumbSequence->setSelection(pageId, selectionAction);

  // If the page was already selected, it will be reloaded.
  // That's by design.
  updateMainArea();
}

void MainWindow::currentPageChanged(const PageInfo& pageInfo,
                                    const QRectF& thumbRect,
                                    const ThumbnailSequence::SelectionFlags flags) {
  m_selectedPage.set(pageInfo.id(), getCurrentView());

  if ((flags & ThumbnailSequence::SELECTED_BY_USER) || focusButton->isChecked()) {
    if (!(flags & ThumbnailSequence::AVOID_SCROLLING_TO)) {
      thumbView->ensureVisible(thumbRect, 0, 0);
    }
  }

  if (flags & ThumbnailSequence::SELECTED_BY_USER) {
    if (isBatchProcessingInProgress()) {
      stopBatchProcessing();
    } else if (!(flags & ThumbnailSequence::REDUNDANT_SELECTION)) {
      // Start loading / processing the newly selected page.
      updateMainArea();
    }
  }

  if ((flags & ThumbnailSequence::SELECTION_CLEARED) && selectionModeBtn->isChecked()) {
    selectionModeBtn->setChecked(false);
  }

  updateAutoSaveTimer();
}

void MainWindow::autoSaveProject() {
  if (m_projectFile.isEmpty()) {
    return;
  }
  if (!ApplicationSettings::getInstance().isAutoSaveProjectEnabled()) {
    return;
  }

  saveProjectWithFeedback(m_projectFile);
}

void MainWindow::pageContextMenuRequested(const PageInfo& pageInfo_, const QPoint& screenPos, bool selected) {
  if (isBatchProcessingInProgress()) {
    return;
  }
  // Make a copy to prevent it from being invalidated.
  const PageInfo pageInfo = pageInfo_;

  if (!selected) {
    goToPage(pageInfo.id());
  }

  QMenu menu;

  auto& iconProvider = IconProvider::getInstance();
  QAction* insBefore = menu.addAction(iconProvider.getIcon("insert-before"), tr("Insert before ..."));
  QAction* insAfter = menu.addAction(iconProvider.getIcon("insert-after"), tr("Insert after ..."));

  menu.addSeparator();

  QAction* remove = menu.addAction(iconProvider.getIcon("user-trash"), tr("Remove from project ..."));

  QAction* action = menu.exec(screenPos);
  if (action == insBefore) {
    showInsertFileDialog(BEFORE, pageInfo.imageId());
  } else if (action == insAfter) {
    showInsertFileDialog(AFTER, pageInfo.imageId());
  } else if (action == remove) {
    showRemovePagesDialog(m_thumbSequence->selectedItems());
  }
}  // MainWindow::pageContextMenuRequested

void MainWindow::pastLastPageContextMenuRequested(const QPoint& screenPos) {
  if (!isProjectLoaded()) {
    return;
  }

  QMenu menu;
  menu.addAction(IconProvider::getInstance().getIcon("insert-here"), tr("Insert here ..."));

  if (menu.exec(screenPos)) {
    showInsertFileDialog(BEFORE, ImageId());
  }
}

void MainWindow::thumbViewFocusToggled(const bool checked) {
  const QRectF rect(m_thumbSequence->selectionLeaderSceneRect());
  if (rect.isNull()) {
    // No selected items.
    return;
  }

  if (checked) {
    thumbView->ensureVisible(rect, 0, 0);
  }
}

void MainWindow::thumbViewScrolled() {
  const QRectF rect(m_thumbSequence->selectionLeaderSceneRect());
  if (rect.isNull()) {
    // No items selected.
    return;
  }

  const QRectF viewportRect(thumbView->viewport()->rect());
  const QRectF viewportItemRect(thumbView->viewportTransform().mapRect(rect));

  const double intersectionThreshold = 0.5;
  if ((viewportItemRect.top() >= viewportRect.top())
      && (viewportItemRect.top() + viewportItemRect.height() * intersectionThreshold <= viewportRect.bottom())) {
    // Item is visible.
  } else if ((viewportItemRect.bottom() <= viewportRect.bottom())
             && (viewportItemRect.bottom() - viewportItemRect.height() * intersectionThreshold >= viewportRect.top())) {
    // Item is visible.
  } else {
    focusButton->setChecked(false);
  }
}

void MainWindow::filterSelectionChanged(const QItemSelection& selected) {
  if (m_ignoreSelectionChanges) {
    return;
  }

  if (selected.empty()) {
    return;
  }

  m_interactiveQueue->cancelAndClear();
  if (m_batchQueue) {
    // Should not happen, but just in case.
    m_batchQueue->cancelAndClear();
  }

  const bool wasBelowFixOrientation = isBelowFixOrientation(m_curFilter);
  const bool wasBelowSelectContent = isBelowSelectContent(m_curFilter);
  m_curFilter = selected.front().top();
  const bool nowBelowFixOrientation = isBelowFixOrientation(m_curFilter);
  const bool nowBelowSelectContent = isBelowSelectContent(m_curFilter);

  m_stages->filterAt(m_curFilter)->selected();

  updateSortOptions();

  // Propagate context boxes down the stage list, if necessary.
  if (!wasBelowSelectContent && nowBelowSelectContent) {
    // IMPORTANT: this needs to go before resetting thumbnails,
    // because it may affect them.
    if (m_contentBoxPropagator) {
      m_contentBoxPropagator->propagate(*m_pages);
    }  // Otherwise probably no project is loaded.
  }
  // Propagate page orientations (that might have changed) to the "Split Pages" stage.
  if (!wasBelowFixOrientation && nowBelowFixOrientation) {
    // IMPORTANT: this needs to go before resetting thumbnails,
    // because it may affect them.
    if (m_pageOrientationPropagator) {
      m_pageOrientationPropagator->propagate(*m_pages);
    }  // Otherwise probably no project is loaded.
  }

  const int horScrollBarPos = thumbView->horizontalScrollBar()->value();
  const int verScrollBarPos = thumbView->verticalScrollBar()->value();

  resetThumbSequence(currentPageOrderProvider(), ThumbnailSequence::KEEP_SELECTION);

  if (!focusButton->isChecked()) {
    thumbView->horizontalScrollBar()->setValue(horScrollBarPos);
    thumbView->verticalScrollBar()->setValue(verScrollBarPos);
  }

  // load default settings for all the pages
  for (const PageInfo& pageInfo : m_thumbSequence->toPageSequence()) {
    for (int i = 0; i < m_stages->count(); i++) {
      m_stages->filterAt(i)->loadDefaultSettings(pageInfo);
    }
  }

  updateMainArea();
}  // MainWindow::filterSelectionChanged

void MainWindow::switchFilter1() {
  filterList->selectRow(0);
}

void MainWindow::switchFilter2() {
  filterList->selectRow(1);
}

void MainWindow::switchFilter3() {
  filterList->selectRow(2);
}

void MainWindow::switchFilter4() {
  filterList->selectRow(3);
}

void MainWindow::switchFilter5() {
  filterList->selectRow(4);
}

void MainWindow::switchFilter6() {
  filterList->selectRow(5);
}

void MainWindow::pageOrderingChanged(int idx) {
  if (m_ignorePageOrderingChanges) {
    return;
  }

  const int horScrollBarPos = thumbView->horizontalScrollBar()->value();
  const int verScrollBarPos = thumbView->verticalScrollBar()->value();

  m_stages->filterAt(m_curFilter)->selectPageOrder(idx);

  m_thumbSequence->reset(currentPageSequence(), ThumbnailSequence::KEEP_SELECTION, currentPageOrderProvider());

  if (!focusButton->isChecked()) {
    thumbView->horizontalScrollBar()->setValue(horScrollBarPos);
    thumbView->verticalScrollBar()->setValue(verScrollBarPos);
  }
}

void MainWindow::reloadRequested() {
  // Start loading / processing the current page.
  updateMainArea();
}

void MainWindow::startBatchProcessing() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  m_interactiveQueue->cancelAndClear();

  m_batchQueue = std::make_unique<ProcessingTaskQueue>();
  PageInfo page(m_thumbSequence->selectionLeader());
  for (; !page.isNull(); page = m_thumbSequence->nextPage(page.id())) {
    for (int i = 0; i < m_stages->count(); i++) {
      m_stages->filterAt(i)->loadDefaultSettings(page);
    }
    m_batchQueue->addProcessingTask(page, createCompositeTask(page, m_curFilter, /*batch=*/true, m_debug));
  }

  focusButton->setChecked(true);

  removeFilterOptionsWidget();
  filterList->setBatchProcessingInProgress(true);
  filterList->setEnabled(false);

  BackgroundTaskPtr task(m_batchQueue->takeForProcessing());
  if (task) {
    do {
      m_workerThreadPool->submitTask(task);
      if (!m_workerThreadPool->hasSpareCapacity()) {
        break;
      }
    } while ((task = m_batchQueue->takeForProcessing()));
  } else {
    stopBatchProcessing();
  }

  page = m_batchQueue->selectedPage();
  if (!page.isNull()) {
    m_thumbSequence->setSelection(page.id());
  }
  // Display the batch processing screen.
  updateMainArea();
}  // MainWindow::startBatchProcessing

void MainWindow::stopBatchProcessing(MainAreaAction mainArea) {
  if (!isBatchProcessingInProgress()) {
    return;
  }

  const PageInfo page(m_batchQueue->selectedPage());
  if (!page.isNull()) {
    m_thumbSequence->setSelection(page.id());
  }

  m_batchQueue->cancelAndClear();
  m_batchQueue.reset();

  filterList->setBatchProcessingInProgress(false);
  filterList->setEnabled(true);

  switch (mainArea) {
    case UPDATE_MAIN_AREA:
      updateMainArea();
      break;
    case CLEAR_MAIN_AREA:
      removeImageWidget();
      break;
  }

  resetThumbSequence(currentPageOrderProvider());
}

void MainWindow::filterResult(const BackgroundTaskPtr& task, const FilterResultPtr& result) {
  // Cancelled or not, we must mark it as finished.
  m_interactiveQueue->processingFinished(task);
  if (m_batchQueue) {
    m_batchQueue->processingFinished(task);
  }

  if (task->isCancelled()) {
    return;
  }

  if (!isBatchProcessingInProgress()) {
    if (!result->filter()) {
      // Error loading file.  No special action is necessary.
    } else if (result->filter() != m_stages->filterAt(m_curFilter)) {
      // Error from one of the previous filters.
      const int idx = m_stages->findFilter(result->filter());
      assert(idx >= 0);
      m_curFilter = idx;

      ScopedIncDec<int> selectionGuard(m_ignoreSelectionChanges);
      filterList->selectRow(idx);
    }
  }

  // This needs to be done even if batch processing is taking place,
  // for instance because thumbnail invalidation is done from here.
  result->updateUI(this);

  if (isBatchProcessingInProgress()) {
    if (m_batchQueue->allProcessed()) {
      stopBatchProcessing();

      QApplication::alert(this);  // Flash the taskbar entry.
      if (m_checkBeepWhenFinished()) {
#if defined(Q_OS_UNIX)
        QString extPlayCmd("play /usr/share/sounds/freedesktop/stereo/bell.oga");
#else
        QString extPlayCmd;
#endif
        QSettings settings;
        QString cmd = settings.value("main_window/external_alarm_cmd", extPlayCmd).toString();
        if (cmd.isEmpty()) {
          QApplication::beep();
        } else {
          Q_UNUSED(std::system(cmd.toStdString().c_str()));
        }
      }

      if (m_selectedPage.get(getCurrentView()) == m_thumbSequence->lastPage().id()) {
        // If batch processing finished at the last page, jump to the first one.
        goFirstPage();
      }
      return;
    }

    do {
      const BackgroundTaskPtr task(m_batchQueue->takeForProcessing());
      if (!task) {
        break;
      }
      m_workerThreadPool->submitTask(task);
    } while (m_workerThreadPool->hasSpareCapacity());

    const PageInfo page(m_batchQueue->selectedPage());
    if (!page.isNull()) {
      m_thumbSequence->setSelection(page.id());
    }
  }
}  // MainWindow::filterResult

void MainWindow::debugToggled(const bool enabled) {
  m_debug = enabled;
}

void MainWindow::fixDpiDialogRequested() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  assert(!m_fixDpiDialog);
  m_fixDpiDialog = new FixDpiDialog(m_pages->toImageFileInfo(), this);
  m_fixDpiDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_fixDpiDialog->setWindowModality(Qt::WindowModal);

  connect(m_fixDpiDialog, SIGNAL(accepted()), SLOT(fixedDpiSubmitted()));

  m_fixDpiDialog->show();
}

void MainWindow::fixedDpiSubmitted() {
  assert(m_fixDpiDialog);
  assert(m_pages);
  assert(m_thumbSequence);

  const PageInfo selectedPageBefore(m_thumbSequence->selectionLeader());

  m_pages->updateMetadataFrom(m_fixDpiDialog->files());

  // The thumbnail list also stores page metadata, including the DPI.
  m_thumbSequence->reset(currentPageSequence(), ThumbnailSequence::KEEP_SELECTION,
                         m_thumbSequence->pageOrderProvider());

  const PageInfo selectedPageAfter(m_thumbSequence->selectionLeader());

  // Reload if the current page was affected.
  // Note that imageId() isn't supposed to change - we check just in case.
  if ((selectedPageBefore.imageId() != selectedPageAfter.imageId())
      || (selectedPageBefore.metadata() != selectedPageAfter.metadata())) {
    reloadRequested();
  }
}

void MainWindow::saveProjectTriggered() {
  if (m_projectFile.isEmpty()) {
    saveProjectAsTriggered();
    return;
  }

  if (saveProjectWithFeedback(m_projectFile)) {
    updateWindowTitle();
  }
}

void MainWindow::saveProjectAsTriggered() {
  // XXX: this function is duplicated in OutOfMemoryDialog.

  QString projectDir;
  if (!m_projectFile.isEmpty()) {
    projectDir = QFileInfo(m_projectFile).absolutePath();
  } else {
    QSettings settings;
    projectDir = settings.value("project/lastDir").toString();
  }

  QString projectFile(
      QFileDialog::getSaveFileName(this, QString(), projectDir, tr("Scan Tailor Projects") + " (*.ScanTailor)"));
  if (projectFile.isEmpty()) {
    return;
  }

  if (!projectFile.endsWith(".ScanTailor", Qt::CaseInsensitive)) {
    projectFile += ".ScanTailor";
  }

  if (saveProjectWithFeedback(projectFile)) {
    m_projectFile = projectFile;
    updateWindowTitle();

    QSettings settings;
    settings.setValue("project/lastDir", QFileInfo(m_projectFile).absolutePath());

    RecentProjects rp;
    rp.read();
    rp.setMostRecent(m_projectFile);
    rp.write();
  }
}  // MainWindow::saveProjectAsTriggered

void MainWindow::newProject() {
  if (!closeProjectInteractive()) {
    return;
  }

  // It will delete itself when it's done.
  auto* context = new ProjectCreationContext(this);
  connect(context, SIGNAL(done(ProjectCreationContext*)), this, SLOT(newProjectCreated(ProjectCreationContext*)));
}

void MainWindow::newProjectCreated(ProjectCreationContext* context) {
  auto pages = make_intrusive<ProjectPages>(context->files(), ProjectPages::AUTO_PAGES, context->layoutDirection());
  switchToNewProject(pages, context->outDir());
}

void MainWindow::openProject() {
  if (!closeProjectInteractive()) {
    return;
  }

  const QString projectDir(QSettings().value("project/lastDir").toString());
  const QString projectFile(QFileDialog::getOpenFileName(this, tr("Open Project"), projectDir,
                                                         tr("Scan Tailor Projects") + " (*.ScanTailor)"));
  if (projectFile.isEmpty()) {
    // Cancelled by user.
    return;
  }

  openProject(projectFile);
}

void MainWindow::openProject(const QString& projectFile) {
  QFile file(projectFile);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(this, tr("Error"), tr("Unable to open the project file."));
    return;
  }

  QDomDocument doc;
  if (!doc.setContent(&file)) {
    QMessageBox::warning(this, tr("Error"), tr("The project file is broken."));
    return;
  }

  file.close();

  auto* context = new ProjectOpeningContext(this, projectFile, doc);
  connect(context, SIGNAL(done(ProjectOpeningContext*)), SLOT(projectOpened(ProjectOpeningContext*)));
  context->proceed();
}

void MainWindow::projectOpened(ProjectOpeningContext* context) {
  RecentProjects rp;
  rp.read();
  rp.setMostRecent(context->projectFile());
  rp.write();

  QSettings().setValue("project/lastDir", QFileInfo(context->projectFile()).absolutePath());

  switchToNewProject(context->projectReader()->pages(), context->projectReader()->outputDirectory(),
                     context->projectFile(), context->projectReader());
}

void MainWindow::closeProject() {
  closeProjectInteractive();
}

void MainWindow::openSettingsDialog() {
  auto* dialog = new SettingsDialog(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);
  connect(dialog, SIGNAL(settingsChanged()), this, SLOT(onSettingsChanged()));
  dialog->show();
}

void MainWindow::openDefaultParamsDialog() {
  auto* dialog = new DefaultParamsDialog(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);
  dialog->show();
}

void MainWindow::onSettingsChanged() {
  ApplicationSettings& settings = ApplicationSettings::getInstance();
  bool needInvalidate = true;

  static_cast<Application*>(qApp)->installLanguage(settings.getLanguage());

  if (m_thumbnailCache) {
    const QSize maxThumbSize = settings.getThumbnailQuality();
    if (m_thumbnailCache->getMaxThumbSize() != maxThumbSize) {
      m_thumbnailCache->setMaxThumbSize(maxThumbSize);
    }
  }

  updateThumbnailViewMode();

  const QSizeF maxLogicalThumbSize = settings.getMaxLogicalThumbnailSize();
  if (m_maxLogicalThumbSize != maxLogicalThumbSize) {
    m_maxLogicalThumbSize = maxLogicalThumbSize;
    updateMaxLogicalThumbSize();
    needInvalidate = false;
  }

  if (needInvalidate) {
    m_thumbSequence->invalidateAllThumbnails();
  }
}

void MainWindow::showAboutDialog() {
  Ui::AboutDialog ui;
  auto* dialog = new QDialog(this);
  ui.setupUi(dialog);
  ui.version->setText(QString(tr("version ")) + QString::fromUtf8(VERSION));

  QResource license(":/GPLv3.html");
  ui.licenseViewer->setHtml(QString::fromUtf8((const char*) license.data(), static_cast<int>(license.size())));

  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);
  dialog->show();
}

/**
 * This function is called asynchronously, always from the main thread.
 */
void MainWindow::handleOutOfMemorySituation() {
  deleteLater();

  m_outOfMemoryDialog->setParams(m_projectFile, m_stages, m_pages, m_selectedPage, m_outFileNameGen);

  closeProjectWithoutSaving();

  m_outOfMemoryDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_outOfMemoryDialog.release()->show();
}

/**
 * Note: the removed widgets are not deleted.
 */
void MainWindow::removeWidgetsFromLayout(QLayout* layout) {
  QLayoutItem* child;
  while ((child = layout->takeAt(0))) {
    delete child;
  }
}

void MainWindow::removeFilterOptionsWidget() {
  removeWidgetsFromLayout(m_optionsFrameLayout);
  // Delete the old widget we were owning, if any.
  m_optionsWidgetCleanup.clear();

  m_optionsWidget = nullptr;
}

void MainWindow::updateProjectActions() {
  const bool loaded = isProjectLoaded();
  actionSaveProject->setEnabled(loaded);
  actionSaveProjectAs->setEnabled(loaded);
  actionFixDpi->setEnabled(loaded);
  actionRelinking->setEnabled(loaded);
}

bool MainWindow::isBatchProcessingInProgress() const {
  return m_batchQueue != nullptr;
}

bool MainWindow::isProjectLoaded() const {
  return !m_outFileNameGen.outDir().isEmpty();
}

bool MainWindow::isBelowSelectContent() const {
  return isBelowSelectContent(m_curFilter);
}

bool MainWindow::isBelowSelectContent(const int filterIdx) const {
  return filterIdx > m_stages->selectContentFilterIdx();
}

bool MainWindow::isBelowFixOrientation(int filterIdx) const {
  return filterIdx > m_stages->fixOrientationFilterIdx();
}

bool MainWindow::isOutputFilter() const {
  return isOutputFilter(m_curFilter);
}

bool MainWindow::isOutputFilter(const int filterIdx) const {
  return filterIdx == m_stages->outputFilterIdx();
}

PageView MainWindow::getCurrentView() const {
  return m_stages->filterAt(m_curFilter)->getView();
}

void MainWindow::updateMainArea() {
  if (m_pages->numImages() == 0) {
    filterList->setBatchProcessingPossible(false);
    setDockWidgetsVisible(false);
    showNewOpenProjectPanel();
    m_statusBarPanel->clear();
  } else if (isBatchProcessingInProgress()) {
    filterList->setBatchProcessingPossible(false);
    setImageWidget(m_batchProcessingWidget.get(), KEEP_OWNERSHIP);
  } else {
    setDockWidgetsVisible(true);
    const PageInfo page(m_thumbSequence->selectionLeader());
    if (page.isNull()) {
      filterList->setBatchProcessingPossible(false);
      removeImageWidget();
      removeFilterOptionsWidget();
    } else {
      // Note that loadPageInteractive may reset it to false.
      filterList->setBatchProcessingPossible(true);
      PageSequence pageSequence = m_thumbSequence->toPageSequence();
      if (pageSequence.numPages() > 0) {
        m_statusBarPanel->updatePage(pageSequence.pageNo(page.id()) + 1, pageSequence.numPages(), page.id());
      }
      loadPageInteractive(page);
    }
  }
}

bool MainWindow::checkReadyForOutput(const PageId* ignore) const {
  return m_stages->pageLayoutFilter()->checkReadyForOutput(*m_pages, ignore);
}

void MainWindow::loadPageInteractive(const PageInfo& page) {
  assert(!isBatchProcessingInProgress());

  m_interactiveQueue->cancelAndClear();

  if (isOutputFilter() && !checkReadyForOutput(&page.id())) {
    filterList->setBatchProcessingPossible(false);

    const QString errText(
        tr("Output is not yet possible, as the final size"
           " of pages is not yet known.\nTo determine it,"
           " run batch processing at \"Select Content\" or"
           " \"Margins\"."));

    removeFilterOptionsWidget();
    setImageWidget(new ErrorWidget(errText), TRANSFER_OWNERSHIP);
    return;
  }

  for (int i = 0; i < m_stages->count(); i++) {
    m_stages->filterAt(i)->loadDefaultSettings(page);
  }

  if (!isBatchProcessingInProgress()) {
    if (m_imageFrameLayout->indexOf(m_processingIndicationWidget.get()) != -1) {
      m_processingIndicationWidget->processingRestartedEffect();
    }
    bool currentWidgetIsImage = (Utils::castOrFindChild<ImageViewBase*>(m_imageFrameLayout->widget(0)) != nullptr);
    setImageWidget(m_processingIndicationWidget.get(), KEEP_OWNERSHIP, nullptr, currentWidgetIsImage);
    m_stages->filterAt(m_curFilter)->preUpdateUI(this, page);
  }

  assert(m_thumbnailCache);

  m_interactiveQueue->cancelAndClear();
  m_interactiveQueue->addProcessingTask(page, createCompositeTask(page, m_curFilter, false, m_debug));
  m_workerThreadPool->submitTask(m_interactiveQueue->takeForProcessing());
}  // MainWindow::loadPageInteractive

void MainWindow::updateWindowTitle() {
  QString projectName;

  if (m_projectFile.isEmpty()) {
    projectName = tr("Unnamed");
  } else {
    projectName = QFileInfo(m_projectFile).completeBaseName();
  }
  const QString version(QString::fromUtf8(VERSION));
  setWindowTitle(tr("%2 - ScanTailor Advanced [%1bit]").arg(sizeof(void*) * 8).arg(projectName));
}

/**
 * \brief Closes the currently project, prompting to save it if necessary.
 *
 * \return true if the project was closed, false if the user cancelled the process.
 */
bool MainWindow::closeProjectInteractive() {
  if (!isProjectLoaded()) {
    return true;
  }

  if (m_projectFile.isEmpty()) {
    switch (promptProjectSave()) {
      case SAVE:
        saveProjectTriggered();
        // fall through
      case DONT_SAVE:
        break;
      case CANCEL:
        return false;
    }
    closeProjectWithoutSaving();
    return true;
  }

  const QFileInfo projectFile(m_projectFile);
  const QFileInfo backupFile(projectFile.absoluteDir(), QString::fromLatin1("Backup.") + projectFile.fileName());
  const QString backupFilePath(backupFile.absoluteFilePath());

  ProjectWriter writer(m_pages, m_selectedPage, m_outFileNameGen);

  if (!writer.write(backupFilePath, m_stages->filters())) {
    // Backup file could not be written???
    QFile::remove(backupFilePath);
    switch (promptProjectSave()) {
      case SAVE:
        saveProjectTriggered();
        // fall through
      case DONT_SAVE:
        break;
      case CANCEL:
        return false;
    }
    closeProjectWithoutSaving();
    return true;
  }

  if (compareFiles(m_projectFile, backupFilePath)) {
    // The project hasn't really changed.
    QFile::remove(backupFilePath);
    closeProjectWithoutSaving();
    return true;
  }

  switch (promptProjectSave()) {
    case SAVE:
      if (!Utils::overwritingRename(backupFilePath, m_projectFile)) {
        QMessageBox::warning(this, tr("Error"), tr("Error saving the project file!"));
        return false;
      }
      // fall through
    case DONT_SAVE:
      QFile::remove(backupFilePath);
      break;
    case CANCEL:
      return false;
  }

  closeProjectWithoutSaving();
  return true;
}  // MainWindow::closeProjectInteractive

void MainWindow::closeProjectWithoutSaving() {
  auto pages = make_intrusive<ProjectPages>();
  switchToNewProject(pages, QString());
}

bool MainWindow::saveProjectWithFeedback(const QString& projectFile) {
  ProjectWriter writer(m_pages, m_selectedPage, m_outFileNameGen);

  if (!writer.write(projectFile, m_stages->filters())) {
    QMessageBox::warning(this, tr("Error"), tr("Error saving the project file!"));
    return false;
  }
  return true;
}

/**
 * Note: showInsertFileDialog(BEFORE, ImageId()) is legal and means inserting at the end.
 */
void MainWindow::showInsertFileDialog(BeforeOrAfter beforeOrAfter, const ImageId& existing) {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }
  // We need to filter out files already in project.
  class ProxyModel : public QSortFilterProxyModel {
   public:
    explicit ProxyModel(const ProjectPages& pages) {
      setDynamicSortFilter(true);

      const PageSequence sequence(pages.toPageSequence(IMAGE_VIEW));
      for (const PageInfo& page : sequence) {
        m_inProjectFiles.push_back(QFileInfo(page.imageId().filePath()));
      }
    }

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override {
      const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
      const QVariant data(idx.data(QFileSystemModel::FilePathRole));
      if (data.isNull()) {
        return true;
      }
      return !m_inProjectFiles.contains(QFileInfo(data.toString()));
    }

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override { return left.row() < right.row(); }

   private:
    QFileInfoList m_inProjectFiles;
  };


  auto dialog
      = std::make_unique<QFileDialog>(this, tr("Files to insert"), QFileInfo(existing.filePath()).absolutePath());
  dialog->setFileMode(QFileDialog::ExistingFiles);
  dialog->setProxyModel(new ProxyModel(*m_pages));
  dialog->setNameFilter(tr("Images not in project (%1)").arg("*.png *.tiff *.tif *.jpeg *.jpg"));
  // XXX: Adding individual pages from a multi-page TIFF where
  // some of the pages are already in project is not supported right now.
  if (dialog->exec() != QDialog::Accepted) {
    return;
  }

  QStringList files(dialog->selectedFiles());
  if (files.empty()) {
    return;
  }

  // The order of items returned by QFileDialog is platform-dependent,
  // so we enforce our own ordering.
  std::sort(files.begin(), files.end(), SmartFilenameOrdering());

  // I suspect on some platforms it may be possible to select the same file twice,
  // so to be safe, remove duplicates.
  files.erase(std::unique(files.begin(), files.end()), files.end());


  std::vector<ImageFileInfo> newFiles;
  std::vector<QString> loadedFiles;
  std::vector<QString> failedFiles;  // Those we failed to read metadata from.
  // dialog->selectedFiles() returns file list in reverse order.
  for (int i = files.size() - 1; i >= 0; --i) {
    const QFileInfo fileInfo(files[i]);
    ImageFileInfo imageFileInfo(fileInfo, std::vector<ImageMetadata>());

    const ImageMetadataLoader::Status status = ImageMetadataLoader::load(
        files.at(i), [&](const ImageMetadata& metadata) { imageFileInfo.imageInfo().push_back(metadata); });

    if (status == ImageMetadataLoader::LOADED) {
      newFiles.push_back(imageFileInfo);
      loadedFiles.push_back(fileInfo.absoluteFilePath());
    } else {
      failedFiles.push_back(fileInfo.absoluteFilePath());
    }
  }

  if (!failedFiles.empty()) {
    auto errDialog = std::make_unique<LoadFilesStatusDialog>(this);
    errDialog->setLoadedFiles(loadedFiles);
    errDialog->setFailedFiles(failedFiles);
    errDialog->setOkButtonName(QString(" %1 ").arg(tr("Skip failed files")));
    if ((errDialog->exec() != QDialog::Accepted) || loadedFiles.empty()) {
      return;
    }
  }

  // Check if there is at least one DPI that's not OK.
  if (std::find_if(newFiles.begin(), newFiles.end(), [&](const ImageFileInfo& p) -> bool { return !p.isDpiOK(); })
      != newFiles.end()) {
    auto dpiDialog = std::make_unique<FixDpiDialog>(newFiles, this);
    dpiDialog->setWindowModality(Qt::WindowModal);
    if (dpiDialog->exec() != QDialog::Accepted) {
      return;
    }

    newFiles = dpiDialog->files();
  }

  // Actually insert the new pages.
  for (const ImageFileInfo& file : newFiles) {
    int imageNum = -1;  // Zero-based image number in a multi-page TIFF.
    for (const ImageMetadata& metadata : file.imageInfo()) {
      ++imageNum;

      const int numSubPages = ProjectPages::adviseNumberOfLogicalPages(metadata, OrthogonalRotation());
      const ImageInfo imageInfo(ImageId(file.fileInfo(), imageNum), metadata, numSubPages, false, false);
      insertImage(imageInfo, beforeOrAfter, existing);
    }
  }
}  // MainWindow::showInsertFileDialog

void MainWindow::showRemovePagesDialog(const std::set<PageId>& pages) {
  auto dialog = std::make_unique<QDialog>(this);
  Ui::RemovePagesDialog ui;
  ui.setupUi(dialog.get());
  ui.icon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(48, 48));

  ui.text->setText(ui.text->text().arg(pages.size()));

  QPushButton* removeBtn = ui.buttonBox->button(QDialogButtonBox::Ok);
  removeBtn->setText(tr("Remove"));

  dialog->setWindowModality(Qt::WindowModal);
  if (dialog->exec() == QDialog::Accepted) {
    removeFromProject(pages);
    eraseOutputFiles(pages);
  }
}

/**
 * Note: insertImage(..., BEFORE, ImageId()) is legal and means inserting at the end.
 */
void MainWindow::insertImage(const ImageInfo& newImage, BeforeOrAfter beforeOrAfter, ImageId existing) {
  std::vector<PageInfo> pages(m_pages->insertImage(newImage, beforeOrAfter, existing, getCurrentView()));

  if (beforeOrAfter == BEFORE) {
    // The second one will be inserted first, then the first
    // one will be inserted BEFORE the second one.
    std::reverse(pages.begin(), pages.end());
  }

  for (const PageInfo& pageInfo : pages) {
    m_outFileNameGen.disambiguator()->registerFile(pageInfo.imageId().filePath());
    m_thumbSequence->insert(pageInfo, beforeOrAfter, existing);
    existing = pageInfo.imageId();
  }
}

void MainWindow::removeFromProject(const std::set<PageId>& pages) {
  m_interactiveQueue->cancelAndRemove(pages);
  if (m_batchQueue) {
    m_batchQueue->cancelAndRemove(pages);
  }

  m_pages->removePages(pages);


  const PageSequence itemsInOrder = m_thumbSequence->toPageSequence();
  std::set<PageId> newSelection;

  bool selectFirstNonDeleted = false;
  if (itemsInOrder.numPages() > 0) {
    // if first page was deleted select first not deleted page
    // otherwise select last not deleted page from beginning
    selectFirstNonDeleted = pages.find(itemsInOrder.pageAt(0).id()) != pages.end();

    PageId lastNonDeleted;
    for (const PageInfo& page : itemsInOrder) {
      const PageId& id = page.id();
      const bool wasDeleted = (pages.find(id) != pages.end());

      if (!wasDeleted) {
        if (selectFirstNonDeleted) {
          m_thumbSequence->setSelection(id);
          newSelection.insert(id);
          break;
        } else {
          lastNonDeleted = id;
        }
      } else if (!selectFirstNonDeleted && !lastNonDeleted.isNull()) {
        m_thumbSequence->setSelection(lastNonDeleted);
        newSelection.insert(lastNonDeleted);
        break;
      }
    }

    m_thumbSequence->removePages(pages);

    if (newSelection.empty()) {
      // fallback to old behaviour
      if (m_thumbSequence->selectionLeader().isNull()) {
        m_thumbSequence->setSelection(m_thumbSequence->firstPage().id());
      }
    }
  }

  updateMainArea();
}  // MainWindow::removeFromProject

void MainWindow::eraseOutputFiles(const std::set<PageId>& pages) {
  std::vector<PageId::SubPage> eraseVariations;
  eraseVariations.reserve(3);

  for (const PageId& pageId : pages) {
    eraseVariations.clear();
    switch (pageId.subPage()) {
      case PageId::SINGLE_PAGE:
        eraseVariations.push_back(PageId::SINGLE_PAGE);
        eraseVariations.push_back(PageId::LEFT_PAGE);
        eraseVariations.push_back(PageId::RIGHT_PAGE);
        break;
      case PageId::LEFT_PAGE:
        eraseVariations.push_back(PageId::SINGLE_PAGE);
        eraseVariations.push_back(PageId::LEFT_PAGE);
        break;
      case PageId::RIGHT_PAGE:
        eraseVariations.push_back(PageId::SINGLE_PAGE);
        eraseVariations.push_back(PageId::RIGHT_PAGE);
        break;
    }

    for (PageId::SubPage subpage : eraseVariations) {
      QFile::remove(m_outFileNameGen.filePathFor(PageId(pageId.imageId(), subpage)));
    }
  }
}

BackgroundTaskPtr MainWindow::createCompositeTask(const PageInfo& page,
                                                  const int lastFilterIdx,
                                                  const bool batch,
                                                  bool debug) {
  intrusive_ptr<fix_orientation::Task> fixOrientationTask;
  intrusive_ptr<page_split::Task> pageSplitTask;
  intrusive_ptr<deskew::Task> deskewTask;
  intrusive_ptr<select_content::Task> selectContentTask;
  intrusive_ptr<page_layout::Task> pageLayoutTask;
  intrusive_ptr<output::Task> outputTask;

  if (batch) {
    debug = false;
  }

  if (lastFilterIdx >= m_stages->outputFilterIdx()) {
    outputTask = m_stages->outputFilter()->createTask(page.id(), m_thumbnailCache, m_outFileNameGen, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->pageLayoutFilterIdx()) {
    pageLayoutTask = m_stages->pageLayoutFilter()->createTask(page.id(), outputTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->selectContentFilterIdx()) {
    selectContentTask = m_stages->selectContentFilter()->createTask(page.id(), pageLayoutTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->deskewFilterIdx()) {
    deskewTask = m_stages->deskewFilter()->createTask(page.id(), selectContentTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->pageSplitFilterIdx()) {
    pageSplitTask = m_stages->pageSplitFilter()->createTask(page, deskewTask, batch, debug);
    debug = false;
  }
  if (lastFilterIdx >= m_stages->fixOrientationFilterIdx()) {
    fixOrientationTask = m_stages->fixOrientationFilter()->createTask(page.id(), pageSplitTask, batch);
    debug = false;
  }
  assert(fixOrientationTask);
  return make_intrusive<LoadFileTask>(batch ? BackgroundTask::BATCH : BackgroundTask::INTERACTIVE, page,
                                      m_thumbnailCache, m_pages, fixOrientationTask);
}  // MainWindow::createCompositeTask

intrusive_ptr<CompositeCacheDrivenTask> MainWindow::createCompositeCacheDrivenTask(const int lastFilterIdx) {
  intrusive_ptr<fix_orientation::CacheDrivenTask> fixOrientationTask;
  intrusive_ptr<page_split::CacheDrivenTask> pageSplitTask;
  intrusive_ptr<deskew::CacheDrivenTask> deskewTask;
  intrusive_ptr<select_content::CacheDrivenTask> selectContentTask;
  intrusive_ptr<page_layout::CacheDrivenTask> pageLayoutTask;
  intrusive_ptr<output::CacheDrivenTask> outputTask;

  if (lastFilterIdx >= m_stages->outputFilterIdx()) {
    outputTask = m_stages->outputFilter()->createCacheDrivenTask(m_outFileNameGen);
  }
  if (lastFilterIdx >= m_stages->pageLayoutFilterIdx()) {
    pageLayoutTask = m_stages->pageLayoutFilter()->createCacheDrivenTask(outputTask);
  }
  if (lastFilterIdx >= m_stages->selectContentFilterIdx()) {
    selectContentTask = m_stages->selectContentFilter()->createCacheDrivenTask(pageLayoutTask);
  }
  if (lastFilterIdx >= m_stages->deskewFilterIdx()) {
    deskewTask = m_stages->deskewFilter()->createCacheDrivenTask(selectContentTask);
  }
  if (lastFilterIdx >= m_stages->pageSplitFilterIdx()) {
    pageSplitTask = m_stages->pageSplitFilter()->createCacheDrivenTask(deskewTask);
  }
  if (lastFilterIdx >= m_stages->fixOrientationFilterIdx()) {
    fixOrientationTask = m_stages->fixOrientationFilter()->createCacheDrivenTask(pageSplitTask);
  }

  assert(fixOrientationTask);
  return fixOrientationTask;
}  // MainWindow::createCompositeCacheDrivenTask

void MainWindow::updateDisambiguationRecords(const PageSequence& pages) {
  for (const PageInfo& page : pages) {
    m_outFileNameGen.disambiguator()->registerFile(page.imageId().filePath());
  }
}

PageSelectionAccessor MainWindow::newPageSelectionAccessor() {
  auto provider = make_intrusive<PageSelectionProviderImpl>(this);
  return PageSelectionAccessor(provider);
}

void MainWindow::changeEvent(QEvent* event) {
  if (event != nullptr) {
    switch (event->type()) {
      case QEvent::LanguageChange:
        retranslateUi(this);
        updateWindowTitle();
        break;
      default:
        QWidget::changeEvent(event);
        break;
    }
  }
}

void MainWindow::setDockWidgetsVisible(bool state) {
  filterDockWidget->setVisible(state);
  thumbnailsDockWidget->setVisible(state);
}

void MainWindow::scaleThumbnails(const QWheelEvent* wheelEvent) {
  const QPoint& angleDelta = wheelEvent->angleDelta();
  const int wheelDist = angleDelta.x() + angleDelta.y();

  if (std::abs(wheelDist) >= 30) {
    const double dx = std::copysign(25.0, wheelDist);
    const double dy = std::copysign(16.0, wheelDist);
    const double width = qBound(100.0, m_maxLogicalThumbSize.width() + dx, 1000.0);
    const double height = qBound(64.0, m_maxLogicalThumbSize.height() + dy, 640.0);
    m_maxLogicalThumbSize = QSizeF(width, height);
    if (!m_maxLogicalThumbSizeUpdater.isActive()) {
      m_maxLogicalThumbSizeUpdater.start(350);
    }

    ApplicationSettings::getInstance().setMaxLogicalThumbnailSize(m_maxLogicalThumbSize);
  }
}

void MainWindow::updateMaxLogicalThumbSize() {
  m_thumbSequence->setMaxLogicalThumbSize(m_maxLogicalThumbSize);
  updateThumbViewMinWidth();
  resetThumbSequence(currentPageOrderProvider(), ThumbnailSequence::KEEP_SELECTION);
}

void MainWindow::setupIcons() {
  auto& iconProvider = IconProvider::getInstance();
  focusButton->setIcon(iconProvider.getIcon("eye"));
  prevPageBtn->setIcon(iconProvider.getIcon("triangle-up-arrow"));
  nextPageBtn->setIcon(iconProvider.getIcon("triangle-down-arrow"));
  filterSelectedBtn->setIcon(iconProvider.getIcon("check-mark"));
  gotoPageBtn->setIcon(iconProvider.getIcon("right-pointing"));
  selectionModeBtn->setIcon(iconProvider.getIcon("checkbox-styled"));
  thumbColumnViewBtn->setIcon(iconProvider.getIcon("column-view"));
  sortingOrderBtn->setIcon(iconProvider.getIcon("sorting-order"));
}

void MainWindow::execGotoPageDialog() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  bool ok;
  const PageSequence pageSequence = m_thumbSequence->toPageSequence();
  const PageId& selectionLeader = m_thumbSequence->selectionLeader().id();
  int pageNumber = QInputDialog::getInt(this, tr("Go To Page"), tr("Enter the page number:"),
                                        pageSequence.pageNo(selectionLeader) + 1, 1, pageSequence.numPages(), 1, &ok);
  if (ok) {
    const PageId& newSelectionLeader = pageSequence.pageAt(pageNumber - 1).id();
    if (selectionLeader != newSelectionLeader) {
      goToPage(newSelectionLeader);
    }
  }
}

void MainWindow::updateThumbnailViewMode() {
  const ThumbnailSequence::ViewMode viewMode
      = (ApplicationSettings::getInstance().isSingleColumnThumbnailDisplayEnabled()) ? ThumbnailSequence::SINGLE_COLUMN
                                                                                     : ThumbnailSequence::MULTI_COLUMN;
  if (m_thumbSequence->getViewMode() != viewMode) {
    m_thumbSequence->setViewMode(viewMode);
  }
}

void MainWindow::updateAutoSaveTimer() {
  if (m_autoSaveTimer.remainingTime() <= 0) {
    m_autoSaveTimer.start(60000);
  }
}

PageSequence MainWindow::currentPageSequence() {
  PageSequence pageSequence = m_pages->toPageSequence(getCurrentView());
  if (sortingOrderBtn->isChecked()) {
    std::reverse(pageSequence.begin(), pageSequence.end());
  }
  return pageSequence;
}
