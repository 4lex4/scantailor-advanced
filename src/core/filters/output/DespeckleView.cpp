// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DespeckleView.h"

#include <QDebug>
#include <QPointer>
#include <utility>

#include "AbstractCommand.h"
#include "BackgroundExecutor.h"
#include "BackgroundTask.h"
#include "BasicImageView.h"
#include "DebugImagesImpl.h"
#include "Despeckle.h"
#include "DespeckleVisualization.h"
#include "ImageViewBase.h"
#include "OutputMargins.h"
#include "ProcessingIndicationWidget.h"
#include "TabbedDebugImages.h"

using namespace imageproc;

namespace output {
class DespeckleView::TaskCancelException : public std::exception {
 public:
  const char* what() const noexcept override { return "Task cancelled"; }
};


class DespeckleView::TaskCancelHandle : public TaskStatus {
 public:
  void cancel() override;

  bool isCancelled() const override;

  void throwIfCancelled() const override;

 private:
  mutable QAtomicInt m_cancelFlag;
};


class DespeckleView::DespeckleTask : public AbstractCommand<BackgroundExecutor::TaskResultPtr> {
 public:
  DespeckleTask(DespeckleView* owner,
                const DespeckleState& despeckleState,
                std::shared_ptr<TaskCancelHandle> cancelHandle,
                double level,
                bool debug);

  BackgroundExecutor::TaskResultPtr operator()() override;

 private:
  QPointer<DespeckleView> m_owner;
  DespeckleState m_despeckleState;
  std::shared_ptr<TaskCancelHandle> m_cancelHandle;
  std::unique_ptr<DebugImages> m_dbg;
  double m_despeckleLevel;
};


class DespeckleView::DespeckleResult : public AbstractCommand<void> {
 public:
  DespeckleResult(QPointer<DespeckleView> owner,
                  std::shared_ptr<TaskCancelHandle> cancelHandle,
                  const DespeckleState& despeckleState,
                  const DespeckleVisualization& visualization,
                  std::unique_ptr<DebugImages> debugImages);

  // This method is called from the main thread.
  void operator()() override;

