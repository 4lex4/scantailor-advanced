// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SkinnedButton.h"

#include <QBitmap>
#include <QEvent>

SkinnedButton::SkinnedButton(const QIcon& icon, QWidget* parent) : QToolButton(parent), m_normalStateIcon(icon) {
  initialize();
}

SkinnedButton::SkinnedButton(const QIcon& normalStateIcon,
                             const QIcon& hoverStateIcon,
                             const QIcon& pressedStateIcon,
                             QWidget* parent)
    : QToolButton(parent),
      m_normalStateIcon(normalStateIcon),
      m_hoverStateIcon(hoverStateIcon),
      m_pressedStateIcon(pressedStateIcon) {
  initialize();
}

void SkinnedButton::setHoverImage(const QIcon& icon) {
  m_hoverStateIcon = icon;
}

void SkinnedButton::setPressedImage(const QIcon& icon) {
  m_pressedStateIcon = icon;
}

void SkinnedButton::initialize() {
  initStyleSheet();

  setIconSize(size());
  updateAppearance();

  connect(this, &QToolButton::pressed, [this]() {
    m_pressed = true;
    updateAppearance();
  });
  connect(this, &QToolButton::released, [this]() {
    m_pressed = false;
    updateAppearance();
  });
}

void SkinnedButton::initStyleSheet() {
  const QString style
      = "QToolButton {"
        "border: none;"
        "background: transparent;"
        "}";
  setStyleSheet(style);
}

bool SkinnedButton::event(QEvent* e) {
  switch (e->type()) {
    case QEvent::HoverEnter:
      m_hovered = true;
      break;
    case QEvent::HoverLeave:
      m_hovered = false;
      break;
    default:
      return QToolButton::event(e);
  }
  updateAppearance();
  return true;
}

void SkinnedButton::updateAppearance() {
  if (m_pressed && !m_pressedStateIcon.isNull()) {
    setIcon(m_pressedStateIcon);
  } else if (m_hovered && !m_hoverStateIcon.isNull()) {
    setIcon(m_hoverStateIcon);
  } else {
    setIcon(m_normalStateIcon);
  }
}
