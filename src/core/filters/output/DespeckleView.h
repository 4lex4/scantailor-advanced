// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.


#ifndef OUTPUT_DESPECKLE_VIEW_H_
#define OUTPUT_DESPECKLE_VIEW_H_

#include <QImage>
#include <QStackedWidget>
#include "DespeckleLevel.h"
#include "DespeckleState.h"
#include "Dpi.h"
#include <BinaryImage.h>
#include "intrusive_ptr.h"

class DebugImages;
class ProcessingIndicationWidget;
class ImageViewBase;

namespace output {
class DespeckleVisualization;

class DespeckleView : public QStackedWidget {
  Q_OBJECT
 public:
  /**
   * \param despeckle_state Describes a particular despeckling.
   * \param visualization Optional despeckle visualization.
   *        If null, it will be reconstructed from \p despeckle_state
   *        when this widget becomes visible.
   * \param debug Indicates whether debugging is turned on.
   */
  DespeckleView(const DespeckleState& despeckle_state, const DespeckleVisualization& visualization, bool debug);

  ~DespeckleView() override;

 public slots:

  void despeckleLevelChanged(double level, bool* handled);

 signals:

  void imageViewCreated(ImageViewBase*);

 protected:
  void hideEvent(QHideEvent* evt) override;

  void showEvent(QShowEvent* evt) override;

 private:
  class TaskCancelException;
  class TaskCancelHandle;
  class DespeckleTask;
  class DespeckleResult;

  enum AnimationAction { RESET_ANIMATION, RESUME_ANIMATION };

  void initiateDespeckling(AnimationAction anim_action);

  void despeckleDone(const DespeckleState& despeckle_state,
                     const DespeckleVisualization& visualization,
                     DebugImages* dbg);

  void cancelBackgroundTask();

  void removeImageViewWidget();

  DespeckleState m_despeckleState;
  intrusive_ptr<TaskCancelHandle> m_cancelHandle;
  ProcessingIndicationWidget* m_processingIndicator;
  double m_despeckleLevel;
  bool m_debug;
};
}  // namespace output
#endif  // ifndef OUTPUT_DESPECKLE_VIEW_H_