 private:
  QPointer<DespeckleView> m_owner;
  std::shared_ptr<TaskCancelHandle> m_cancelHandle;
  std::unique_ptr<DebugImages> m_dbg;
  DespeckleState m_despeckleState;
  DespeckleVisualization m_visualization;
};


/*============================ DespeckleView ==============================*/

DespeckleView::DespeckleView(const DespeckleState& despeckleState,
                             const DespeckleVisualization& visualization,
                             bool debug)
    : m_despeckleState(despeckleState),
      m_processingIndicator(new ProcessingIndicationWidget(this)),
      m_despeckleLevel(despeckleState.level()),
      m_debug(debug) {
  addWidget(m_processingIndicator);

  if (!visualization.isNull()) {
    // Create the image view.
    auto widget = std::make_unique<BasicImageView>(visualization.image(), visualization.downscaledImage());
    setCurrentIndex(addWidget(widget.release()));
    emit imageViewCreated(dynamic_cast<ImageViewBase*>(widget.get()));
  }
}

DespeckleView::~DespeckleView() {
  cancelBackgroundTask();
}

void DespeckleView::despeckleLevelChanged(const double newLevel, bool* handled) {
  if (newLevel == m_despeckleLevel) {
    return;
  }

  m_despeckleLevel = newLevel;

  if (isVisible()) {
    *handled = true;
    if (currentWidget() == m_processingIndicator) {
      initiateDespeckling(RESUME_ANIMATION);
    } else {
      initiateDespeckling(RESET_ANIMATION);
    }
  }
}

void DespeckleView::hideEvent(QHideEvent* evt) {
  QStackedWidget::hideEvent(evt);
  // We don't want background despeckling to continue when user
  // switches to another tab.
  cancelBackgroundTask();
}

void DespeckleView::showEvent(QShowEvent* evt) {
  QStackedWidget::showEvent(evt);

  if (currentWidget() == m_processingIndicator) {
    initiateDespeckling(RESET_ANIMATION);
  }
}

void DespeckleView::initiateDespeckling(const AnimationAction animAction) {
  removeImageViewWidget();
  if (animAction == RESET_ANIMATION) {
    m_processingIndicator->resetAnimation();
  } else {
    m_processingIndicator->processingRestartedEffect();
  }

  cancelBackgroundTask();
  m_cancelHandle.reset(new TaskCancelHandle);

  // Note that we are getting rid of m_initialSpeckles,
  // as we wouldn't need it any more.

  const auto task = std::make_shared<DespeckleTask>(this, m_despeckleState, m_cancelHandle, m_despeckleLevel, m_debug);
  ImageViewBase::backgroundExecutor().enqueueTask(task);
}

void DespeckleView::despeckleDone(const DespeckleState& despeckleState,
                                  const DespeckleVisualization& visualization,
                                  DebugImages* dbg) {
  assert(!visualization.isNull());

  m_despeckleState = despeckleState;

  removeImageViewWidget();

  std::unique_ptr<QWidget> widget
      = std::make_unique<BasicImageView>(visualization.image(), visualization.downscaledImage(), OutputMargins());

  if (dbg && !dbg->empty()) {
    auto tabWidget = std::make_unique<TabbedDebugImages>();
    tabWidget->addTab(widget.release(), "Main");
    AutoRemovingFile file;
    QString label;
    while (!(file = dbg->retrieveNext(&label)).get().isNull()) {
      tabWidget->addTab(new DebugImageView(file), label);
    }
    widget = std::move(tabWidget);
  }

  setCurrentIndex(addWidget(widget.release()));
  emit imageViewCreated(dynamic_cast<ImageViewBase*>(widget.get()));
}

void DespeckleView::cancelBackgroundTask() {
  if (m_cancelHandle) {
    m_cancelHandle->cancel();
    m_cancelHandle.reset();
  }
}

void DespeckleView::removeImageViewWidget() {
  // Widget 0 is always m_processingIndicator, so we start with 1.
  // Also, normally there can't be more than 2 widgets, but just in case ...
  while (count() > 1) {
    QWidget* wgt = widget(1);
    removeWidget(wgt);
    delete wgt;
  }
}

/*============================= DespeckleTask ==========================*/

DespeckleView::DespeckleTask::DespeckleTask(DespeckleView* owner,
                                            const DespeckleState& despeckleState,
                                            std::shared_ptr<TaskCancelHandle> cancelHandle,
                                            const double level,
                                            const bool debug)
    : m_owner(owner),
      m_despeckleState(despeckleState),
      m_cancelHandle(std::move(cancelHandle)),
      m_despeckleLevel(level) {
  if (debug) {
    m_dbg = std::make_unique<DebugImagesImpl>();
  }
}

BackgroundExecutor::TaskResultPtr DespeckleView::DespeckleTask::operator()() {
  try {
    m_cancelHandle->throwIfCancelled();

    m_despeckleState = m_despeckleState.redespeckle(m_despeckleLevel, *m_cancelHandle, m_dbg.get());

    m_cancelHandle->throwIfCancelled();

    DespeckleVisualization visualization(m_despeckleState.visualize());

    m_cancelHandle->throwIfCancelled();
    return std::make_shared<DespeckleResult>(m_owner, m_cancelHandle, m_despeckleState, visualization,
                                             std::move(m_dbg));
  } catch (const TaskCancelException&) {
    return nullptr;
  }
}

/*======================== DespeckleResult ===========================*/

DespeckleView::DespeckleResult::DespeckleResult(QPointer<DespeckleView> owner,
                                                std::shared_ptr<TaskCancelHandle> cancelHandle,
                                                const DespeckleState& despeckleState,
                                                const DespeckleVisualization& visualization,
                                                std::unique_ptr<DebugImages> debugImages)
    : m_owner(std::move(owner)),
      m_cancelHandle(std::move(cancelHandle)),
      m_dbg(std::move(debugImages)),
      m_despeckleState(despeckleState),
      m_visualization(visualization) {}

void DespeckleView::DespeckleResult::operator()() {
  if (m_cancelHandle->isCancelled()) {
    return;
  }

  if (DespeckleView* owner = m_owner) {
    owner->despeckleDone(m_despeckleState, m_visualization, m_dbg.get());
  }
}

/*========================= TaskCancelHandle ============================*/

void DespeckleView::TaskCancelHandle::cancel() {
  m_cancelFlag.fetchAndStoreRelaxed(1);
}

bool DespeckleView::TaskCancelHandle::isCancelled() const {
  return m_cancelFlag.fetchAndAddRelaxed(0) != 0;
}

void DespeckleView::TaskCancelHandle::throwIfCancelled() const {
  if (isCancelled()) {
    throw TaskCancelException();
  }
}
}  // namespace output