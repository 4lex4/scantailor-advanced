
#include "DarkScheme.h"
#include <memory>
#include <unordered_map>

std::unique_ptr<QPalette> DarkScheme::getPalette() const {
  std::unique_ptr<QPalette> darkPalette(new QPalette());

  darkPalette->setColor(QPalette::Window, QColor(0x53, 0x53, 0x53));
  darkPalette->setColor(QPalette::WindowText, QColor(0xDD, 0xDD, 0xDD));
  darkPalette->setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x98, 0x98, 0x98));
  darkPalette->setColor(QPalette::Base, QColor(0x45, 0x45, 0x45));
  darkPalette->setColor(QPalette::Disabled, QPalette::Base, QColor(0x4D, 0x4D, 0x4D));
  darkPalette->setColor(QPalette::AlternateBase, darkPalette->color(QPalette::Window));
  darkPalette->setColor(QPalette::Disabled, QPalette::AlternateBase,
                        darkPalette->color(QPalette::Disabled, QPalette::Window));
  darkPalette->setColor(QPalette::ToolTipBase, QColor(0x70, 0x70, 0x70));
  darkPalette->setColor(QPalette::ToolTipText, darkPalette->color(QPalette::WindowText));
  darkPalette->setColor(QPalette::Text, darkPalette->color(QPalette::WindowText));
  darkPalette->setColor(QPalette::Disabled, QPalette::Text,
                        darkPalette->color(QPalette::Disabled, QPalette::WindowText));
  darkPalette->setColor(QPalette::Light, QColor(0x66, 0x66, 0x66));
  darkPalette->setColor(QPalette::Midlight, QColor(0x53, 0x53, 0x53));
  darkPalette->setColor(QPalette::Dark, QColor(0x40, 0x40, 0x40));
  darkPalette->setColor(QPalette::Mid, QColor(0x33, 0x33, 0x33));
  darkPalette->setColor(QPalette::Shadow, QColor(0x26, 0x26, 0x26));
  darkPalette->setColor(QPalette::Button, darkPalette->color(QPalette::Base));
  darkPalette->setColor(QPalette::Disabled, QPalette::Button, darkPalette->color(QPalette::Disabled, QPalette::Base));
  darkPalette->setColor(QPalette::ButtonText, darkPalette->color(QPalette::WindowText));
  darkPalette->setColor(QPalette::Disabled, QPalette::ButtonText,
                        darkPalette->color(QPalette::Disabled, QPalette::WindowText));
  darkPalette->setColor(QPalette::BrightText, QColor(0xFC, 0x52, 0x48));
  darkPalette->setColor(QPalette::Link, QColor(0x4F, 0x95, 0xFC));
  darkPalette->setColor(QPalette::Highlight, QColor(0x6B, 0x6B, 0x6B));
  darkPalette->setColor(QPalette::Disabled, QPalette::Highlight, QColor(0x5D, 0x5D, 0x5D));
  darkPalette->setColor(QPalette::HighlightedText, darkPalette->color(QPalette::WindowText));
  darkPalette->setColor(QPalette::Disabled, QPalette::HighlightedText,
                        darkPalette->color(QPalette::Disabled, QPalette::WindowText));

  return darkPalette;
}

std::unique_ptr<QString> DarkScheme::getStyleSheet() const {
  std::unique_ptr<QString> qsStylesheet = nullptr;

  QFile qfDarkStyle(QString(":/dark_scheme/stylesheet.qss"));
  if (qfDarkStyle.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qsStylesheet = std::make_unique<QString>(qfDarkStyle.readAll());

    qfDarkStyle.close();
  }

  return qsStylesheet;
}

std::unique_ptr<ColorParams> DarkScheme::getColorParams() const {
  std::unique_ptr<ColorParams> customColors(new ColorParams());

  customColors->insert(
      ColorParams::value_type("thumbnail_sequence_selected_item_background", QColor(0x44, 0x44, 0x44)));
  customColors->insert(ColorParams::value_type("open_new_project_border_color", QColor(0x53, 0x53, 0x53)));
  customColors->insert(ColorParams::value_type("processing_indication_fade_color", QColor(0x28, 0x28, 0x28)));
  customColors->insert(ColorParams::value_type("processing_indication_head_color", QColor(0xDD, 0xDD, 0xDD)));
  customColors->insert(ColorParams::value_type("processing_indication_tail_color", QColor(0x6B, 0x6B, 0x6B)));
  customColors->insert(
      ColorParams::value_type("stage_list_head_color", customColors->at("processing_indication_head_color")));
  customColors->insert(
      ColorParams::value_type("stage_list_tail_color", customColors->at("processing_indication_tail_color")));
  customColors->insert(ColorParams::value_type("fix_dpi_dialog_error_text_color", QColor(0xF3, 0x49, 0x41)));

  return customColors;
}
