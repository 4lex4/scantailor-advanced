/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OUTPUT_COLOR_GRAYSCALE_OPTIONS_H_
#define OUTPUT_COLOR_GRAYSCALE_OPTIONS_H_

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
    bool enabled;
    int level;
    bool normalizationEnabled;
    bool forceBlackAndWhite;
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
  PosterizationOptions posterizationOptions;
};
}  // namespace output
#endif  // ifndef OUTPUT_COLOR_GRAYSCALE_OPTIONS_H_
