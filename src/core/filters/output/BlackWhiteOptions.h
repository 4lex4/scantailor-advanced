// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_BLACKWHITEOPTIONS_H_
#define SCANTAILOR_OUTPUT_BLACKWHITEOPTIONS_H_

class QString;
class QDomDocument;
class QDomElement;

namespace output {
enum BinarizationMethod { OTSU, SAUVOLA, WOLF, EDGEPLUS, BLURDIV, EDGEDIV };

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


inline bool BlackWhiteOptions::isSavitzkyGolaySmoothingEnabled() const {
  return m_savitzkyGolaySmoothingEnabled;
}

inline void BlackWhiteOptions::setSavitzkyGolaySmoothingEnabled(bool savitzkyGolaySmoothingEnabled) {
  BlackWhiteOptions::m_savitzkyGolaySmoothingEnabled = savitzkyGolaySmoothingEnabled;
}

inline bool BlackWhiteOptions::isMorphologicalSmoothingEnabled() const {
  return m_morphologicalSmoothingEnabled;
}

inline void BlackWhiteOptions::setMorphologicalSmoothingEnabled(bool morphologicalSmoothingEnabled) {
  BlackWhiteOptions::m_morphologicalSmoothingEnabled = morphologicalSmoothingEnabled;
}

inline int BlackWhiteOptions::getWindowSize() const {
  return m_windowSize;
}

inline void BlackWhiteOptions::setWindowSize(int windowSize) {
  BlackWhiteOptions::m_windowSize = windowSize;
}

inline double BlackWhiteOptions::getSauvolaCoef() const {
  return m_sauvolaCoef;
}

inline void BlackWhiteOptions::setSauvolaCoef(double sauvolaCoef) {
  BlackWhiteOptions::m_sauvolaCoef = sauvolaCoef;
}

inline int BlackWhiteOptions::getWolfLowerBound() const {
  return m_wolfLowerBound;
}

inline void BlackWhiteOptions::setWolfLowerBound(int wolfLowerBound) {
  BlackWhiteOptions::m_wolfLowerBound = wolfLowerBound;
}

inline int BlackWhiteOptions::getWolfUpperBound() const {
  return m_wolfUpperBound;
}

inline void BlackWhiteOptions::setWolfUpperBound(int wolfUpperBound) {
  BlackWhiteOptions::m_wolfUpperBound = wolfUpperBound;
}

inline double BlackWhiteOptions::getWolfCoef() const {
  return m_wolfCoef;
}

inline void BlackWhiteOptions::setWolfCoef(double wolfCoef) {
  BlackWhiteOptions::m_wolfCoef = wolfCoef;
}

inline BinarizationMethod BlackWhiteOptions::getBinarizationMethod() const {
  return m_binarizationMethod;
}

inline void BlackWhiteOptions::setBinarizationMethod(BinarizationMethod binarizationMethod) {
  BlackWhiteOptions::m_binarizationMethod = binarizationMethod;
}

inline int BlackWhiteOptions::thresholdAdjustment() const {
  return m_thresholdAdjustment;
}

inline void BlackWhiteOptions::setThresholdAdjustment(int val) {
  m_thresholdAdjustment = val;
}

inline bool BlackWhiteOptions::normalizeIllumination() const {
  return m_normalizeIllumination;
}

inline void BlackWhiteOptions::setNormalizeIllumination(bool val) {
  m_normalizeIllumination = val;
}

inline const BlackWhiteOptions::ColorSegmenterOptions& BlackWhiteOptions::getColorSegmenterOptions() const {
  return m_colorSegmenterOptions;
}

inline void BlackWhiteOptions::setColorSegmenterOptions(
    const BlackWhiteOptions::ColorSegmenterOptions& colorSegmenterOptions) {
  BlackWhiteOptions::m_colorSegmenterOptions = colorSegmenterOptions;
}

inline bool BlackWhiteOptions::ColorSegmenterOptions::isEnabled() const {
  return m_isEnabled;
}

inline void BlackWhiteOptions::ColorSegmenterOptions::setEnabled(bool enabled) {
  ColorSegmenterOptions::m_isEnabled = enabled;
}

inline int BlackWhiteOptions::ColorSegmenterOptions::getNoiseReduction() const {
  return m_noiseReduction;
}

inline void BlackWhiteOptions::ColorSegmenterOptions::setNoiseReduction(int noiseReduction) {
  ColorSegmenterOptions::m_noiseReduction = noiseReduction;
}

inline int BlackWhiteOptions::ColorSegmenterOptions::getRedThresholdAdjustment() const {
  return m_redThresholdAdjustment;
}

inline void BlackWhiteOptions::ColorSegmenterOptions::setRedThresholdAdjustment(int redThresholdAdjustment) {
  ColorSegmenterOptions::m_redThresholdAdjustment = redThresholdAdjustment;
}

inline int BlackWhiteOptions::ColorSegmenterOptions::getGreenThresholdAdjustment() const {
  return m_greenThresholdAdjustment;
}

inline void BlackWhiteOptions::ColorSegmenterOptions::setGreenThresholdAdjustment(int greenThresholdAdjustment) {
  ColorSegmenterOptions::m_greenThresholdAdjustment = greenThresholdAdjustment;
}

inline int BlackWhiteOptions::ColorSegmenterOptions::getBlueThresholdAdjustment() const {
  return m_blueThresholdAdjustment;
}

inline void BlackWhiteOptions::ColorSegmenterOptions::setBlueThresholdAdjustment(int blueThresholdAdjustment) {
  ColorSegmenterOptions::m_blueThresholdAdjustment = blueThresholdAdjustment;
}
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_BLACKWHITEOPTIONS_H_
