
#include "SplittingOptions.h"

namespace output {

SplittingOptions::SplittingOptions()
    : splitOutput(false), splittingMode(BLACK_AND_WHITE_FOREGROUND), originalBackground(false) {}

SplittingOptions::SplittingOptions(const QDomElement& el)
    : splitOutput(el.attribute("splitOutput") == "1"),
      splittingMode(parseSplittingMode(el.attribute("splittingMode"))),
      originalBackground(el.attribute("originalBackground") == "1") {}

QDomElement SplittingOptions::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("splitOutput", splitOutput ? "1" : "0");
  el.setAttribute("splittingMode", formatSplittingMode(splittingMode));
  el.setAttribute("originalBackground", originalBackground ? "1" : "0");

  return el;
}

bool SplittingOptions::isSplitOutput() const {
  return splitOutput;
}

void SplittingOptions::setSplitOutput(bool splitOutput) {
  SplittingOptions::splitOutput = splitOutput;
}

SplittingMode SplittingOptions::getSplittingMode() const {
  return splittingMode;
}

void SplittingOptions::setSplittingMode(SplittingMode foregroundType) {
  SplittingOptions::splittingMode = foregroundType;
}

bool SplittingOptions::isOriginalBackground() const {
  return originalBackground;
}

void SplittingOptions::setOriginalBackground(bool originalBackground) {
  SplittingOptions::originalBackground = originalBackground;
}

SplittingMode SplittingOptions::parseSplittingMode(const QString& str) {
  if (str == "color") {
    return COLOR_FOREGROUND;
  } else {
    return BLACK_AND_WHITE_FOREGROUND;
  }
}

QString SplittingOptions::formatSplittingMode(const SplittingMode type) {
  QString str = "";
  switch (type) {
    case BLACK_AND_WHITE_FOREGROUND:
      str = "bw";
      break;
    case COLOR_FOREGROUND:
      str = "color";
      break;
  }

  return str;
}

bool SplittingOptions::operator==(const SplittingOptions& other) const {
  return (splitOutput == other.splitOutput) && (splittingMode == other.splittingMode)
         && (originalBackground == other.originalBackground);
}

bool SplittingOptions::operator!=(const SplittingOptions& other) const {
  return !(*this == other);
}
}  // namespace output