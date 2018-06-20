
#ifndef SCANTAILOR_SPLITTINGOPTIONS_H
#define SCANTAILOR_SPLITTINGOPTIONS_H

#include <QtXml/QDomElement>

namespace output {
enum SplittingMode { BLACK_AND_WHITE_FOREGROUND, COLOR_FOREGROUND };

class SplittingOptions {
 public:
  SplittingOptions();

  explicit SplittingOptions(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool isSplitOutput() const;

  void setSplitOutput(bool splitOutput);

  SplittingMode getSplittingMode() const;

  void setSplittingMode(SplittingMode foregroundType);

  bool operator==(const SplittingOptions& other) const;

  bool operator!=(const SplittingOptions& other) const;

  bool isOriginalBackgroundEnabled() const;

  void setOriginalBackgroundEnabled(bool enable);

 private:
  static SplittingMode parseSplittingMode(const QString& str);

  static QString formatSplittingMode(SplittingMode type);

  bool m_isSplitOutput;
  SplittingMode m_splittingMode;
  bool m_isOriginalBackgroundEnabled;
};
}  // namespace output


#endif  // SCANTAILOR_SPLITTINGOPTIONS_H
