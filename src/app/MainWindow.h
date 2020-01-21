// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_MAINWINDOW_H_
#define SCANTAILOR_APP_MAINWINDOW_H_

#include <QMainWindow>
#include <QObjectCleanupHandler>
#include <QPointer>
#include <QSizeF>
#include <QString>
#include <QTimer>
#include <boost/function.hpp>
#include <memory>
#include <set>
#include <vector>

#include "AbstractCommand.h"
#include "BackgroundTask.h"
#include "BeforeOrAfter.h"
#include "FilterResult.h"
#include "FilterUiInterface.h"
#include "NonCopyable.h"
#include "OutputFileNameGenerator.h"
#include "PageId.h"
#include "PageRange.h"
#include "PageView.h"
#include "SelectedPage.h"
#include "StatusBarPanel.h"
#include "ThumbnailSequence.h"
#include "intrusive_ptr.h"
#include "ui_MainWindow.h"

class AbstractFilter;
class AbstractRelinker;
class ThumbnailPixmapCache;
class ProjectPages;
class PageSequence;
class StageSequence;
class PageOrderProvider;
class PageSelectionAccessor;
class FilterOptionsWidget;
class ProcessingIndicationWidget;
class ImageInfo;
class ImageViewBase;
class PageInfo;
class QStackedLayout;
class WorkerThreadPool;
class ProjectReader;
class DebugImages;
class ContentBoxPropagator;
class PageOrientationPropagator;
class ProjectCreationContext;
class ProjectOpeningContext;
class CompositeCacheDrivenTask;
class TabbedDebugImages;
class ProcessingTaskQueue;
class FixDpiDialog;
class OutOfMemoryDialog;
class QLineF;
class QRectF;
class QLayout;

class MainWindow : public QMainWindow, private FilterUiInterface, private Ui::MainWindow {
  DECLARE_NON_COPYABLE(MainWindow)

  Q_OBJECT
 public:
  MainWindow();

  ~MainWindow() override;

  PageSequence allPages() const;

  std::set<PageId> selectedPages() const;

  std::vector<PageRange> selectedRanges() const;

 protected:
  bool eventFilter(QObject* obj, QEvent* ev) override;

  void closeEvent(QCloseEvent* event) override;

  void timerEvent(QTimerEvent* event) override;

  void changeEvent(QEvent* event) override;

 public slots:

  void openProject(const QString& projectFile);

 private:
  enum MainAreaAction { UPDATE_MAIN_AREA, CLEAR_MAIN_AREA };

 private slots:

  void autoSaveProject();

  void goFirstPage();

  void goLastPage();

  void goNextPage();

  void goPrevPage();

  void goNextSelectedPage();

  void goPrevSelectedPage();

  void execGotoPageDialog();

  void goToPage(const PageId& pageId,
                ThumbnailSequence::SelectionAction selectionAction = ThumbnailSequence::RESET_SELECTION);

  void currentPageChanged(const PageInfo& pageInfo, const QRectF& thumbRect, ThumbnailSequence::SelectionFlags flags);

  void pageContextMenuRequested(const PageInfo& pageInfo, const QPoint& screenPos, bool selected);

  void pastLastPageContextMenuRequested(const QPoint& screenPos);

  void thumbViewFocusToggled(bool checked);

  void thumbViewScrolled();

  void filterSelectionChanged(const QItemSelection& selected);

  void switchFilter1();

  void switchFilter2();

  void switchFilter3();

  void switchFilter4();

  void switchFilter5();

  void switchFilter6();

  void pageOrderingChanged(int idx);

  void reloadRequested();

  void startBatchProcessing();

  void stopBatchProcessing(MainAreaAction mainArea = UPDATE_MAIN_AREA);

  void invalidateThumbnail(const PageId& pageId) override;

  void invalidateThumbnail(const PageInfo& pageInfo);

  void invalidateAllThumbnails() override;

  void showRelinkingDialog();

  void filterResult(const BackgroundTaskPtr& task, const FilterResultPtr& result);

  void debugToggled(bool enabled);

  void fixDpiDialogRequested();

  void fixedDpiSubmitted();

  void saveProjectTriggered();

  void saveProjectAsTriggered();

  void newProject();

  void newProjectCreated(ProjectCreationContext* context);

  void openProject();

  void projectOpened(ProjectOpeningContext* context);

  void closeProject();

  void openSettingsDialog();

  void openDefaultParamsDialog();

  void onSettingsChanged();

  void showAboutDialog();

  void handleOutOfMemorySituation();

 private:
  class PageSelectionProviderImpl;

  enum SavePromptResult { SAVE, DONT_SAVE, CANCEL };

  using FilterPtr = intrusive_ptr<AbstractFilter>;

  static void removeWidgetsFromLayout(QLayout* layout);

  void setOptionsWidget(FilterOptionsWidget* widget, Ownership ownership) override;

  void setImageWidget(QWidget* widget,
                      Ownership ownership,
                      DebugImages* debugImages = nullptr,
                      bool overlay = false) override;

