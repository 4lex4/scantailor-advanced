/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef OUTPUT_BLACK_WHITE_OPTIONS_H_
#define OUTPUT_BLACK_WHITE_OPTIONS_H_

class QString;
class QDomDocument;
class QDomElement;

namespace output {
enum BinarizationMethod { OTSU, SAUVOLA, WOLF };

class BlackWhiteOptions {
 public:
  class ColorSegmenterOptions {
   public:
    ColorSegmenterOptions();

    explicit ColorSegmenterOptions(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    bool operator==(const ColorSegmenterOptions& other) const;

    bool operator!=(const ColorSegmenterOptions& other) const;

    bool isEnabled() const;

    void setEnabled(bool enabled);

    int getNoiseReduction() const;

    void setNoiseReduction(int noiseReduction);

    int getRedThresholdAdjustment() const;

    void setRedThresholdAdjustment(int redThresholdAdjustment);

    int getGreenThresholdAdjustment() const;

    void setGreenThresholdAdjustment(int greenThresholdAdjustment);

    int getBlueThresholdAdjustment() const;

    void setBlueThresholdAdjustment(int blueThresholdAdjustment);

   private:
    bool enabled;
    int noiseReduction;
    int redThresholdAdjustment;
    int greenThresholdAdjustment;
    int blueThresholdAdjustment;
  };

 public:
  BlackWhiteOptions();

  explicit BlackWhiteOptions(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool operator==(const BlackWhiteOptions& other) const;

  bool operator!=(const BlackWhiteOptions& other) const;

  int thresholdAdjustment() const;

  void setThresholdAdjustment(int val);

  bool normalizeIllumination() const;

  void setNormalizeIllumination(bool val);

  bool isSavitzkyGolaySmoothingEnabled() const;

  void setSavitzkyGolaySmoothingEnabled(bool savitzkyGolaySmoothingEnabled);

  bool isMorphologicalSmoothingEnabled() const;

  void setMorphologicalSmoothingEnabled(bool morphologicalSmoothingEnabled);

  int getWindowSize() const;

  void setWindowSize(int windowSize);

  double getSauvolaCoef() const;

  void setSauvolaCoef(double sauvolaCoef);

  int getWolfLowerBound() const;

  void setWolfLowerBound(int wolfLowerBound);

  int getWolfUpperBound() const;

  void setWolfUpperBound(int wolfUpperBound);

  double getWolfCoef() const;

  void setWolfCoef(double wolfCoef);

  BinarizationMethod getBinarizationMethod() const;

  void setBinarizationMethod(BinarizationMethod binarizationMethod);

  const ColorSegmenterOptions& getColorSegmenterOptions() const;

  void setColorSegmenterOptions(const ColorSegmenterOptions& colorSegmenterOptions);

 private:
  static BinarizationMethod parseBinarizationMethod(const QString& str);

  static QString formatBinarizationMethod(BinarizationMethod type);


  int m_thresholdAdjustment;
  bool savitzkyGolaySmoothingEnabled;
  bool morphologicalSmoothingEnabled;
  bool m_normalizeIllumination;
  int windowSize;
  double sauvolaCoef;
  int wolfLowerBound;
  int wolfUpperBound;
  double wolfCoef;
  BinarizationMethod binarizationMethod;
  ColorSegmenterOptions colorSegmenterOptions;
};
}  // namespace output
#endif  // ifndef OUTPUT_BLACK_WHITE_OPTIONS_H_
