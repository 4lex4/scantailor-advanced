
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
  m_palette.setColor(QPalette::Window, QColor(0xF0, 0xF0, 0xF0));
  m_palette.setColor(QPalette::WindowText, QColor(0x30, 0x30, 0x30));
  m_palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x90, 0x90, 0x90));
  m_palette.setColor(QPalette::Base, QColor(0xFC, 0xFC, 0xFC));
  m_palette.setColor(QPalette::Disabled, QPalette::Base, QColor(0xFA, 0xFA, 0xFA));
  m_palette.setColor(QPalette::AlternateBase, m_palette.color(QPalette::Window));
  m_palette.setColor(QPalette::Disabled, QPalette::AlternateBase,
                     m_palette.color(QPalette::Disabled, QPalette::Window));
  m_palette.setColor(QPalette::ToolTipBase, QColor(0xFF, 0xFF, 0xCD));
  m_palette.setColor(QPalette::ToolTipText, Qt::black);
  m_palette.setColor(QPalette::Text, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Disabled, QPalette::Text, m_palette.color(QPalette::Disabled, QPalette::WindowText));
  m_palette.setColor(QPalette::Light, Qt::white);
  m_palette.setColor(QPalette::Midlight, QColor(0xF0, 0xF0, 0xF0));
  m_palette.setColor(QPalette::Dark, QColor(0xDA, 0xDA, 0xDA));
  m_palette.setColor(QPalette::Mid, QColor(0xCC, 0xCC, 0xCC));
  m_palette.setColor(QPalette::Shadow, QColor(0xBE, 0xBE, 0xBE));
  m_palette.setColor(QPalette::Button, m_palette.color(QPalette::Base));
  m_palette.setColor(QPalette::Disabled, QPalette::Button, m_palette.color(QPalette::Disabled, QPalette::Base));
  m_palette.setColor(QPalette::ButtonText, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Disabled, QPalette::ButtonText,
                     m_palette.color(QPalette::Disabled, QPalette::WindowText));
  m_palette.setColor(QPalette::BrightText, QColor(0xF4, 0x00, 0x00));
  m_palette.setColor(QPalette::Link, QColor(0x00, 0x00, 0xFF));
  m_palette.setColor(QPalette::Highlight, QColor(0xB5, 0xB5, 0xB5));
  m_palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(0xDF, 0xDF, 0xDF));
  m_palette.setColor(QPalette::HighlightedText, m_palette.color(QPalette::WindowText));
  m_palette.setColor(QPalette::Disabled, QPalette::HighlightedText,
                     m_palette.color(QPalette::Disabled, QPalette::WindowText));
}

void LightColorScheme::loadStyleSheet() {
  QFile styleSheetFile(QString::fromUtf8(":/light_scheme/qss/stylesheet.qss"));
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
  m_customColors[ThumbnailSequenceSelectedItemBackground] = QColor(0x72, 0x72, 0x72);
  m_customColors[ThumbnailSequenceSelectedItemText] = Qt::white;
  m_customColors[ThumbnailSequenceItemText] = Qt::black;
  m_customColors[ThumbnailSequenceSelectionLeaderBackground] = QColor(0x5E, 0x5E, 0x5E);
  m_customColors[OpenNewProjectBorder] = QColor(0xCC, 0xCC, 0xCC);
  m_customColors[ProcessingIndicationFade] = QColor(0x93, 0x93, 0x93);
  m_customColors[ProcessingIndicationHeadColor] = m_palette.color(QPalette::WindowText);
  m_customColors[ProcessingIndicationTail] = m_palette.color(QPalette::Highlight);
  m_customColors[StageListHead] = m_customColors.at(ProcessingIndicationHeadColor);
  m_customColors[StageListTail] = m_customColors.at(ProcessingIndicationTail);
  m_customColors[FixDpiDialogErrorText] = QColor(0xFB, 0x00, 0x00);
}
