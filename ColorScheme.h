
#ifndef SCANTAILOR_COLORSCHEME_H
#define SCANTAILOR_COLORSCHEME_H

#include <QtGui/QPalette>
#include <unordered_map>

typedef std::unordered_map<std::string, QColor> ColorParams;

class ColorScheme {
 public:
  virtual ~ColorScheme() = default;

  virtual std::unique_ptr<QPalette> getPalette() const = 0;

  virtual std::unique_ptr<QString> getStyleSheet() const = 0;

  /**
   * List of colors for elements that don't support styling.
   *
   * Available parameters:
   *     thumbnail_sequence_selected_item_text,
   *     thumbnail_sequence_item_text,
   *     thumbnail_sequence_selected_item_background,
   *     relinkable_path_visualization_border_color,
   *     open_new_project_border_color,
   *     processing_indication_head_color,
   *     processing_indication_tail_color,
   *     processing_indication_fade_color,
   *     stage_list_head_color,
   *     stage_list_tail_color,
   *     fix_dpi_dialog_error_text_color
   *
   * @return the list of colors to override the default values by the color scheme
   */
  virtual std::unique_ptr<ColorParams> getColorParams() const = 0;
};

#endif  // SCANTAILOR_COLORSCHEME_H
