// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_COLORCOMMONOPTIONS_H_
#define SCANTAILOR_OUTPUT_COLORCOMMONOPTIONS_H_

#include <AutoManualMode.h>

class QString;
class QDomDocument;
class QDomElement;

namespace output {
enum FillingColor { FILL_BACKGROUND, FILL_WHITE };

class ColorCommonOptions {
 public:
  class PosterizationOptions {
   public:
    PosterizationOptions();

    explicit PosterizationOptions(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    bool operator==(const PosterizationOptions& other) const;

    bool operator!=(const PosterizationOptions& other) const;

    bool isEnabled() const;

    void setEnabled(bool enabled);

    int getLevel() const;

    void setLevel(int level);

    bool isNormalizationEnabled() const;

    void setNormalizationEnabled(bool normalizationEnabled);

    bool isForceBlackAndWhite() const;

    void setForceBlackAndWhite(bool forceBlackAndWhite);

   private:
    bool m_isEnabled;
    int m_level;
    bool m_isNormalizationEnabled;
    bool m_forceBlackAndWhite;
  };

  ColorCommonOptions();

  explicit ColorCommonOptions(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool fillOffcut() const;

  void setFillOffcut(bool fillOffcut);

  bool fillMargins() const;

  void setFillMargins(bool val);

  bool normalizeIllumination() const;

  void setNormalizeIllumination(bool val);

  FillingColor getFillingColor() const;

  void setFillingColor(FillingColor fillingColor);

  bool operator==(const ColorCommonOptions& other) const;

  bool operator!=(const ColorCommonOptions& other) const;

  const PosterizationOptions& getPosterizationOptions() const;

  void setPosterizationOptions(const PosterizationOptions& posterizationOptions);

 private:
  static FillingColor parseFillingColor(const QString& str);

  static QString formatFillingColor(FillingColor type);


  bool m_fillOffcut;
  bool m_fillMargins;
  bool m_normalizeIllumination;
  FillingColor m_fillingColor;
  PosterizationOptions m_posterizationOptions;
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_COLORCOMMONOPTIONS_H_
