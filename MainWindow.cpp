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

#include "MainWindow.h"
#include <QDir>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QResource>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStackedLayout>
#include <boost/lambda/lambda.hpp>
#include "AbstractRelinker.h"
#include "Application.h"
#include "AutoRemovingFile.h"
#include "BasicImageView.h"
#include "CommandLine.h"
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

class MainWindow::PageSelectionProviderImpl : public PageSelectionProvider {
 public:
  PageSelectionProviderImpl(MainWindow* wnd) : m_ptrWnd(wnd) {}

  virtual PageSequence allPages() const { return m_ptrWnd ? m_ptrWnd->allPages() : PageSequence(); }

  virtual std::set<PageId> selectedPages() const { return m_ptrWnd ? m_ptrWnd->selectedPages() : std::set<PageId>(); }

  std::vector<PageRange> selectedRanges() const {
    return m_ptrWnd ? m_ptrWnd->selectedRanges() : std::vector<PageRange>();
  }

 private:
  QPointer<MainWindow> m_ptrWnd;
};


MainWindow::MainWindow()
    : m_ptrPages(new ProjectPages),
      m_ptrStages(new StageSequence(m_ptrPages, newPageSelectionAccessor())),
      m_ptrWorkerThreadPool(new WorkerThreadPool),
      m_ptrInteractiveQueue(new ProcessingTaskQueue()),
      m_ptrOutOfMemoryDialog(new OutOfMemoryDialog),
      m_curFilter(0),
      m_ignoreSelectionChanges(0),
      m_ignorePageOrderingChanges(0),
      m_debug(false),
      m_closing(false) {
  m_maxLogicalThumbSize = QSize(250, 160);
  m_ptrThumbSequence = std::make_unique<ThumbnailSequence>(m_maxLogicalThumbSize);

  m_thumbResizeTimer.setSingleShot(true);
  connect(&m_thumbResizeTimer, SIGNAL(timeout()), SLOT(invalidateAllThumbnails()));

  m_autoSaveTimer.setSingleShot(true);
  connect(&m_autoSaveTimer, SIGNAL(timeout()), SLOT(autoSaveProject()));

  setupUi(this);
  sortOptions->setVisible(false);

  createBatchProcessingWidget();
  m_ptrProcessingIndicationWidget.reset(new ProcessingIndicationWidget);

  filterList->setStages(m_ptrStages);
  filterList->selectRow(0);

  setupThumbView();  // Expects m_ptrThumbSequence to be initialized.
  m_ptrTabbedDebugImages.reset(new TabbedDebugImages);

  m_debug = actionDebug->isChecked();

  m_pImageFrameLayout = new QStackedLayout(imageViewFrame);
  m_pImageFrameLayout->setStackingMode(QStackedLayout::StackAll);

  m_pOptionsFrameLayout = new QStackedLayout(filterOptions);

  m_statusBarPanel = std::make_unique<StatusBarPanel>();
  QMainWindow::statusBar()->addPermanentWidget(m_statusBarPanel.get());
  connect(m_ptrThumbSequence.get(), &ThumbnailSequence::newSelectionLeader, [this](const PageInfo& page_info) {
    PageSequence pageSequence = m_ptrThumbSequence->toPageSequence();
    if (pageSequence.numPages() > 0) {
      m_statusBarPanel->updatePage(pageSequence.pageNo(page_info.id()) + 1, pageSequence.numPages(), page_info.id());
    } else {
      m_statusBarPanel->clear();
    }
  });

  m_unitsMenuActionGroup = std::make_unique<QActionGroup>(this);
  for (QAction* action : menuUnits->actions()) {
    m_unitsMenuActionGroup->addAction(action);
  }
  switch (unitsFromString(QSettings().value("settings/units", "mm").toString())) {
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
  connect(actionPixels, &QAction::toggled, [this](bool checked) {
    if (checked) {
      UnitsProvider::getInstance()->setUnits(PIXELS);
      QSettings().setValue("settings/units", unitsToString(PIXELS));
    }
  });
  connect(actionMilimeters, &QAction::toggled, [this](bool checked) {
    if (checked) {
      UnitsProvider::getInstance()->setUnits(MILLIMETRES);
      QSettings().setValue("settings/units", unitsToString(MILLIMETRES));
    }
  });
  connect(actionCentimetres, &QAction::toggled, [this](bool checked) {
    if (checked) {
      UnitsProvider::getInstance()->setUnits(CENTIMETRES);
      QSettings().setValue("settings/units", unitsToString(CENTIMETRES));
    }
  });
  connect(actionInches, &QAction::toggled, [this](bool checked) {
    if (checked) {
      UnitsProvider::getInstance()->setUnits(INCHES);
      QSettings().setValue("settings/units", unitsToString(INCHES));
    }
  });

  addAction(actionFirstPage);
  addAction(actionLastPage);
  addAction(actionNextPage);
  addAction(actionPrevPage);
  addAction(actionPrevPageQ);
  addAction(actionNextPageW);

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
  connect(actionAbout, SIGNAL(triggered(bool)), this, SLOT(showAboutDialog()));
  connect(&OutOfMemoryHandler::instance(), SIGNAL(outOfMemory()), SLOT(handleOutOfMemorySituation()));

  connect(actionSwitchFilter1, SIGNAL(triggered(bool)), SLOT(switchFilter1()));
  connect(actionSwitchFilter2, SIGNAL(triggered(bool)), SLOT(switchFilter2()));
  connect(actionSwitchFilter3, SIGNAL(triggered(bool)), SLOT(switchFilter3()));
  connect(actionSwitchFilter4, SIGNAL(triggered(bool)), SLOT(switchFilter4()));
  connect(actionSwitchFilter5, SIGNAL(triggered(bool)), SLOT(switchFilter5()));
  connect(actionSwitchFilter6, SIGNAL(triggered(bool)), SLOT(switchFilter6()));

  connect(filterList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
          SLOT(filterSelectionChanged(const QItemSelection&)));
  connect(filterList, SIGNAL(launchBatchProcessing()), this, SLOT(startBatchProcessing()));

  connect(m_ptrWorkerThreadPool.get(), SIGNAL(taskResult(const BackgroundTaskPtr&, const FilterResultPtr&)), this,
          SLOT(filterResult(const BackgroundTaskPtr&, const FilterResultPtr&)));

  connect(m_ptrThumbSequence.get(),
          SIGNAL(newSelectionLeader(const PageInfo&, const QRectF&, ThumbnailSequence::SelectionFlags)), this,
          SLOT(currentPageChanged(const PageInfo&, const QRectF&, ThumbnailSequence::SelectionFlags)));
  connect(m_ptrThumbSequence.get(), SIGNAL(pageContextMenuRequested(const PageInfo&, const QPoint&, bool)), this,
          SLOT(pageContextMenuRequested(const PageInfo&, const QPoint&, bool)));
  connect(m_ptrThumbSequence.get(), SIGNAL(pastLastPageContextMenuRequested(const QPoint&)),
          SLOT(pastLastPageContextMenuRequested(const QPoint&)));

  connect(thumbView->verticalScrollBar(), SIGNAL(sliderMoved(int)), this, SLOT(thumbViewScrolled()));
  connect(thumbView->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(thumbViewScrolled()));
  connect(focusButton, SIGNAL(clicked(bool)), this, SLOT(thumbViewFocusToggled(bool)));
  connect(sortOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(pageOrderingChanged(int)));

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

  QSettings settings;
  if (settings.value("mainWindow/maximized") == false) {
    const QVariant geom(settings.value("mainWindow/nonMaximizedGeometry"));
    if (!restoreGeometry(geom.toByteArray())) {
      resize(1014, 689);  // A sensible value.
    }
  }
  m_autoSaveProject = settings.value("settings/auto_save_project").toBool();
}

MainWindow::~MainWindow() {
  m_ptrInteractiveQueue->cancelAndClear();
  if (m_ptrBatchQueue) {
    m_ptrBatchQueue->cancelAndClear();
  }
  m_ptrWorkerThreadPool->shutdown();

  removeWidgetsFromLayout(m_pImageFrameLayout);
  removeWidgetsFromLayout(m_pOptionsFrameLayout);
  m_ptrTabbedDebugImages->clear();
}

PageSequence MainWindow::allPages() const {
  return m_ptrThumbSequence->toPageSequence();
}

std::set<PageId> MainWindow::selectedPages() const {
  return m_ptrThumbSequence->selectedItems();
}

std::vector<PageRange> MainWindow::selectedRanges() const {
  return m_ptrThumbSequence->selectedRanges();
}

void MainWindow::switchToNewProject(const intrusive_ptr<ProjectPages>& pages,
                                    const QString& out_dir,
                                    const QString& project_file_path,
                                    const ProjectReader* project_reader) {
  stopBatchProcessing(CLEAR_MAIN_AREA);
  m_ptrInteractiveQueue->cancelAndClear();

  if (!out_dir.isEmpty()) {
    Utils::maybeCreateCacheDir(out_dir);
  }
  m_ptrPages = pages;
  m_projectFile = project_file_path;

  if (project_reader) {
    m_selectedPage = project_reader->selectedPage();
  }

  intrusive_ptr<FileNameDisambiguator> disambiguator;
  if (project_reader) {
    disambiguator = project_reader->namingDisambiguator();
  } else {
    disambiguator.reset(new FileNameDisambiguator);
  }

  m_outFileNameGen = OutputFileNameGenerator(disambiguator, out_dir, pages->layoutDirection());
  // These two need to go in this order.
  updateDisambiguationRecords(pages->toPageSequence(IMAGE_VIEW));

  // Recreate the stages and load their state.
  m_ptrStages = make_intrusive<StageSequence>(pages, newPageSelectionAccessor());
  if (project_reader) {
    project_reader->readFilterSettings(m_ptrStages->filters());
  }

  // Connect the filter list model to the view and select
  // the first item.
  {
    ScopedIncDec<int> guard(m_ignoreSelectionChanges);
    filterList->setStages(m_ptrStages);
    filterList->selectRow(0);
    m_curFilter = 0;
    // Setting a data model also implicitly sets a new
    // selection model, so we have to reconnect to it.
    connect(filterList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
            SLOT(filterSelectionChanged(const QItemSelection&)));
  }

  updateSortOptions();

  m_ptrContentBoxPropagator = std::make_unique<ContentBoxPropagator>(
      m_ptrStages->pageLayoutFilter(), createCompositeCacheDrivenTask(m_ptrStages->selectContentFilterIdx()));

  m_ptrPageOrientationPropagator = std::make_unique<PageOrientationPropagator>(
      m_ptrStages->pageSplitFilter(), createCompositeCacheDrivenTask(m_ptrStages->fixOrientationFilterIdx()));

  // Thumbnails are stored relative to the output directory,
  // so recreate the thumbnail cache.
  if (out_dir.isEmpty()) {
    m_ptrThumbnailCache.reset();
  } else {
    m_ptrThumbnailCache = Utils::createThumbnailCache(m_outFileNameGen.outDir());
  }
  resetThumbSequence(currentPageOrderProvider());

  removeFilterOptionsWidget();
  updateProjectActions();
  updateWindowTitle();
  updateMainArea();

  if (!QDir(out_dir).exists()) {
    showRelinkingDialog();
  }
}  // MainWindow::switchToNewProject

void MainWindow::showNewOpenProjectPanel() {
  std::unique_ptr<QWidget> outer_widget(new QWidget);
  QGridLayout* layout = new QGridLayout(outer_widget.get());
  outer_widget->setLayout(layout);

  NewOpenProjectPanel* nop = new NewOpenProjectPanel(outer_widget.get());
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
  setImageWidget(outer_widget.release(), TRANSFER_OWNERSHIP);

  filterList->setBatchProcessingPossible(false);
}  // MainWindow::showNewOpenProjectPanel

void MainWindow::createBatchProcessingWidget() {
  m_ptrBatchProcessingWidget.reset(new QWidget);
  QGridLayout* layout = new QGridLayout(m_ptrBatchProcessingWidget.get());
  m_ptrBatchProcessingWidget->setLayout(layout);

  SkinnedButton* stop_btn = new SkinnedButton(":/icons/stop-big.png", ":/icons/stop-big-hovered.png",
                                              ":/icons/stop-big-pressed.png", m_ptrBatchProcessingWidget.get());
  stop_btn->setStatusTip(tr("Stop batch processing"));

  class LowerPanel : public QWidget {
   public:
    LowerPanel(QWidget* parent = 0) : QWidget(parent) { ui.setupUi(this); }

    Ui::BatchProcessingLowerPanel ui;
  };


  LowerPanel* lower_panel = new LowerPanel(m_ptrBatchProcessingWidget.get());
  m_checkBeepWhenFinished = [lower_panel]() { return lower_panel->ui.beepWhenFinished->isChecked(); };

  int row = 0;  // Row 0 is reserved.
  layout->addWidget(stop_btn, ++row, 1, Qt::AlignCenter);
  layout->addWidget(lower_panel, ++row, 0, 1, 3, Qt::AlignHCenter | Qt::AlignTop);
  layout->setColumnStretch(0, 1);
  layout->setColumnStretch(2, 1);
  layout->setRowStretch(0, 1);
  layout->setRowStretch(row, 1);

  connect(stop_btn, SIGNAL(clicked()), SLOT(stopBatchProcessing()));
}  // MainWindow::createBatchProcessingWidget

void MainWindow::setupThumbView() {
  const int sb = thumbView->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
  int inner_width = thumbView->maximumViewportSize().width() - sb;
  if (thumbView->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, 0, thumbView)) {
    inner_width -= thumbView->frameWidth() * 2;
  }
  const int delta_x = thumbView->size().width() - inner_width;
  thumbView->setMinimumWidth((int) std::ceil(m_maxLogicalThumbSize.width() + delta_x));

  m_ptrThumbSequence->attachView(thumbView);

  thumbView->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* ev) {
  if ((obj == thumbView) && (ev->type() == QEvent::Resize)) {
    m_thumbResizeTimer.start(200);
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

  const int chunk_size = 4096;
  for (;;) {
    const QByteArray chunk1(file1.read(chunk_size));
    const QByteArray chunk2(file2.read(chunk_size));
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

  const intrusive_ptr<AbstractFilter> filter(m_ptrStages->filterAt(m_curFilter));

  return filter->pageOrderOptions()[idx].provider();
}

void MainWindow::updateSortOptions() {
  const ScopedIncDec<int> guard(m_ignorePageOrderingChanges);

  const intrusive_ptr<AbstractFilter> filter(m_ptrStages->filterAt(m_curFilter));

  sortOptions->clear();

  for (const PageOrderOption& opt : filter->pageOrderOptions()) {
    sortOptions->addItem(opt.name());
  }

  sortOptions->setVisible(sortOptions->count() > 0);

  if (sortOptions->count() > 0) {
    sortOptions->setCurrentIndex(filter->selectedPageOrder());
  }
}

void MainWindow::resetThumbSequence(const intrusive_ptr<const PageOrderProvider>& page_order_provider) {
  if (m_ptrThumbnailCache) {
    const intrusive_ptr<CompositeCacheDrivenTask> task(createCompositeCacheDrivenTask(m_curFilter));

    m_ptrThumbSequence->setThumbnailFactory(
        make_intrusive<ThumbnailFactory>(m_ptrThumbnailCache, m_maxLogicalThumbSize, task));
  }

  m_ptrThumbSequence->reset(m_ptrPages->toPageSequence(getCurrentView()), ThumbnailSequence::RESET_SELECTION,
                            page_order_provider);

  if (!m_ptrThumbnailCache) {
    // Empty project.
    assert(m_ptrPages->numImages() == 0);
    m_ptrThumbSequence->setThumbnailFactory(nullptr);
  }

  const PageId page(m_selectedPage.get(getCurrentView()));
  if (m_ptrThumbSequence->setSelection(page)) {
    // OK
  } else if (m_ptrThumbSequence->setSelection(PageId(page.imageId(), PageId::LEFT_PAGE))) {
    // OK
  } else if (m_ptrThumbSequence->setSelection(PageId(page.imageId(), PageId::RIGHT_PAGE))) {
    // OK
  } else if (m_ptrThumbSequence->setSelection(PageId(page.imageId(), PageId::SINGLE_PAGE))) {
    // OK
  } else {
    // Last resort.
    m_ptrThumbSequence->setSelection(m_ptrThumbSequence->firstPage().id());
  }
}  // MainWindow::resetThumbSequence

void MainWindow::setOptionsWidget(FilterOptionsWidget* widget, const Ownership ownership) {
  if (isBatchProcessingInProgress()) {
    if (ownership == TRANSFER_OWNERSHIP) {
      delete widget;
    }

    return;
  }

  if (m_ptrOptionsWidget != widget) {
    removeWidgetsFromLayout(m_pOptionsFrameLayout);
  }
  // Delete the old widget we were owning, if any.
  m_optionsWidgetCleanup.clear();

  if (ownership == TRANSFER_OWNERSHIP) {
    m_optionsWidgetCleanup.add(widget);
  }

  if (m_ptrOptionsWidget == widget) {
    return;
  }

  if (m_ptrOptionsWidget) {
    disconnect(m_ptrOptionsWidget, SIGNAL(reloadRequested()), this, SLOT(reloadRequested()));
    disconnect(m_ptrOptionsWidget, SIGNAL(invalidateThumbnail(const PageId&)), this,
               SLOT(invalidateThumbnail(const PageId&)));
    disconnect(m_ptrOptionsWidget, SIGNAL(invalidateThumbnail(const PageInfo&)), this,
               SLOT(invalidateThumbnail(const PageInfo&)));
    disconnect(m_ptrOptionsWidget, SIGNAL(invalidateAllThumbnails()), this, SLOT(invalidateAllThumbnails()));
    disconnect(m_ptrOptionsWidget, SIGNAL(goToPage(const PageId&)), this, SLOT(goToPage(const PageId&)));
  }

  m_pOptionsFrameLayout->addWidget(widget);
  m_ptrOptionsWidget = widget;

  // We use an asynchronous connection here, because the slot
  // will probably delete the options panel, which could be
  // responsible for the emission of this signal.  Qt doesn't
  // like when we delete an object while it's emitting a singal.
  connect(widget, SIGNAL(reloadRequested()), this, SLOT(reloadRequested()), Qt::QueuedConnection);
  connect(widget, SIGNAL(invalidateThumbnail(const PageId&)), this, SLOT(invalidateThumbnail(const PageId&)));
  connect(widget, SIGNAL(invalidateThumbnail(const PageInfo&)), this, SLOT(invalidateThumbnail(const PageInfo&)));
  connect(widget, SIGNAL(invalidateAllThumbnails()), this, SLOT(invalidateAllThumbnails()));
  connect(widget, SIGNAL(goToPage(const PageId&)), this, SLOT(goToPage(const PageId&)));
}  // MainWindow::setOptionsWidget

void MainWindow::setImageWidget(QWidget* widget,
                                const Ownership ownership,
                                DebugImages* debug_images,
                                bool clear_image_widget) {
  if (isBatchProcessingInProgress() && (widget != m_ptrBatchProcessingWidget.get())) {
    if (ownership == TRANSFER_OWNERSHIP) {
      delete widget;
    }

    return;
  }

  QWidget* current_widget = m_pImageFrameLayout->currentWidget();
  if (dynamic_cast<ProcessingIndicationWidget*>(current_widget) != nullptr) {
    if (!clear_image_widget) {
      return;
    }
  }

  bool current_widget_is_image = (Utils::castOrFindChild<ImageViewBase*>(current_widget) != nullptr);

  if (clear_image_widget || !current_widget_is_image) {
    removeImageWidget();
  }

  if (ownership == TRANSFER_OWNERSHIP) {
    m_imageWidgetCleanup.add(widget);
  }

  if (!debug_images || debug_images->empty()) {
    m_pImageFrameLayout->addWidget(widget);
    if (!clear_image_widget && current_widget_is_image) {
      m_pImageFrameLayout->setCurrentWidget(widget);
    }
  } else {
    m_ptrTabbedDebugImages->addTab(widget, "Main");
    AutoRemovingFile file;
    QString label;
    while (!(file = debug_images->retrieveNext(&label)).get().isNull()) {
      QWidget* widget = new DebugImageView(file);
      m_imageWidgetCleanup.add(widget);
      m_ptrTabbedDebugImages->addTab(widget, label);
    }
    m_pImageFrameLayout->addWidget(m_ptrTabbedDebugImages.get());
  }
}  // MainWindow::setImageWidget

void MainWindow::removeImageWidget() {
  removeWidgetsFromLayout(m_pImageFrameLayout);

  m_ptrTabbedDebugImages->clear();
  // Delete the old widget we were owning, if any.
  m_imageWidgetCleanup.clear();
}

void MainWindow::invalidateThumbnail(const PageId& page_id) {
  m_ptrThumbSequence->invalidateThumbnail(page_id);
}

void MainWindow::invalidateThumbnail(const PageInfo& page_info) {
  m_ptrThumbSequence->invalidateThumbnail(page_info);
}

void MainWindow::invalidateAllThumbnails() {
  m_ptrThumbSequence->invalidateAllThumbnails();
}

intrusive_ptr<AbstractCommand<void>> MainWindow::relinkingDialogRequester() {
  class Requester : public AbstractCommand<void> {
   public:
    Requester(MainWindow* wnd) : m_ptrWnd(wnd) {}

    virtual void operator()() {
      if (MainWindow* wnd = m_ptrWnd) {
        wnd->showRelinkingDialog();
      }
    }

   private:
    QPointer<MainWindow> m_ptrWnd;
  };


  return make_intrusive<Requester>(this);
}

void MainWindow::showRelinkingDialog() {
  if (!isProjectLoaded()) {
    return;
  }

  RelinkingDialog* dialog = new RelinkingDialog(m_projectFile, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);

  m_ptrPages->listRelinkablePaths(dialog->pathCollector());
  dialog->pathCollector()(RelinkablePath(m_outFileNameGen.outDir(), RelinkablePath::Dir));

  connect(dialog, &QDialog::accepted, [this, dialog]() { this->performRelinking(dialog->relinker()); });

  dialog->show();
}

void MainWindow::performRelinking(const intrusive_ptr<AbstractRelinker>& relinker) {
  assert(relinker);

  if (!isProjectLoaded()) {
    return;
  }

  m_ptrPages->performRelinking(*relinker);
  m_ptrStages->performRelinking(*relinker);
  m_outFileNameGen.performRelinking(*relinker);

  Utils::maybeCreateCacheDir(m_outFileNameGen.outDir());

  m_ptrThumbnailCache->setThumbDir(Utils::outputDirToThumbDir(m_outFileNameGen.outDir()));
  resetThumbSequence(currentPageOrderProvider());
  m_selectedPage.set(m_ptrThumbSequence->selectionLeader().id(), getCurrentView());

  reloadRequested();
}

void MainWindow::goFirstPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo first_page(m_ptrThumbSequence->firstPage());
  if (!first_page.isNull()) {
    goToPage(first_page.id());
  }
}

void MainWindow::goLastPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo last_page(m_ptrThumbSequence->lastPage());
  if (!last_page.isNull()) {
    goToPage(last_page.id());
  }
}

