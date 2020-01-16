// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "NativeColorScheme.h"

#include <QPalette>

NativeColorScheme::NativeColorScheme() {
  loadStyleSheet();
  loadColorParams();
}

QStyle* NativeColorScheme::getStyle() const {
  return nullptr;
}

const QPalette* NativeColorScheme::getPalette() const {
  return nullptr;
}

const QString* NativeColorScheme::getStyleSheet() const {
  return &m_styleSheet;
}

const ColorScheme::ColorParams* NativeColorScheme::getColorParams() const {
  return &m_customColors;
}

void NativeColorScheme::loadStyleSheet() {
  const QPalette palette = QPalette();

  m_styleSheet.append(QString("QGraphicsView, ImageViewBase, QFrame#imageViewFrame {\n"
                              "  background-color: %1;\n"
                              "  background-attachment: scroll;\n"
                              "}")
                          .arg(palette.window().color().darker(115).name()));
}

void NativeColorScheme::loadColorParams() {
  const QPalette palette = QPalette();

  m_customColors[ThumbnailSequenceSelectedItemBackground] = palette.color(QPalette::Highlight).lighter(130);
  m_customColors[ThumbnailSequenceSelectionLeaderBackground] = palette.color(QPalette::Highlight);
  m_customColors[ProcessingIndicationFade] = palette.window().color().darker(115);
  if (palette.window().color().lightnessF() < 0.5) {
    // If system scheme is dark, adapt some colors.
    m_customColors[ProcessingIndicationHeadColor] = palette.color(QPalette::Window).lighter(200);
    m_customColors[ProcessingIndicationTail] = palette.color(QPalette::Window).lighter(130);
    m_customColors[StageListHead] = m_customColors.at(ProcessingIndicationHeadColor);
    m_customColors[StageListTail] = m_customColors.at(ProcessingIndicationTail);
  }
}
