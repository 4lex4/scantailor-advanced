// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
    bool m_isEnabled;
    int m_noiseReduction;
    int m_redThresholdAdjustment;
    int m_greenThresholdAdjustment;
    int m_blueThresholdAdjustment;
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
  bool m_savitzkyGolaySmoothingEnabled;
  bool m_morphologicalSmoothingEnabled;
  bool m_normalizeIllumination;
  int m_windowSize;
  double m_sauvolaCoef;
  int m_wolfLowerBound;
  int m_wolfUpperBound;
  double m_wolfCoef;
  BinarizationMethod m_binarizationMethod;
  ColorSegmenterOptions m_colorSegmenterOptions;
};
}  // namespace output
#endif  // ifndef OUTPUT_BLACK_WHITE_OPTIONS_H_
