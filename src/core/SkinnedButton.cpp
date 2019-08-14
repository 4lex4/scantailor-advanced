/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "SkinnedButton.h"
#include <QBitmap>
#include <QEvent>

SkinnedButton::SkinnedButton(const QIcon& icon, QWidget* parent) : QToolButton(parent), m_normalStateIcon(icon) {
  initialize();
}

SkinnedButton::SkinnedButton(const QIcon& normal_state_icon,
                             const QIcon& hover_state_icon,
                             const QIcon& pressed_state_icon,
                             QWidget* parent)
    : QToolButton(parent),
      m_normalStateIcon(normal_state_icon),
      m_hoverStateIcon(hover_state_icon),
      m_pressedStateIcon(pressed_state_icon) {
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
