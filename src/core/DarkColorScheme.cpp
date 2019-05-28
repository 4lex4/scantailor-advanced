
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
  m_palette.setColor(QPalette::Window, QColor(0x53, 0x53, 0x53));
  m_palette.setColor(QPalette::WindowText, QColor(0xDD, 0xDD, 0xDD));
  m_palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x98, 0x98, 0x98));
  m_palette.setColor(QPalette::Base, QColor(0x45, 0x45, 0x45));
  m_palette.setColor(QPalette::Disabled, QPalette::Base, QColor(0x4D, 0x4D, 0x4D));
  m_palette.setColor(QPalette::AlternateBase, m_palette.color(QPalette::Window));
  m_palette.setColor(QPalette::Disabled, QPalette::AlternateBase,
                     m_palette.color(QPalette::Disabled, QPalette::Window));
  m_palette.setColor(QPalette::ToolTipBase, QColor(0x70, 0x70, 0x70));
  m_palette.setColor(QPalette::ToolTipText, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Text, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Disabled, QPalette::Text, m_palette.color(QPalette::Disabled, QPalette::WindowText));
  m_palette.setColor(QPalette::Light, QColor(0x66, 0x66, 0x66));
  m_palette.setColor(QPalette::Midlight, QColor(0x53, 0x53, 0x53));
  m_palette.setColor(QPalette::Dark, QColor(0x40, 0x40, 0x40));
  m_palette.setColor(QPalette::Mid, QColor(0x33, 0x33, 0x33));
  m_palette.setColor(QPalette::Shadow, QColor(0x26, 0x26, 0x26));
  m_palette.setColor(QPalette::Button, m_palette.color(QPalette::Base));
  m_palette.setColor(QPalette::Disabled, QPalette::Button, m_palette.color(QPalette::Disabled, QPalette::Base));
  m_palette.setColor(QPalette::ButtonText, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                     m_palette.color(QPalette::Disabled, QPalette::WindowText));
  m_palette.setColor(QPalette::BrightText, QColor(0xFC, 0x52, 0x48));
  m_palette.setColor(QPalette::Link, QColor(0x4F, 0x95, 0xFC));
  m_palette.setColor(QPalette::Highlight, QColor(0x6B, 0x6B, 0x6B));
  m_palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(0x5D, 0x5D, 0x5D));
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
  m_customColors[ThumbnailSequenceSelectedItemBackground] = QColor(0x42, 0x42, 0x42);
  m_customColors[ThumbnailSequenceSelectionLeaderBackground] = QColor(0x55, 0x55, 0x55);
  m_customColors[OpenNewProjectBorder] = QColor(0x53, 0x53, 0x53);
  m_customColors[ProcessingIndicationFade] = QColor(0x28, 0x28, 0x28);
  m_customColors[ProcessingIndicationHeadColor] = m_palette.color(QPalette::WindowText);
  m_customColors[ProcessingIndicationTail] = m_palette.color(QPalette::Highlight);
  m_customColors[StageListHead] = m_customColors.at(ProcessingIndicationHeadColor);
  m_customColors[StageListTail] = m_customColors.at(ProcessingIndicationTail);
  m_customColors[FixDpiDialogErrorText] = QColor(0xF3, 0x49, 0x41);
}
