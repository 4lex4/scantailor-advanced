// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ProcessingIndicationWidget.h"
#include <ColorInterpolation.h>
#include <QPaintEvent>
#include <QPainter>
#include <QTimerEvent>
#include "ColorSchemeManager.h"

using namespace imageproc;

static const double distinction_increase = 1.0 / 5.0;
static const double distinction_decrease = -1.0 / 3.0;

ProcessingIndicationWidget::ProcessingIndicationWidget(QWidget* parent)
    : QWidget(parent), m_animation(10), m_distinction(1.0), m_distinctionDelta(distinction_increase), m_timerId(0) {
  m_headColor = ColorSchemeManager::instance().getColorParam(ColorScheme::ProcessingIndicationHeadColor,
                                                             palette().color(QPalette::Window).darker(200));
  m_tailColor = ColorSchemeManager::instance().getColorParam(ColorScheme::ProcessingIndicationTail,
                                                             palette().color(QPalette::Window).darker(130));
}

void ProcessingIndicationWidget::resetAnimation() {
  m_distinction = 1.0;
  m_distinctionDelta = distinction_increase;
}

void ProcessingIndicationWidget::processingRestartedEffect() {
  m_distinction = 1.0;
  m_distinctionDelta = distinction_decrease;
}

void ProcessingIndicationWidget::paintEvent(QPaintEvent* event) {
  QRect animation_rect(animationRect());
  if (!event->rect().contains(animation_rect)) {
    update(animation_rect);

    return;
  }

  QColor head_color(colorInterpolation(m_tailColor, m_headColor, m_distinction));

  m_distinction += m_distinctionDelta;
  if (m_distinction > 1.0) {
    m_distinction = 1.0;
  } else if (m_distinction <= 0.0) {
    m_distinction = 0.0;
    m_distinctionDelta = distinction_increase;
  }

  QPainter painter(this);

  QColor fade_color
      = ColorSchemeManager::instance().getColorParam(ColorScheme::ProcessingIndicationFade, palette().window().color());
  fade_color.setAlpha(127);
  painter.fillRect(rect(), fade_color);

  m_animation.nextFrame(head_color, m_tailColor, &painter, animation_rect);

  if (m_timerId == 0) {
    m_timerId = startTimer(180);
  }
}

void ProcessingIndicationWidget::timerEvent(QTimerEvent* event) {
  killTimer(event->timerId());
  m_timerId = 0;
  update(animationRect());
}

QRect ProcessingIndicationWidget::animationRect() const {
  QRect r(0, 0, 80, 80);
  r.moveCenter(rect().center());
  r &= rect();

  return r;
}
