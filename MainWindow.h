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

#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_

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

  void openProject(const QString& project_file);

 private:
  enum MainAreaAction { UPDATE_MAIN_AREA, CLEAR_MAIN_AREA };

 private slots:

  void autoSaveProject();

  void goFirstPage();

  void goLastPage();

  void goNextPage();

  void goPrevPage();

  void goToPage(const PageId& page_id);

  void currentPageChanged(const PageInfo& page_info, const QRectF& thumb_rect, ThumbnailSequence::SelectionFlags flags);

  void pageContextMenuRequested(const PageInfo& page_info, const QPoint& screen_pos, bool selected);

  void pastLastPageContextMenuRequested(const QPoint& screen_pos);

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

  void stopBatchProcessing(MainAreaAction main_area = UPDATE_MAIN_AREA);

  void invalidateThumbnail(const PageId& page_id) override;

  void invalidateThumbnail(const PageInfo& page_info);

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

  typedef intrusive_ptr<AbstractFilter> FilterPtr;

  void setOptionsWidget(FilterOptionsWidget* widget, Ownership ownership) override;

  void setImageWidget(QWidget* widget,
                      Ownership ownership,
                      DebugImages* debug_images = nullptr,
                      bool clear_image_widget = true) override;

  intrusive_ptr<AbstractCommand<void>> relinkingDialogRequester() override;

  void switchToNewProject(const intrusive_ptr<ProjectPages>& pages,
                          const QString& out_dir,
                          const QString& project_file_path = QString(),
                          const ProjectReader* project_reader = nullptr);

  intrusive_ptr<ThumbnailPixmapCache> createThumbnailCache();

  void setupThumbView();

  void showNewOpenProjectPanel();

  SavePromptResult promptProjectSave();

  static bool compareFiles(const QString& fpath1, const QString& fpath2);

  intrusive_ptr<const PageOrderProvider> currentPageOrderProvider() const;

  void updateSortOptions();

  void resetThumbSequence(const intrusive_ptr<const PageOrderProvider>& page_order_provider);

  void removeWidgetsFromLayout(QLayout* layout);

  void removeFilterOptionsWidget();

  void removeImageWidget();

  void updateProjectActions();

  bool isBatchProcessingInProgress() const;

  bool isProjectLoaded() const;

  bool isBelowSelectContent() const;

  bool isBelowSelectContent(int filter_idx) const;

  bool isBelowFixOrientation(int filter_idx) const;

  bool isOutputFilter() const;

  bool isOutputFilter(int filter_idx) const;

  PageView getCurrentView() const;

  void updateMainArea();

  bool checkReadyForOutput(const PageId* ignore = nullptr) const;

  void loadPageInteractive(const PageInfo& page);

  void updateWindowTitle();

  bool closeProjectInteractive();

  void closeProjectWithoutSaving();

  bool saveProjectWithFeedback(const QString& project_file);

  void showInsertFileDialog(BeforeOrAfter before_or_after, const ImageId& existig);

  void showRemovePagesDialog(const std::set<PageId>& pages);

  void insertImage(const ImageInfo& new_image, BeforeOrAfter before_or_after, ImageId existing);

  void removeFromProject(const std::set<PageId>& pages);

  void eraseOutputFiles(const std::set<PageId>& pages);

  BackgroundTaskPtr createCompositeTask(const PageInfo& page, int last_filter_idx, bool batch, bool debug);

  intrusive_ptr<CompositeCacheDrivenTask> createCompositeCacheDrivenTask(int last_filter_idx);

  void createBatchProcessingWidget();

  void updateDisambiguationRecords(const PageSequence& pages);

  void performRelinking(const intrusive_ptr<AbstractRelinker>& relinker);

  PageSelectionAccessor newPageSelectionAccessor();

  void setDockWidgetsVisible(bool state);

  QSizeF m_maxLogicalThumbSize;
  intrusive_ptr<ProjectPages> m_ptrPages;
  intrusive_ptr<StageSequence> m_ptrStages;
  QString m_projectFile;
  OutputFileNameGenerator m_outFileNameGen;
  intrusive_ptr<ThumbnailPixmapCache> m_ptrThumbnailCache;
  std::unique_ptr<ThumbnailSequence> m_ptrThumbSequence;
  std::unique_ptr<WorkerThreadPool> m_ptrWorkerThreadPool;
  std::unique_ptr<ProcessingTaskQueue> m_ptrBatchQueue;
  std::unique_ptr<ProcessingTaskQueue> m_ptrInteractiveQueue;
  QStackedLayout* m_pImageFrameLayout;
  QStackedLayout* m_pOptionsFrameLayout;
  QPointer<FilterOptionsWidget> m_ptrOptionsWidget;
  QPointer<FixDpiDialog> m_ptrFixDpiDialog;
  std::unique_ptr<TabbedDebugImages> m_ptrTabbedDebugImages;
  std::unique_ptr<ContentBoxPropagator> m_ptrContentBoxPropagator;
  std::unique_ptr<PageOrientationPropagator> m_ptrPageOrientationPropagator;
  std::unique_ptr<QWidget> m_ptrBatchProcessingWidget;
  std::unique_ptr<ProcessingIndicationWidget> m_ptrProcessingIndicationWidget;
  boost::function<bool()> m_checkBeepWhenFinished;
  SelectedPage m_selectedPage;
  QObjectCleanupHandler m_optionsWidgetCleanup;
  QObjectCleanupHandler m_imageWidgetCleanup;
  std::unique_ptr<OutOfMemoryDialog> m_ptrOutOfMemoryDialog;
  int m_curFilter;
  int m_ignoreSelectionChanges;
  int m_ignorePageOrderingChanges;
  bool m_debug;
  bool m_closing;
  bool m_beepOnBatchProcessingCompletion;
  QTimer m_thumbResizeTimer;
  QTimer m_autoSaveTimer;
  bool m_autoSaveProject;
  std::unique_ptr<StatusBarPanel> m_statusBarPanel;
  std::unique_ptr<QActionGroup> m_unitsMenuActionGroup;
};


#endif  // ifndef MAINWINDOW_H_
