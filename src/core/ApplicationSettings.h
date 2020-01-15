// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_APPLICATIONSETTINGS_H_
#define SCANTAILOR_CORE_APPLICATIONSETTINGS_H_


#include <foundation/NonCopyable.h>

#include <QSettings>
#include <QSize>
#include <QSizeF>
#include <QString>

class ApplicationSettings {
  DECLARE_NON_COPYABLE(ApplicationSettings)
 private:
  ApplicationSettings();

 public:
  static ApplicationSettings& getInstance();

  bool isOpenGlEnabled() const;

  void setOpenGlEnabled(bool enabled);

  QString getColorScheme() const;

  void setColorScheme(const QString& scheme);

  bool isAutoSaveProjectEnabled() const;

  void setAutoSaveProjectEnabled(bool enabled);

  int getTiffBwCompression() const;

  void setTiffBwCompression(int compression);

  int getTiffColorCompression() const;

  void setTiffColorCompression(int compression);

  bool isBlackOnWhiteDetectionEnabled() const;

  void setBlackOnWhiteDetectionEnabled(bool enabled);

  bool isBlackOnWhiteDetectionOutputEnabled() const;

  void setBlackOnWhiteDetectionOutputEnabled(bool enabled);

  bool isHighlightDeviationEnabled() const;

  void setHighlightDeviationEnabled(bool enabled);

  double getDeskewDeviationCoef() const;

  void setDeskewDeviationCoef(double value);

  double getDeskewDeviationThreshold() const;

  void setDeskewDeviationThreshold(double value);

  double getSelectContentDeviationCoef() const;

  void setSelectContentDeviationCoef(double value);

  double getSelectContentDeviationThreshold() const;

  void setSelectContentDeviationThreshold(double value);

  double getMarginsDeviationCoef() const;

  void setMarginsDeviationCoef(double value);

  double getMarginsDeviationThreshold() const;

  void setMarginsDeviationThreshold(double value);

  QSize getThumbnailQuality() const;

  void setThumbnailQuality(const QSize& quality);

  QSizeF getMaxLogicalThumbnailSize() const;

  void setMaxLogicalThumbnailSize(const QSizeF& size);

  bool isSingleColumnThumbnailDisplayEnabled() const;

  void setSingleColumnThumbnailDisplayEnabled(bool enabled);

  QString getLanguage() const;

  void setLanguage(const QString& language);

  QString getUnits() const;

  void setUnits(const QString& units);

  QString getCurrentProfile() const;

  void setCurrentProfile(const QString& profile);

  bool isCancelingSelectionQuestionEnabled();

  void setCancelingSelectionQuestionEnabled(bool enabled);

 private:
  static inline QString getKey(const QString& keyName);

  static const bool DEFAULT_OPENGL_STATE;
  static const QString DEFAULT_COLOR_SCHEME;
  static const bool DEFAULT_AUTO_SAVE_PROJECT;
  static const int DEFAULT_TIFF_BW_COMPRESSION;
  static const int DEFAULT_TIFF_COLOR_COMPRESSION;
  static const bool DEFAULT_BLACK_ON_WHITE_DETECTION;
  static const bool DEFAULT_BLACK_ON_WHITE_DETECTION_OUTPUT;
  static const bool DEFAULT_HIGHLIGHT_DEVIATION;
  static const double DEFAULT_DESKEW_DEVIATION_COEF;
  static const double DEFAULT_DESKEW_DEVIATION_THRESHOLD;
  static const double DEFAULT_SELECT_CONTENT_DEVIATION_COEF;
  static const double DEFAULT_SELECT_CONTENT_DEVIATION_THRESHOLD;
  static const double DEFAULT_MARGINS_DEVIATION_COEF;
  static const double DEFAULT_MARGINS_DEVIATION_THRESHOLD;
  static const QSize DEFAULT_THUMBNAIL_QUALITY;
  static const QSizeF DEFAULT_MAX_LOGICAL_THUMBNAIL_SIZE;
  static const bool DEFAULT_SINGLE_COLUMN_THUMBNAIL_DISPLAY;
  static const QString DEFAULT_LANGUAGE;
  static const QString DEFAULT_UNITS;
  static const QString DEFAULT_PROFILE;
  static const bool DEFAULT_SHOW_CANCELING_SELECTION_QUESTION;

  static const QString ROOT_KEY;
  static const QString OPENGL_STATE_KEY;
  static const QString AUTO_SAVE_PROJECT_KEY;
  static const QString COLOR_SCHEME_KEY;
  static const QString TIFF_BW_COMPRESSION_KEY;
  static const QString TIFF_COLOR_COMPRESSION_KEY;
  static const QString BLACK_ON_WHITE_DETECTION_KEY;
  static const QString BLACK_ON_WHITE_DETECTION_OUTPUT_KEY;
  static const QString HIGHLIGHT_DEVIATION_KEY;
  static const QString DESKEW_DEVIATION_COEF_KEY;
  static const QString DESKEW_DEVIATION_THRESHOLD_KEY;
  static const QString SELECT_CONTENT_DEVIATION_COEF_KEY;
  static const QString SELECT_CONTENT_DEVIATION_THRESHOLD_KEY;
  static const QString MARGINS_DEVIATION_COEF_KEY;
  static const QString MARGINS_DEVIATION_THRESHOLD_KEY;
  static const QString THUMBNAIL_QUALITY_KEY;
  static const QString MAX_LOGICAL_THUMBNAIL_SIZE_KEY;
  static const QString SINGLE_COLUMN_THUMBNAIL_DISPLAY_KEY;
  static const QString LANGUAGE_KEY;
  static const QString UNITS_KEY;
  static const QString CURRENT_PROFILE_KEY;
  static const QString SHOW_CANCELING_SELECTION_QUESTION_KEY;

  QSettings m_settings;
};


#endif  // SCANTAILOR_CORE_APPLICATIONSETTINGS_H_