void MainWindow::goNextPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo next_page(m_ptrThumbSequence->nextPage(m_ptrThumbSequence->selectionLeader().id()));
  if (!next_page.isNull()) {
    goToPage(next_page.id());
  }
}

void MainWindow::goPrevPage() {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }

  const PageInfo prev_page(m_ptrThumbSequence->prevPage(m_ptrThumbSequence->selectionLeader().id()));
  if (!prev_page.isNull()) {
    goToPage(prev_page.id());
  }
}

void MainWindow::goToPage(const PageId& page_id) {
  focusButton->setChecked(true);

  m_ptrThumbSequence->setSelection(page_id);

  // If the page was already selected, it will be reloaded.
  // That's by design.
  updateMainArea();

  if (m_autoSaveTimer.remainingTime() <= 0) {
    m_autoSaveTimer.start(30000);
  }
}

void MainWindow::currentPageChanged(const PageInfo& page_info,
                                    const QRectF& thumb_rect,
                                    const ThumbnailSequence::SelectionFlags flags) {
  m_selectedPage.set(page_info.id(), getCurrentView());

  if ((flags & ThumbnailSequence::SELECTED_BY_USER) || focusButton->isChecked()) {
    if (!(flags & ThumbnailSequence::AVOID_SCROLLING_TO)) {
      thumbView->ensureVisible(thumb_rect, 0, 0);
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

  if (flags & ThumbnailSequence::SELECTED_BY_USER) {
    if (m_autoSaveTimer.remainingTime() <= 0) {
      m_autoSaveTimer.start(30000);
    }
  }
}

void MainWindow::autoSaveProject() {
  if (m_projectFile.isEmpty()) {
    return;
  }

  if (!m_autoSaveProject) {
    return;
  }

  saveProjectWithFeedback(m_projectFile);
}

void MainWindow::pageContextMenuRequested(const PageInfo& page_info_, const QPoint& screen_pos, bool selected) {
  if (isBatchProcessingInProgress()) {
    return;
  }
  // Make a copy to prevent it from being invalidated.
  const PageInfo page_info(page_info_);

  if (!selected) {
    goToPage(page_info.id());
  }

  QMenu menu;

  QAction* ins_before = menu.addAction(QIcon(":/icons/insert-before-16.png"), tr("Insert before ..."));
  QAction* ins_after = menu.addAction(QIcon(":/icons/insert-after-16.png"), tr("Insert after ..."));

  menu.addSeparator();

  QAction* remove = menu.addAction(QIcon(":/icons/user-trash.png"), tr("Remove from project ..."));

  QAction* action = menu.exec(screen_pos);
  if (action == ins_before) {
    showInsertFileDialog(BEFORE, page_info.imageId());
  } else if (action == ins_after) {
    showInsertFileDialog(AFTER, page_info.imageId());
  } else if (action == remove) {
    showRemovePagesDialog(m_ptrThumbSequence->selectedItems());
  }
}  // MainWindow::pageContextMenuRequested

void MainWindow::pastLastPageContextMenuRequested(const QPoint& screen_pos) {
  if (!isProjectLoaded()) {
    return;
  }

  QMenu menu;
  menu.addAction(QIcon(":/icons/insert-here-16.png"), tr("Insert here ..."));

  if (menu.exec(screen_pos)) {
    showInsertFileDialog(BEFORE, ImageId());
  }
}

void MainWindow::thumbViewFocusToggled(const bool checked) {
  const QRectF rect(m_ptrThumbSequence->selectionLeaderSceneRect());
  if (rect.isNull()) {
    // No selected items.
    return;
  }

  if (checked) {
    thumbView->ensureVisible(rect, 0, 0);
  }
}

void MainWindow::thumbViewScrolled() {
  const QRectF rect(m_ptrThumbSequence->selectionLeaderSceneRect());
  if (rect.isNull()) {
    // No items selected.
    return;
  }

  const QRectF viewport_rect(thumbView->viewport()->rect());
  const QRectF viewport_item_rect(thumbView->viewportTransform().mapRect(rect));

  const double intersection_threshold = 0.5;
  if ((viewport_item_rect.top() >= viewport_rect.top())
      && (viewport_item_rect.top() + viewport_item_rect.height() * intersection_threshold <= viewport_rect.bottom())) {
    // Item is visible.
  } else if ((viewport_item_rect.bottom() <= viewport_rect.bottom())
             && (viewport_item_rect.bottom() - viewport_item_rect.height() * intersection_threshold
                 >= viewport_rect.top())) {
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

  m_ptrInteractiveQueue->cancelAndClear();
  if (m_ptrBatchQueue) {
    // Should not happen, but just in case.
    m_ptrBatchQueue->cancelAndClear();
  }

  const bool was_below_fix_orientation = isBelowFixOrientation(m_curFilter);
  const bool was_below_select_content = isBelowSelectContent(m_curFilter);
  m_curFilter = selected.front().top();
  const bool now_below_fix_orientation = isBelowFixOrientation(m_curFilter);
  const bool now_below_select_content = isBelowSelectContent(m_curFilter);

  m_ptrStages->filterAt(m_curFilter)->selected();

  updateSortOptions();

  // Propagate context boxes down the stage list, if necessary.
  if (!was_below_select_content && now_below_select_content) {
    // IMPORTANT: this needs to go before resetting thumbnails,
    // because it may affect them.
    if (m_ptrContentBoxPropagator) {
      m_ptrContentBoxPropagator->propagate(*m_ptrPages);
    }  // Otherwise probably no project is loaded.
  }
  // Propagate page orientations (that might have changed) to the "Split Pages" stage.
  if (!was_below_fix_orientation && now_below_fix_orientation) {
    // IMPORTANT: this needs to go before resetting thumbnails,
    // because it may affect them.
    if (m_ptrPageOrientationPropagator) {
      m_ptrPageOrientationPropagator->propagate(*m_ptrPages);
    }  // Otherwise probably no project is loaded.
  }

  const int hor_scroll_bar_pos = thumbView->horizontalScrollBar()->value();
  const int ver_scroll_bar_pos = thumbView->verticalScrollBar()->value();

  resetThumbSequence(currentPageOrderProvider());

  if (!focusButton->isChecked()) {
    thumbView->horizontalScrollBar()->setValue(hor_scroll_bar_pos);
    thumbView->verticalScrollBar()->setValue(ver_scroll_bar_pos);
  }

  // load default settings for all the pages
  for (const PageInfo& pageInfo : m_ptrThumbSequence->toPageSequence()) {
    for (int i = 0; i < m_ptrStages->count(); i++) {
      m_ptrStages->filterAt(i)->loadDefaultSettings(pageInfo);
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

  const int hor_scroll_bar_pos = thumbView->horizontalScrollBar()->value();
  const int ver_scroll_bar_pos = thumbView->verticalScrollBar()->value();

  m_ptrStages->filterAt(m_curFilter)->selectPageOrder(idx);

  m_ptrThumbSequence->reset(m_ptrPages->toPageSequence(getCurrentView()), ThumbnailSequence::KEEP_SELECTION,
                            currentPageOrderProvider());

  if (!focusButton->isChecked()) {
    thumbView->horizontalScrollBar()->setValue(hor_scroll_bar_pos);
    thumbView->verticalScrollBar()->setValue(ver_scroll_bar_pos);
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

  m_ptrInteractiveQueue->cancelAndClear();

  m_ptrBatchQueue.reset(new ProcessingTaskQueue);
  PageInfo page(m_ptrThumbSequence->selectionLeader());
  for (; !page.isNull(); page = m_ptrThumbSequence->nextPage(page.id())) {
    for (int i = 0; i < m_ptrStages->count(); i++) {
      m_ptrStages->filterAt(i)->loadDefaultSettings(page);
    }
    m_ptrBatchQueue->addProcessingTask(page, createCompositeTask(page, m_curFilter, /*batch=*/true, m_debug));
  }

  focusButton->setChecked(true);

  removeFilterOptionsWidget();
  filterList->setBatchProcessingInProgress(true);
  filterList->setEnabled(false);

  BackgroundTaskPtr task(m_ptrBatchQueue->takeForProcessing());
  if (task) {
    do {
      m_ptrWorkerThreadPool->submitTask(task);
      if (!m_ptrWorkerThreadPool->hasSpareCapacity()) {
        break;
      }
    } while ((task = m_ptrBatchQueue->takeForProcessing()));
  } else {
    stopBatchProcessing();
  }

  page = m_ptrBatchQueue->selectedPage();
  if (!page.isNull()) {
    m_ptrThumbSequence->setSelection(page.id());
  }
  // Display the batch processing screen.
  updateMainArea();
}  // MainWindow::startBatchProcessing

void MainWindow::stopBatchProcessing(MainAreaAction main_area) {
  if (!isBatchProcessingInProgress()) {
    return;
  }

  const PageInfo page(m_ptrBatchQueue->selectedPage());
  if (!page.isNull()) {
    m_ptrThumbSequence->setSelection(page.id());
  }

  m_ptrBatchQueue->cancelAndClear();
  m_ptrBatchQueue.reset();

  filterList->setBatchProcessingInProgress(false);
  filterList->setEnabled(true);

  switch (main_area) {
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
  m_ptrInteractiveQueue->processingFinished(task);
  if (m_ptrBatchQueue) {
    m_ptrBatchQueue->processingFinished(task);
  }

  if (task->isCancelled()) {
    return;
  }

  if (!isBatchProcessingInProgress()) {
    if (!result->filter()) {
      // Error loading file.  No special action is necessary.
    } else if (result->filter() != m_ptrStages->filterAt(m_curFilter)) {
      // Error from one of the previous filters.
      const int idx = m_ptrStages->findFilter(result->filter());
      assert(idx >= 0);
      m_curFilter = idx;

      ScopedIncDec<int> selection_guard(m_ignoreSelectionChanges);
      filterList->selectRow(idx);
    }
  }

  // This needs to be done even if batch processing is taking place,
  // for instance because thumbnail invalidation is done from here.
  result->updateUI(this);

  if (isBatchProcessingInProgress()) {
    if (m_ptrBatchQueue->allProcessed()) {
      stopBatchProcessing();

      QApplication::alert(this);  // Flash the taskbar entry.
      if (m_checkBeepWhenFinished()) {
#if defined(Q_OS_UNIX)
        QString ext_play_cmd("play /usr/share/sounds/freedesktop/stereo/bell.oga");
#else
        QString ext_play_cmd;
#endif
        QSettings settings;
        QString cmd = settings.value("main_window/external_alarm_cmd", ext_play_cmd).toString();
        if (cmd.isEmpty()) {
          QApplication::beep();
        } else {
          Q_UNUSED(std::system(cmd.toStdString().c_str()));
        }
      }

      if (m_selectedPage.get(getCurrentView()) == m_ptrThumbSequence->lastPage().id()) {
        // If batch processing finished at the last page, jump to the first one.
        goFirstPage();
      }

      return;
    }

    do {
      const BackgroundTaskPtr task(m_ptrBatchQueue->takeForProcessing());
      if (!task) {
        break;
      }
      m_ptrWorkerThreadPool->submitTask(task);
    } while (m_ptrWorkerThreadPool->hasSpareCapacity());

    const PageInfo page(m_ptrBatchQueue->selectedPage());
    if (!page.isNull()) {
      m_ptrThumbSequence->setSelection(page.id());
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

  assert(!m_ptrFixDpiDialog);
  m_ptrFixDpiDialog = new FixDpiDialog(m_ptrPages->toImageFileInfo(), this);
  m_ptrFixDpiDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_ptrFixDpiDialog->setWindowModality(Qt::WindowModal);

  connect(m_ptrFixDpiDialog, SIGNAL(accepted()), SLOT(fixedDpiSubmitted()));

  m_ptrFixDpiDialog->show();
}

void MainWindow::fixedDpiSubmitted() {
  assert(m_ptrFixDpiDialog);
  assert(m_ptrPages);
  assert(m_ptrThumbSequence);

  const PageInfo selected_page_before(m_ptrThumbSequence->selectionLeader());

  m_ptrPages->updateMetadataFrom(m_ptrFixDpiDialog->files());

  // The thumbnail list also stores page metadata, including the DPI.
  m_ptrThumbSequence->reset(m_ptrPages->toPageSequence(getCurrentView()), ThumbnailSequence::KEEP_SELECTION,
                            m_ptrThumbSequence->pageOrderProvider());

  const PageInfo selected_page_after(m_ptrThumbSequence->selectionLeader());

  // Reload if the current page was affected.
  // Note that imageId() isn't supposed to change - we check just in case.
  if ((selected_page_before.imageId() != selected_page_after.imageId())
      || (selected_page_before.metadata() != selected_page_after.metadata())) {
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

  QString project_dir;
  if (!m_projectFile.isEmpty()) {
    project_dir = QFileInfo(m_projectFile).absolutePath();
  } else {
    QSettings settings;
    project_dir = settings.value("project/lastDir").toString();
  }

  QString project_file(
      QFileDialog::getSaveFileName(this, QString(), project_dir, tr("Scan Tailor Projects") + " (*.ScanTailor)"));
  if (project_file.isEmpty()) {
    return;
  }

  if (!project_file.endsWith(".ScanTailor", Qt::CaseInsensitive)) {
    project_file += ".ScanTailor";
  }

  if (saveProjectWithFeedback(project_file)) {
    m_projectFile = project_file;
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
  ProjectCreationContext* context = new ProjectCreationContext(this);
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

  QSettings settings;
  const QString project_dir(settings.value("project/lastDir").toString());

  const QString project_file(QFileDialog::getOpenFileName(this, tr("Open Project"), project_dir,
                                                          tr("Scan Tailor Projects") + " (*.ScanTailor)"));
  if (project_file.isEmpty()) {
    // Cancelled by user.
    return;
  }

  openProject(project_file);
}

void MainWindow::openProject(const QString& project_file) {
  QFile file(project_file);
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

  ProjectOpeningContext* context = new ProjectOpeningContext(this, project_file, doc);
  connect(context, SIGNAL(done(ProjectOpeningContext*)), SLOT(projectOpened(ProjectOpeningContext*)));
  context->proceed();
}

void MainWindow::projectOpened(ProjectOpeningContext* context) {
  RecentProjects rp;
  rp.read();
  rp.setMostRecent(context->projectFile());
  rp.write();

  QSettings settings;
  settings.setValue("project/lastDir", QFileInfo(context->projectFile()).absolutePath());

  switchToNewProject(context->projectReader()->pages(), context->projectReader()->outputDirectory(),
                     context->projectFile(), context->projectReader());
}

void MainWindow::closeProject() {
  closeProjectInteractive();
}

void MainWindow::openSettingsDialog() {
  SettingsDialog* dialog = new SettingsDialog(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);
  connect(dialog, SIGNAL(settingsChanged()), this, SLOT(onSettingsChanged()));
  dialog->show();
}

void MainWindow::openDefaultParamsDialog() {
  DefaultParamsDialog* dialog = new DefaultParamsDialog(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWindowModality(Qt::WindowModal);
  dialog->show();
}

void MainWindow::onSettingsChanged() {
  QSettings settings;

  m_autoSaveProject = settings.value("settings/auto_save_project").toBool();

  if (auto* app = dynamic_cast<Application*>(qApp)) {
    app->installLanguage(settings.value("settings/language").toString());
  }

  m_ptrThumbSequence->invalidateAllThumbnails();
}

void MainWindow::showAboutDialog() {
  Ui::AboutDialog ui;
  QDialog* dialog = new QDialog(this);
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

  m_ptrOutOfMemoryDialog->setParams(m_projectFile, m_ptrStages, m_ptrPages, m_selectedPage, m_outFileNameGen);

  closeProjectWithoutSaving();

  m_ptrOutOfMemoryDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_ptrOutOfMemoryDialog.release()->show();
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
  removeWidgetsFromLayout(m_pOptionsFrameLayout);
  // Delete the old widget we were owning, if any.
  m_optionsWidgetCleanup.clear();

  m_ptrOptionsWidget = 0;
}

void MainWindow::updateProjectActions() {
  const bool loaded = isProjectLoaded();
  actionSaveProject->setEnabled(loaded);
  actionSaveProjectAs->setEnabled(loaded);
  actionFixDpi->setEnabled(loaded);
  actionRelinking->setEnabled(loaded);
}

bool MainWindow::isBatchProcessingInProgress() const {
  return m_ptrBatchQueue.get() != 0;
}

bool MainWindow::isProjectLoaded() const {
  return !m_outFileNameGen.outDir().isEmpty();
}

bool MainWindow::isBelowSelectContent() const {
  return isBelowSelectContent(m_curFilter);
}

bool MainWindow::isBelowSelectContent(const int filter_idx) const {
  return filter_idx > m_ptrStages->selectContentFilterIdx();
}

bool MainWindow::isBelowFixOrientation(int filter_idx) const {
  return filter_idx > m_ptrStages->fixOrientationFilterIdx();
}

bool MainWindow::isOutputFilter() const {
  return isOutputFilter(m_curFilter);
}

bool MainWindow::isOutputFilter(const int filter_idx) const {
  return filter_idx == m_ptrStages->outputFilterIdx();
}

PageView MainWindow::getCurrentView() const {
  return m_ptrStages->filterAt(m_curFilter)->getView();
}

void MainWindow::updateMainArea() {
  if (m_ptrPages->numImages() == 0) {
    filterList->setBatchProcessingPossible(false);
    setDockWidgetsVisible(false);
    showNewOpenProjectPanel();
    m_statusBarPanel->clear();
  } else if (isBatchProcessingInProgress()) {
    filterList->setBatchProcessingPossible(false);
    setImageWidget(m_ptrBatchProcessingWidget.get(), KEEP_OWNERSHIP);
  } else {
    setDockWidgetsVisible(true);
    const PageInfo page(m_ptrThumbSequence->selectionLeader());
    if (page.isNull()) {
      filterList->setBatchProcessingPossible(false);
      removeImageWidget();
      removeFilterOptionsWidget();
    } else {
      // Note that loadPageInteractive may reset it to false.
      filterList->setBatchProcessingPossible(true);
      PageSequence pageSequence = m_ptrThumbSequence->toPageSequence();
      if (pageSequence.numPages() > 0) {
        m_statusBarPanel->updatePage(pageSequence.pageNo(page.id()) + 1, pageSequence.numPages(), page.id());
      }
      loadPageInteractive(page);
    }
  }
}

bool MainWindow::checkReadyForOutput(const PageId* ignore) const {
  return m_ptrStages->pageLayoutFilter()->checkReadyForOutput(*m_ptrPages, ignore);
}

void MainWindow::loadPageInteractive(const PageInfo& page) {
  assert(!isBatchProcessingInProgress());

  m_ptrInteractiveQueue->cancelAndClear();

  if (isOutputFilter() && !checkReadyForOutput(&page.id())) {
    filterList->setBatchProcessingPossible(false);

    const QString err_text(
        tr("Output is not yet possible, as the final size"
           " of pages is not yet known.\nTo determine it,"
           " run batch processing at \"Select Content\" or"
           " \"Margins\"."));

    removeFilterOptionsWidget();
    setImageWidget(new ErrorWidget(err_text), TRANSFER_OWNERSHIP);

    return;
  }

  for (int i = 0; i < m_ptrStages->count(); i++) {
    m_ptrStages->filterAt(i)->loadDefaultSettings(page);
  }

  if (!isBatchProcessingInProgress()) {
    if (m_pImageFrameLayout->indexOf(m_ptrProcessingIndicationWidget.get()) != -1) {
      m_ptrProcessingIndicationWidget->processingRestartedEffect();
    }
    setImageWidget(m_ptrProcessingIndicationWidget.get(), KEEP_OWNERSHIP, 0, false);
    m_ptrStages->filterAt(m_curFilter)->preUpdateUI(this, page);
  }

  assert(m_ptrThumbnailCache);

  m_ptrInteractiveQueue->cancelAndClear();
  m_ptrInteractiveQueue->addProcessingTask(page, createCompositeTask(page, m_curFilter, /*batch=*/false, m_debug));
  m_ptrWorkerThreadPool->submitTask(m_ptrInteractiveQueue->takeForProcessing());
}  // MainWindow::loadPageInteractive

void MainWindow::updateWindowTitle() {
  QString project_name;
  CommandLine cli = CommandLine::get();

  if (m_projectFile.isEmpty()) {
    project_name = tr("Unnamed");
  } else if (cli.hasWindowTitle()) {
    project_name = cli.getWindowTitle();
  } else {
    project_name = QFileInfo(m_projectFile).baseName();
  }
  const QString version(QString::fromUtf8(VERSION));
  setWindowTitle(tr("%2 - ScanTailor Advanced [%1bit]").arg(sizeof(void*) * 8).arg(project_name));
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

  const QFileInfo project_file(m_projectFile);
  const QFileInfo backup_file(project_file.absoluteDir(), QString::fromLatin1("Backup.") + project_file.fileName());
  const QString backup_file_path(backup_file.absoluteFilePath());

  ProjectWriter writer(m_ptrPages, m_selectedPage, m_outFileNameGen);

  if (!writer.write(backup_file_path, m_ptrStages->filters())) {
    // Backup file could not be written???
    QFile::remove(backup_file_path);
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

  if (compareFiles(m_projectFile, backup_file_path)) {
    // The project hasn't really changed.
    QFile::remove(backup_file_path);
    closeProjectWithoutSaving();

    return true;
  }

  switch (promptProjectSave()) {
    case SAVE:
      if (!Utils::overwritingRename(backup_file_path, m_projectFile)) {
        QMessageBox::warning(this, tr("Error"), tr("Error saving the project file!"));

        return false;
      }
      // fall through
    case DONT_SAVE:
      QFile::remove(backup_file_path);
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

bool MainWindow::saveProjectWithFeedback(const QString& project_file) {
  ProjectWriter writer(m_ptrPages, m_selectedPage, m_outFileNameGen);

  if (!writer.write(project_file, m_ptrStages->filters())) {
    QMessageBox::warning(this, tr("Error"), tr("Error saving the project file!"));

    return false;
  }

  return true;
}

/**
 * Note: showInsertFileDialog(BEFORE, ImageId()) is legal and means inserting at the end.
 */
void MainWindow::showInsertFileDialog(BeforeOrAfter before_or_after, const ImageId& existing) {
  if (isBatchProcessingInProgress() || !isProjectLoaded()) {
    return;
  }
  // We need to filter out files already in project.
  class ProxyModel : public QSortFilterProxyModel {
   public:
    ProxyModel(const ProjectPages& pages) {
      setDynamicSortFilter(true);

      const PageSequence sequence(pages.toPageSequence(IMAGE_VIEW));
      for (const PageInfo& page : sequence) {
        m_inProjectFiles.push_back(QFileInfo(page.imageId().filePath()));
      }
    }

   protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
      const QModelIndex idx(source_parent.child(source_row, 0));
      const QVariant data(idx.data(QFileSystemModel::FilePathRole));
      if (data.isNull()) {
        return true;
      }

      return !m_inProjectFiles.contains(QFileInfo(data.toString()));
    }

    virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const { return left.row() < right.row(); }

   private:
    QFileInfoList m_inProjectFiles;
  };


  std::unique_ptr<QFileDialog> dialog(
      new QFileDialog(this, tr("Files to insert"), QFileInfo(existing.filePath()).absolutePath()));
  dialog->setFileMode(QFileDialog::ExistingFiles);
  dialog->setProxyModel(new ProxyModel(*m_ptrPages));
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


  std::vector<ImageFileInfo> new_files;
  std::vector<QString> loaded_files;
  std::vector<QString> failed_files;  // Those we failed to read metadata from.
  // dialog->selectedFiles() returns file list in reverse order.
  for (int i = files.size() - 1; i >= 0; --i) {
    const QFileInfo file_info(files[i]);
    ImageFileInfo image_file_info(file_info, std::vector<ImageMetadata>());

    const ImageMetadataLoader::Status status = ImageMetadataLoader::load(
        files.at(i), [&](const ImageMetadata& metadata) { image_file_info.imageInfo().push_back(metadata); });

    if (status == ImageMetadataLoader::LOADED) {
      new_files.push_back(image_file_info);
      loaded_files.push_back(file_info.absoluteFilePath());
    } else {
      failed_files.push_back(file_info.absoluteFilePath());
    }
  }

  if (!failed_files.empty()) {
    std::unique_ptr<LoadFilesStatusDialog> err_dialog(new LoadFilesStatusDialog(this));
    err_dialog->setLoadedFiles(loaded_files);
    err_dialog->setFailedFiles(failed_files);
    err_dialog->setOkButtonName(QString(" %1 ").arg(tr("Skip failed files")));
    if ((err_dialog->exec() != QDialog::Accepted) || loaded_files.empty()) {
      return;
    }
  }

  // Check if there is at least one DPI that's not OK.
  if (std::find_if(new_files.begin(), new_files.end(), [&](ImageFileInfo p) -> bool { return !p.isDpiOK(); })
      != new_files.end()) {
    std::unique_ptr<FixDpiDialog> dpi_dialog(new FixDpiDialog(new_files, this));
    dpi_dialog->setWindowModality(Qt::WindowModal);
    if (dpi_dialog->exec() != QDialog::Accepted) {
      return;
    }

    new_files = dpi_dialog->files();
  }

  // Actually insert the new pages.
  for (const ImageFileInfo& file : new_files) {
    int image_num = -1;  // Zero-based image number in a multi-page TIFF.
    for (const ImageMetadata& metadata : file.imageInfo()) {
      ++image_num;

      const int num_sub_pages = ProjectPages::adviseNumberOfLogicalPages(metadata, OrthogonalRotation());
      const ImageInfo image_info(ImageId(file.fileInfo(), image_num), metadata, num_sub_pages, false, false);
      insertImage(image_info, before_or_after, existing);
    }
  }
}  // MainWindow::showInsertFileDialog

void MainWindow::showRemovePagesDialog(const std::set<PageId>& pages) {
  std::unique_ptr<QDialog> dialog(new QDialog(this));
  Ui::RemovePagesDialog ui;
  ui.setupUi(dialog.get());
  ui.icon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxQuestion).pixmap(48, 48));

  ui.text->setText(ui.text->text().arg(pages.size()));

  QPushButton* remove_btn = ui.buttonBox->button(QDialogButtonBox::Ok);
  remove_btn->setText(tr("Remove"));

  dialog->setWindowModality(Qt::WindowModal);
  if (dialog->exec() == QDialog::Accepted) {
    removeFromProject(pages);
    eraseOutputFiles(pages);
  }
}

/**
 * Note: insertImage(..., BEFORE, ImageId()) is legal and means inserting at the end.
 */
void MainWindow::insertImage(const ImageInfo& new_image, BeforeOrAfter before_or_after, ImageId existing) {
  std::vector<PageInfo> pages(m_ptrPages->insertImage(new_image, before_or_after, existing, getCurrentView()));

  if (before_or_after == BEFORE) {
    // The second one will be inserted first, then the first
    // one will be inserted BEFORE the second one.
    std::reverse(pages.begin(), pages.end());
  }

  for (const PageInfo& page_info : pages) {
    m_outFileNameGen.disambiguator()->registerFile(page_info.imageId().filePath());
    m_ptrThumbSequence->insert(page_info, before_or_after, existing);
    existing = page_info.imageId();
  }
}

void MainWindow::removeFromProject(const std::set<PageId>& pages) {
  m_ptrInteractiveQueue->cancelAndRemove(pages);
  if (m_ptrBatchQueue) {
    m_ptrBatchQueue->cancelAndRemove(pages);
  }

  m_ptrPages->removePages(pages);


  const PageSequence itemsInOrder = m_ptrThumbSequence->toPageSequence();
  std::set<PageId> new_selection;

  bool select_first_non_deleted = false;
  if (itemsInOrder.numPages() > 0) {
    // if first page was deleted select first not deleted page
    // otherwise select last not deleted page from beginning
    select_first_non_deleted = pages.find(itemsInOrder.pageAt(0).id()) != pages.end();

    PageId last_non_deleted;
    for (const PageInfo& page : itemsInOrder) {
      const PageId& id = page.id();
      const bool was_deleted = (pages.find(id) != pages.end());

      if (!was_deleted) {
        if (select_first_non_deleted) {
          m_ptrThumbSequence->setSelection(id);
          new_selection.insert(id);
          break;
        } else {
          last_non_deleted = id;
        }
      } else if (!select_first_non_deleted && !last_non_deleted.isNull()) {
        m_ptrThumbSequence->setSelection(last_non_deleted);
        new_selection.insert(last_non_deleted);
        break;
      }
    }

    m_ptrThumbSequence->removePages(pages);

    if (new_selection.empty()) {
      // fallback to old behaviour
      if (m_ptrThumbSequence->selectionLeader().isNull()) {
        m_ptrThumbSequence->setSelection(m_ptrThumbSequence->firstPage().id());
      }
    }
  }

  updateMainArea();
}  // MainWindow::removeFromProject

void MainWindow::eraseOutputFiles(const std::set<PageId>& pages) {
  std::vector<PageId::SubPage> erase_variations;
  erase_variations.reserve(3);

  for (const PageId& page_id : pages) {
    erase_variations.clear();
    switch (page_id.subPage()) {
      case PageId::SINGLE_PAGE:
        erase_variations.push_back(PageId::SINGLE_PAGE);
        erase_variations.push_back(PageId::LEFT_PAGE);
        erase_variations.push_back(PageId::RIGHT_PAGE);
        break;
      case PageId::LEFT_PAGE:
        erase_variations.push_back(PageId::SINGLE_PAGE);
        erase_variations.push_back(PageId::LEFT_PAGE);
        break;
      case PageId::RIGHT_PAGE:
        erase_variations.push_back(PageId::SINGLE_PAGE);
        erase_variations.push_back(PageId::RIGHT_PAGE);
        break;
    }

    for (PageId::SubPage subpage : erase_variations) {
      QFile::remove(m_outFileNameGen.filePathFor(PageId(page_id.imageId(), subpage)));
    }
  }
}

BackgroundTaskPtr MainWindow::createCompositeTask(const PageInfo& page,
                                                  const int last_filter_idx,
                                                  const bool batch,
                                                  bool debug) {
  intrusive_ptr<fix_orientation::Task> fix_orientation_task;
  intrusive_ptr<page_split::Task> page_split_task;
  intrusive_ptr<deskew::Task> deskew_task;
  intrusive_ptr<select_content::Task> select_content_task;
  intrusive_ptr<page_layout::Task> page_layout_task;
  intrusive_ptr<output::Task> output_task;

  if (batch) {
    debug = false;
  }

  if (last_filter_idx >= m_ptrStages->outputFilterIdx()) {
    output_task
        = m_ptrStages->outputFilter()->createTask(page.id(), m_ptrThumbnailCache, m_outFileNameGen, batch, debug);
    debug = false;
  }
  if (last_filter_idx >= m_ptrStages->pageLayoutFilterIdx()) {
    page_layout_task = m_ptrStages->pageLayoutFilter()->createTask(page.id(), output_task, batch, debug);
    debug = false;
  }
  if (last_filter_idx >= m_ptrStages->selectContentFilterIdx()) {
    select_content_task = m_ptrStages->selectContentFilter()->createTask(page.id(), page_layout_task, batch, debug);
    debug = false;
  }
  if (last_filter_idx >= m_ptrStages->deskewFilterIdx()) {
    deskew_task = m_ptrStages->deskewFilter()->createTask(page.id(), select_content_task, batch, debug);
    debug = false;
  }
  if (last_filter_idx >= m_ptrStages->pageSplitFilterIdx()) {
    page_split_task = m_ptrStages->pageSplitFilter()->createTask(page, deskew_task, batch, debug);
    debug = false;
  }
  if (last_filter_idx >= m_ptrStages->fixOrientationFilterIdx()) {
    fix_orientation_task = m_ptrStages->fixOrientationFilter()->createTask(page.id(), page_split_task, batch);
    debug = false;
  }
  assert(fix_orientation_task);

  return make_intrusive<LoadFileTask>(batch ? BackgroundTask::BATCH : BackgroundTask::INTERACTIVE, page,
                                      m_ptrThumbnailCache, m_ptrPages, fix_orientation_task);
}  // MainWindow::createCompositeTask

intrusive_ptr<CompositeCacheDrivenTask> MainWindow::createCompositeCacheDrivenTask(const int last_filter_idx) {
  intrusive_ptr<fix_orientation::CacheDrivenTask> fix_orientation_task;
  intrusive_ptr<page_split::CacheDrivenTask> page_split_task;
  intrusive_ptr<deskew::CacheDrivenTask> deskew_task;
  intrusive_ptr<select_content::CacheDrivenTask> select_content_task;
  intrusive_ptr<page_layout::CacheDrivenTask> page_layout_task;
  intrusive_ptr<output::CacheDrivenTask> output_task;

  if (last_filter_idx >= m_ptrStages->outputFilterIdx()) {
    output_task = m_ptrStages->outputFilter()->createCacheDrivenTask(m_outFileNameGen);
  }
  if (last_filter_idx >= m_ptrStages->pageLayoutFilterIdx()) {
    page_layout_task = m_ptrStages->pageLayoutFilter()->createCacheDrivenTask(output_task);
  }
  if (last_filter_idx >= m_ptrStages->selectContentFilterIdx()) {
    select_content_task = m_ptrStages->selectContentFilter()->createCacheDrivenTask(page_layout_task);
  }
  if (last_filter_idx >= m_ptrStages->deskewFilterIdx()) {
    deskew_task = m_ptrStages->deskewFilter()->createCacheDrivenTask(select_content_task);
  }
  if (last_filter_idx >= m_ptrStages->pageSplitFilterIdx()) {
    page_split_task = m_ptrStages->pageSplitFilter()->createCacheDrivenTask(deskew_task);
  }
  if (last_filter_idx >= m_ptrStages->fixOrientationFilterIdx()) {
    fix_orientation_task = m_ptrStages->fixOrientationFilter()->createCacheDrivenTask(page_split_task);
  }

  assert(fix_orientation_task);

  return fix_orientation_task;
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
