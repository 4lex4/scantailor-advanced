
#ifndef SCANTAILOR_COLORSCHEME_H
#define SCANTAILOR_COLORSCHEME_H

#include <QtGui/QPalette>
#include <unordered_map>

class ColorScheme {
 public:
  virtual ~ColorScheme() = default;

  virtual QPalette getPalette() const = 0;

  virtual std::unique_ptr<QString> getStyleSheet() const = 0;

  enum ColorParam {
    ThumbnailSequenceItemText,
    ThumbnailSequenceSelectedItemText,
    ThumbnailSequenceSelectedItemBackground,
    ThumbnailSequenceSelectionLeaderText,
    ThumbnailSequenceSelectionLeaderBackground,
    RelinkablePathVisualizationBorder,
    OpenNewProjectBorder,
    ProcessingIndicationHeadColor,
    ProcessingIndicationTail,
    ProcessingIndicationFade,
    StageListHead,
    StageListTail,
    FixDpiDialogErrorText
  };

  using ColorParams = std::unordered_map<ColorParam, QColor>;

  /**
   * List of colors for elements that don't support styling.
   *
   * @return the list of colors to override the default values by the color scheme
   */
  virtual ColorParams getColorParams() const = 0;
};

#endif  // SCANTAILOR_COLORSCHEME_H
