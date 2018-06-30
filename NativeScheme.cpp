
#include "NativeScheme.h"
#include <QPalette>

QStyle* NativeScheme::getStyle() const {
  return nullptr;
}

QPalette NativeScheme::getPalette() const {
  return QPalette();
}

std::unique_ptr<QString> NativeScheme::getStyleSheet() const {
  auto styleSheet = std::make_unique<QString>();
  const QPalette palette = getPalette();

  styleSheet->append(QString(
      "QGraphicsView, ImageViewBase, QFrame#imageViewFrame {\n"
      "  background-color: %1;\n"
      "  background-attachment: scroll;\n"
      "}").arg(palette.background().color().darker(115).name()));

  return styleSheet;
}

ColorScheme::ColorParams NativeScheme::getColorParams() const {
  ColorScheme::ColorParams customColors;
  const QPalette palette = getPalette();

  customColors[ThumbnailSequenceSelectedItemBackground] = palette.color(QPalette::Highlight).lighter(130);
  customColors[ThumbnailSequenceSelectionLeaderBackground] = palette.color(QPalette::Highlight);
  customColors[ProcessingIndicationFade] = palette.background().color().darker(115);
  if (palette.window().color().lightnessF() < 0.5) {
    // If system scheme is dark, adapt some colors.
    customColors[ProcessingIndicationHeadColor] = palette.color(QPalette::Window).lighter(200);
    customColors[ProcessingIndicationTail] = palette.color(QPalette::Window).lighter(130);
    customColors[StageListHead] = customColors.at(ProcessingIndicationHeadColor);
    customColors[StageListTail] = customColors.at(ProcessingIndicationTail);
  }

  return customColors;
}
