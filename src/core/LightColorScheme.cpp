// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "LightColorScheme.h"

#include <QFile>
#include <QStyle>
#include <QStyleFactory>

#include "Utils.h"

using namespace core;

LightColorScheme::LightColorScheme() {
  loadPalette();
  loadStyleSheet();
  loadColorParams();
}

const QPalette* LightColorScheme::getPalette() const {
  return &m_palette;
}

const QString* LightColorScheme::getStyleSheet() const {
  return &m_styleSheet;
}

const ColorScheme::ColorParams* LightColorScheme::getColorParams() const {
  return &m_customColors;
}

QStyle* LightColorScheme::getStyle() const {
  return QStyleFactory::create("Fusion");
}

void LightColorScheme::loadPalette() {
  m_palette.setColor(QPalette::Window, "#F0F0F0");
  m_palette.setColor(QPalette::WindowText, "#303030");
  m_palette.setColor(QPalette::Disabled, QPalette::WindowText, "#909090");
  m_palette.setColor(QPalette::Base, "#FCFCFC");
  m_palette.setColor(QPalette::Disabled, QPalette::Base, "#FAFAFA");
  m_palette.setColor(QPalette::AlternateBase, m_palette.color(QPalette::Window));
  m_palette.setColor(QPalette::Disabled, QPalette::AlternateBase,
                     m_palette.color(QPalette::Disabled, QPalette::Window));
  m_palette.setColor(QPalette::ToolTipBase, "#FFFFCD");
  m_palette.setColor(QPalette::ToolTipText, Qt::black);
  m_palette.setColor(QPalette::Text, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Disabled, QPalette::Text, m_palette.color(QPalette::Disabled, QPalette::WindowText));
  m_palette.setColor(QPalette::Light, Qt::white);
  m_palette.setColor(QPalette::Midlight, "#F0F0F0");
  m_palette.setColor(QPalette::Dark, "#DADADA");
  m_palette.setColor(QPalette::Mid, "#CCCCCC");
  m_palette.setColor(QPalette::Shadow, "#BEBEBE");
  m_palette.setColor(QPalette::Button, m_palette.color(QPalette::Base));
  m_palette.setColor(QPalette::Disabled, QPalette::Button, m_palette.color(QPalette::Disabled, QPalette::Base));
  m_palette.setColor(QPalette::ButtonText, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                     m_palette.color(QPalette::Disabled, QPalette::WindowText));
  m_palette.setColor(QPalette::BrightText, "#F40000");
  m_palette.setColor(QPalette::Link, "#0000FF");
  m_palette.setColor(QPalette::Highlight, "#B5B5B5");
  m_palette.setColor(QPalette::Disabled, QPalette::Highlight, "#DFDFDF");
  m_palette.setColor(QPalette::HighlightedText, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
                     m_palette.color(QPalette::Disabled, QPalette::WindowText));
}

void LightColorScheme::loadStyleSheet() {
  QFile styleSheetFile(QString::fromUtf8(":/light_scheme/stylesheet/stylesheet.qss"));
  if (styleSheetFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_styleSheet = styleSheetFile.readAll();
    styleSheetFile.close();
  }

#ifdef _WIN32
  m_styleSheet = Utils::qssConvertPxToEm(m_styleSheet, 13, 4);
#else
  m_styleSheet = Utils::qssConvertPxToEm(m_styleSheet, 16, 4);
#endif
}

void LightColorScheme::loadColorParams() {
  m_customColors[ThumbnailSequenceSelectedItemBackground] = "#727272";
  m_customColors[ThumbnailSequenceSelectedItemText] = Qt::white;
  m_customColors[ThumbnailSequenceItemText] = Qt::black;
  m_customColors[ThumbnailSequenceSelectionLeaderBackground] = "#5E5E5E";
  m_customColors[OpenNewProjectBorder] = "#CCCCCC";
  m_customColors[ProcessingIndicationFade] = "#939393";
  m_customColors[ProcessingIndicationHeadColor] = m_palette.color(QPalette::WindowText);
  m_customColors[ProcessingIndicationTail] = m_palette.color(QPalette::Highlight);
  m_customColors[StageListHead] = m_customColors.at(ProcessingIndicationHeadColor);
  m_customColors[StageListTail] = m_customColors.at(ProcessingIndicationTail);
  m_customColors[FixDpiDialogErrorText] = "#FB0000";
}
