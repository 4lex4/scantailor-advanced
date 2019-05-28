
#include "DarkColorScheme.h"
#include <QFile>
#include <QStyle>
#include <QStyleFactory>
#include "Utils.h"

using namespace core;

DarkColorScheme::DarkColorScheme() {
  loadPalette();
  loadStyleSheet();
  loadColorParams();
}

const QPalette* DarkColorScheme::getPalette() const {
  return &m_palette;
}

const QString* DarkColorScheme::getStyleSheet() const {
  return &m_styleSheet;
}

const ColorScheme::ColorParams* DarkColorScheme::getColorParams() const {
  return &m_customColors;
}

QStyle* DarkColorScheme::getStyle() const {
  return QStyleFactory::create("Fusion");
}

void DarkColorScheme::loadPalette() {
  m_palette.setColor(QPalette::Window, "#535353");
  m_palette.setColor(QPalette::WindowText, "#DDDDDD");
  m_palette.setColor(QPalette::Disabled, QPalette::WindowText, "#989898");
  m_palette.setColor(QPalette::Base, "#454545");
  m_palette.setColor(QPalette::Disabled, QPalette::Base, "#4D4D4D");
  m_palette.setColor(QPalette::AlternateBase, m_palette.color(QPalette::Window));
  m_palette.setColor(QPalette::Disabled, QPalette::AlternateBase,
                     m_palette.color(QPalette::Disabled, QPalette::Window));
  m_palette.setColor(QPalette::ToolTipBase, "#707070");
  m_palette.setColor(QPalette::ToolTipText, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Text, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Disabled, QPalette::Text, m_palette.color(QPalette::Disabled, QPalette::WindowText));
  m_palette.setColor(QPalette::Light, "#666666");
  m_palette.setColor(QPalette::Midlight, "#535353");
  m_palette.setColor(QPalette::Dark, "#404040");
  m_palette.setColor(QPalette::Mid, "#333333");
  m_palette.setColor(QPalette::Shadow, "#262626");
  m_palette.setColor(QPalette::Button, m_palette.color(QPalette::Base));
  m_palette.setColor(QPalette::Disabled, QPalette::Button, m_palette.color(QPalette::Disabled, QPalette::Base));
  m_palette.setColor(QPalette::ButtonText, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                     m_palette.color(QPalette::Disabled, QPalette::WindowText));
  m_palette.setColor(QPalette::BrightText, "#FC5248");
  m_palette.setColor(QPalette::Link, "#4F95FC");
  m_palette.setColor(QPalette::Highlight, "#6B6B6B");
  m_palette.setColor(QPalette::Disabled, QPalette::Highlight, "#5D5D5D");
  m_palette.setColor(QPalette::HighlightedText, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
                     m_palette.color(QPalette::Disabled, QPalette::WindowText));
}

void DarkColorScheme::loadStyleSheet() {
  QFile styleSheetFile(QString::fromUtf8(":/dark_scheme/qss/stylesheet.qss"));
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

void DarkColorScheme::loadColorParams() {
  m_customColors[ThumbnailSequenceSelectedItemBackground] = "#424242";
  m_customColors[ThumbnailSequenceSelectionLeaderBackground] = "#555555";
  m_customColors[OpenNewProjectBorder] = "#535353";
  m_customColors[ProcessingIndicationFade] = "#282828";
  m_customColors[ProcessingIndicationHeadColor] = m_palette.color(QPalette::WindowText);
  m_customColors[ProcessingIndicationTail] = m_palette.color(QPalette::Highlight);
  m_customColors[StageListHead] = m_customColors.at(ProcessingIndicationHeadColor);
  m_customColors[StageListTail] = m_customColors.at(ProcessingIndicationTail);
  m_customColors[FixDpiDialogErrorText] = "#F34941";
}