  intrusive_ptr<AbstractCommand<void>> relinkingDialogRequester() override;

  void switchToNewProject(const intrusive_ptr<ProjectPages>& pages,
                          const QString& outDir,
                          const QString& projectFilePath = QString(),
                          const ProjectReader* projectReader = nullptr);

  void updateThumbViewMinWidth();

  void setupThumbView();

  void showNewOpenProjectPanel();

  SavePromptResult promptProjectSave();

  static bool compareFiles(const QString& fpath1, const QString& fpath2);

  intrusive_ptr<const PageOrderProvider> currentPageOrderProvider() const;

  void updateSortOptions();

  void resetThumbSequence(const intrusive_ptr<const PageOrderProvider>& pageOrderProvider,
                          ThumbnailSequence::SelectionAction selectionAction = ThumbnailSequence::RESET_SELECTION);

  void removeFilterOptionsWidget();

  void removeImageWidget();

  void updateProjectActions();

  bool isBatchProcessingInProgress() const;

  bool isProjectLoaded() const;

  bool isBelowSelectContent() const;

  bool isBelowSelectContent(int filterIdx) const;

  bool isBelowFixOrientation(int filterIdx) const;

  bool isOutputFilter() const;

  bool isOutputFilter(int filterIdx) const;

  PageView getCurrentView() const;

  void updateMainArea();

  bool checkReadyForOutput(const PageId* ignore = nullptr) const;

  void loadPageInteractive(const PageInfo& page);

  void updateWindowTitle();

  bool closeProjectInteractive();

  void closeProjectWithoutSaving();

  bool saveProjectWithFeedback(const QString& projectFile);

  void showInsertFileDialog(BeforeOrAfter beforeOrAfter, const ImageId& existig);

  void showRemovePagesDialog(const std::set<PageId>& pages);

  void insertImage(const ImageInfo& newImage, BeforeOrAfter beforeOrAfter, ImageId existing);

  void removeFromProject(const std::set<PageId>& pages);

  void eraseOutputFiles(const std::set<PageId>& pages);

  BackgroundTaskPtr createCompositeTask(const PageInfo& page, int lastFilterIdx, bool batch, bool debug);

  intrusive_ptr<CompositeCacheDrivenTask> createCompositeCacheDrivenTask(int lastFilterIdx);

  void createBatchProcessingWidget();

  void updateDisambiguationRecords(const PageSequence& pages);

  void performRelinking(const intrusive_ptr<AbstractRelinker>& relinker);

  PageSelectionAccessor newPageSelectionAccessor();

  void setDockWidgetsVisible(bool state);

  void scaleThumbnails(const QWheelEvent* wheelEvent);

  void updateMaxLogicalThumbSize();

  void updateThumbnailViewMode();

  void updateAutoSaveTimer();

  PageSequence currentPageSequence();

  void setupIcons();

  QSizeF m_maxLogicalThumbSize;
  intrusive_ptr<ProjectPages> m_pages;
  intrusive_ptr<StageSequence> m_stages;
  QString m_projectFile;
  OutputFileNameGenerator m_outFileNameGen;
  intrusive_ptr<ThumbnailPixmapCache> m_thumbnailCache;
  std::unique_ptr<ThumbnailSequence> m_thumbSequence;
  std::unique_ptr<WorkerThreadPool> m_workerThreadPool;
  std::unique_ptr<ProcessingTaskQueue> m_batchQueue;
  std::unique_ptr<ProcessingTaskQueue> m_interactiveQueue;
  QStackedLayout* m_imageFrameLayout;
  QStackedLayout* m_optionsFrameLayout;
  QPointer<FilterOptionsWidget> m_optionsWidget;
  QPointer<FixDpiDialog> m_fixDpiDialog;
  std::unique_ptr<TabbedDebugImages> m_tabbedDebugImages;
  std::unique_ptr<ContentBoxPropagator> m_contentBoxPropagator;
  std::unique_ptr<PageOrientationPropagator> m_pageOrientationPropagator;
  std::unique_ptr<QWidget> m_batchProcessingWidget;
  std::unique_ptr<ProcessingIndicationWidget> m_processingIndicationWidget;
  boost::function<bool()> m_checkBeepWhenFinished;
  SelectedPage m_selectedPage;
  QObjectCleanupHandler m_optionsWidgetCleanup;
  QObjectCleanupHandler m_imageWidgetCleanup;
  std::unique_ptr<OutOfMemoryDialog> m_outOfMemoryDialog;
  int m_curFilter;
  int m_ignoreSelectionChanges;
  int m_ignorePageOrderingChanges;
  bool m_debug;
  bool m_closing;
  QTimer m_autoSaveTimer;
  std::unique_ptr<StatusBarPanel> m_statusBarPanel;
  std::unique_ptr<QActionGroup> m_unitsMenuActionGroup;
  QTimer m_maxLogicalThumbSizeUpdater;
  QTimer m_sceneItemsPosUpdater;
};


#endif  // ifndef SCANTAILOR_APP_MAINWINDOW_H_
