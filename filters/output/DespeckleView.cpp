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

#include "DespeckleView.h"
#include <QDebug>
#include <QPointer>
#include <utility>
#include "AbstractCommand.h"
#include "BackgroundExecutor.h"
#include "BackgroundTask.h"
#include "BasicImageView.h"
#include "DebugImages.h"
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


class DespeckleView::TaskCancelHandle : public TaskStatus, public ref_countable {
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
                const DespeckleState& despeckle_state,
                intrusive_ptr<TaskCancelHandle> cancel_handle,
                double level,
                bool debug);

  BackgroundExecutor::TaskResultPtr operator()() override;

 private:
  QPointer<DespeckleView> m_owner;
  DespeckleState m_despeckleState;
  intrusive_ptr<TaskCancelHandle> m_cancelHandle;
  std::unique_ptr<DebugImages> m_dbg;
  double m_despeckleLevel;
};


class DespeckleView::DespeckleResult : public AbstractCommand<void> {
 public:
  DespeckleResult(QPointer<DespeckleView> owner,
                  intrusive_ptr<TaskCancelHandle> cancel_handle,
                  const DespeckleState& despeckle_state,
                  const DespeckleVisualization& visualization,
                  std::unique_ptr<DebugImages> debug_images);

  // This method is called from the main thread.
  void operator()() override;

 private:
  QPointer<DespeckleView> m_owner;
  intrusive_ptr<TaskCancelHandle> m_cancelHandle;
  std::unique_ptr<DebugImages> m_dbg;
  DespeckleState m_despeckleState;
  DespeckleVisualization m_visualization;
};


/*============================ DespeckleView ==============================*/

DespeckleView::DespeckleView(const DespeckleState& despeckle_state,
                             const DespeckleVisualization& visualization,
                             bool debug)
    : m_despeckleState(despeckle_state),
      m_processingIndicator(new ProcessingIndicationWidget(this)),
      m_despeckleLevel(despeckle_state.level()),
      m_debug(debug) {
  addWidget(m_processingIndicator);

  if (!visualization.isNull()) {
    // Create the image view.
    std::unique_ptr<QWidget> widget(new BasicImageView(visualization.image(), visualization.downscaledImage()));
    setCurrentIndex(addWidget(widget.release()));
    emit imageViewCreated(dynamic_cast<ImageViewBase*>(widget.get()));
  }
}

DespeckleView::~DespeckleView() {
  cancelBackgroundTask();
}

void DespeckleView::despeckleLevelChanged(const double new_level, bool* handled) {
  if (new_level == m_despeckleLevel) {
    return;
  }

  m_despeckleLevel = new_level;

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

void DespeckleView::initiateDespeckling(const AnimationAction anim_action) {
  removeImageViewWidget();
  if (anim_action == RESET_ANIMATION) {
    m_processingIndicator->resetAnimation();
  } else {
    m_processingIndicator->processingRestartedEffect();
  }

  cancelBackgroundTask();
  m_cancelHandle.reset(new TaskCancelHandle);

  // Note that we are getting rid of m_initialSpeckles,
  // as we wouldn't need it any more.

  const auto task = make_intrusive<DespeckleTask>(this, m_despeckleState, m_cancelHandle, m_despeckleLevel, m_debug);
  ImageViewBase::backgroundExecutor().enqueueTask(task);
}

void DespeckleView::despeckleDone(const DespeckleState& despeckle_state,
                                  const DespeckleVisualization& visualization,
                                  DebugImages* dbg) {
  assert(!visualization.isNull());

  m_despeckleState = despeckle_state;

  removeImageViewWidget();

  std::unique_ptr<QWidget> widget(
      new BasicImageView(visualization.image(), visualization.downscaledImage(), OutputMargins()));

  if (dbg && !dbg->empty()) {
    auto tab_widget = std::make_unique<TabbedDebugImages>();
    tab_widget->addTab(widget.release(), "Main");
    AutoRemovingFile file;
    QString label;
    while (!(file = dbg->retrieveNext(&label)).get().isNull()) {
      tab_widget->addTab(new DebugImageView(file), label);
    }
    widget = std::move(tab_widget);
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
                                            const DespeckleState& despeckle_state,
                                            intrusive_ptr<TaskCancelHandle> cancel_handle,
                                            const double level,
                                            const bool debug)
    : m_owner(owner),
      m_despeckleState(despeckle_state),
      m_cancelHandle(std::move(cancel_handle)),
      m_despeckleLevel(level) {
  if (debug) {
    m_dbg = std::make_unique<DebugImages>();
  }
}

BackgroundExecutor::TaskResultPtr DespeckleView::DespeckleTask::operator()() {
  try {
    m_cancelHandle->throwIfCancelled();

    m_despeckleState = m_despeckleState.redespeckle(m_despeckleLevel, *m_cancelHandle, m_dbg.get());

    m_cancelHandle->throwIfCancelled();

    DespeckleVisualization visualization(m_despeckleState.visualize());

    m_cancelHandle->throwIfCancelled();

    return make_intrusive<DespeckleResult>(m_owner, m_cancelHandle, m_despeckleState, visualization, std::move(m_dbg));
  } catch (const TaskCancelException&) {
    return nullptr;
  }
}

/*======================== DespeckleResult ===========================*/

DespeckleView::DespeckleResult::DespeckleResult(QPointer<DespeckleView> owner,
                                                intrusive_ptr<TaskCancelHandle> cancel_handle,
                                                const DespeckleState& despeckle_state,
                                                const DespeckleVisualization& visualization,
                                                std::unique_ptr<DebugImages> debug_images)
    : m_owner(std::move(owner)),
      m_cancelHandle(std::move(cancel_handle)),
      m_dbg(std::move(debug_images)),
      m_despeckleState(despeckle_state),
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