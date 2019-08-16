// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PROCESSING_INDICATION_WIDGET_H_
#define PROCESSING_INDICATION_WIDGET_H_

#include <QColor>
#include <QWidget>
#include "BubbleAnimation.h"

class QRect;

/**
 * \brief This widget is displayed in the central area od the main window
 *        when an image is being processed.
 */
class ProcessingIndicationWidget : public QWidget {
 public:
  explicit ProcessingIndicationWidget(QWidget* parent = nullptr);

  /**
   * \brief Resets animation to the state it had just after
   *        constructing this object.
   */
  void resetAnimation();

  /**
   * \brief Launch the "processing restarted" effect.
   */
  void processingRestartedEffect();

 protected:
  void paintEvent(QPaintEvent* event) override;

  void timerEvent(QTimerEvent* event) override;

 private:
  QRect animationRect() const;

  BubbleAnimation m_animation;
  QColor m_headColor;
  QColor m_tailColor;
  double m_distinction;
  double m_distinctionDelta;
  int m_timerId;
};


#endif  // ifndef PROCESSING_INDICATION_WIDGET_H_
