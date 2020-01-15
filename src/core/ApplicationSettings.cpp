// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ApplicationSettings.h"

#include <tiff.h>

#include <QLocale>
#include <QtCore/QSettings>

const bool ApplicationSettings::DEFAULT_OPENGL_STATE = false;
const QString ApplicationSettings::DEFAULT_COLOR_SCHEME = "dark";
const bool ApplicationSettings::DEFAULT_AUTO_SAVE_PROJECT = false;
const int ApplicationSettings::DEFAULT_TIFF_BW_COMPRESSION = COMPRESSION_CCITTFAX4;
const int ApplicationSettings::DEFAULT_TIFF_COLOR_COMPRESSION = COMPRESSION_LZW;
const bool ApplicationSettings::DEFAULT_BLACK_ON_WHITE_DETECTION = true;
const bool ApplicationSettings::DEFAULT_BLACK_ON_WHITE_DETECTION_OUTPUT = true;
const bool ApplicationSettings::DEFAULT_HIGHLIGHT_DEVIATION = true;
const double ApplicationSettings::DEFAULT_DESKEW_DEVIATION_COEF = 1.5;
const double ApplicationSettings::DEFAULT_DESKEW_DEVIATION_THRESHOLD = 1.0;
const double ApplicationSettings::DEFAULT_SELECT_CONTENT_DEVIATION_COEF = 0.35;
const double ApplicationSettings::DEFAULT_SELECT_CONTENT_DEVIATION_THRESHOLD = 1.0;
const double ApplicationSettings::DEFAULT_MARGINS_DEVIATION_COEF = 0.35;
const double ApplicationSettings::DEFAULT_MARGINS_DEVIATION_THRESHOLD = 1.0;
const QSize ApplicationSettings::DEFAULT_THUMBNAIL_QUALITY = QSize(200, 200);
const QSizeF ApplicationSettings::DEFAULT_MAX_LOGICAL_THUMBNAIL_SIZE = QSizeF(250, 160);
const bool ApplicationSettings::DEFAULT_SINGLE_COLUMN_THUMBNAIL_DISPLAY = false;
const QString ApplicationSettings::DEFAULT_LANGUAGE = QLocale::system().name();
const QString ApplicationSettings::DEFAULT_UNITS = "mm";
const QString ApplicationSettings::DEFAULT_PROFILE = "Default";
const bool ApplicationSettings::DEFAULT_SHOW_CANCELING_SELECTION_QUESTION = true;

const QString ApplicationSettings::ROOT_KEY = "settings";
const QString ApplicationSettings::OPENGL_STATE_KEY = "enable_opengl";
const QString ApplicationSettings::AUTO_SAVE_PROJECT_KEY = "auto_save_project";
const QString ApplicationSettings::COLOR_SCHEME_KEY = "color_scheme";
const QString ApplicationSettings::TIFF_BW_COMPRESSION_KEY = "bw_compression";
const QString ApplicationSettings::TIFF_COLOR_COMPRESSION_KEY = "color_compression";
const QString ApplicationSettings::BLACK_ON_WHITE_DETECTION_KEY = "black_on_white_detection";
const QString ApplicationSettings::BLACK_ON_WHITE_DETECTION_OUTPUT_KEY = "black_on_white_detection_at_output";
const QString ApplicationSettings::HIGHLIGHT_DEVIATION_KEY = "highlight_deviation";
const QString ApplicationSettings::DESKEW_DEVIATION_COEF_KEY = "deskew_deviation_coef";
const QString ApplicationSettings::DESKEW_DEVIATION_THRESHOLD_KEY = "deskew_deviation_threshold";
const QString ApplicationSettings::SELECT_CONTENT_DEVIATION_COEF_KEY = "select_content_deviation_coef";
const QString ApplicationSettings::SELECT_CONTENT_DEVIATION_THRESHOLD_KEY = "select_content_deviation_threshold";
const QString ApplicationSettings::MARGINS_DEVIATION_COEF_KEY = "margins_deviation_coef";
const QString ApplicationSettings::MARGINS_DEVIATION_THRESHOLD_KEY = "margins_deviation_threshold";
const QString ApplicationSettings::THUMBNAIL_QUALITY_KEY = "thumbnail_quality";
const QString ApplicationSettings::MAX_LOGICAL_THUMBNAIL_SIZE_KEY = "max_logical_thumb_size";
const QString ApplicationSettings::SINGLE_COLUMN_THUMBNAIL_DISPLAY_KEY = "single_column_thumbnail_display";
const QString ApplicationSettings::LANGUAGE_KEY = "language";
const QString ApplicationSettings::UNITS_KEY = "units";
const QString ApplicationSettings::CURRENT_PROFILE_KEY = "current_profile";
const QString ApplicationSettings::SHOW_CANCELING_SELECTION_QUESTION_KEY = "selection_canceling_question";

QString ApplicationSettings::getKey(const QString& keyName) {
  return ApplicationSettings::ROOT_KEY + '/' + keyName;
}

ApplicationSettings::ApplicationSettings() = default;

ApplicationSettings& ApplicationSettings::getInstance() {
  static ApplicationSettings instance;
  return instance;
}

bool ApplicationSettings::isOpenGlEnabled() const {
  return m_settings.value(getKey(OPENGL_STATE_KEY), DEFAULT_OPENGL_STATE).toBool();
}

void ApplicationSettings::setOpenGlEnabled(bool enabled) {
  m_settings.setValue(getKey(OPENGL_STATE_KEY), enabled);
}

QString ApplicationSettings::getColorScheme() const {
  return m_settings.value(getKey(COLOR_SCHEME_KEY), DEFAULT_COLOR_SCHEME).toString();
}

void ApplicationSettings::setColorScheme(const QString& scheme) {
  m_settings.setValue(getKey(COLOR_SCHEME_KEY), scheme);
}

bool ApplicationSettings::isAutoSaveProjectEnabled() const {
  return m_settings.value(getKey(AUTO_SAVE_PROJECT_KEY), DEFAULT_AUTO_SAVE_PROJECT).toBool();
}

void ApplicationSettings::setAutoSaveProjectEnabled(bool enabled) {
  m_settings.setValue(getKey(AUTO_SAVE_PROJECT_KEY), enabled);
}

int ApplicationSettings::getTiffBwCompression() const {
  return m_settings.value(getKey(TIFF_BW_COMPRESSION_KEY), DEFAULT_TIFF_BW_COMPRESSION).toInt();
}

void ApplicationSettings::setTiffBwCompression(int compression) {
  m_settings.setValue(getKey(TIFF_BW_COMPRESSION_KEY), compression);
}

int ApplicationSettings::getTiffColorCompression() const {
  return m_settings.value(getKey(TIFF_COLOR_COMPRESSION_KEY), DEFAULT_TIFF_COLOR_COMPRESSION).toInt();
}

void ApplicationSettings::setTiffColorCompression(int compression) {
  m_settings.setValue(getKey(TIFF_COLOR_COMPRESSION_KEY), compression);
}

bool ApplicationSettings::isBlackOnWhiteDetectionEnabled() const {
  return m_settings.value(getKey(BLACK_ON_WHITE_DETECTION_KEY), DEFAULT_BLACK_ON_WHITE_DETECTION).toBool();
}

void ApplicationSettings::setBlackOnWhiteDetectionEnabled(bool enabled) {
  m_settings.setValue(getKey(BLACK_ON_WHITE_DETECTION_KEY), enabled);
}

bool ApplicationSettings::isBlackOnWhiteDetectionOutputEnabled() const {
  return m_settings.value(getKey(BLACK_ON_WHITE_DETECTION_OUTPUT_KEY), DEFAULT_BLACK_ON_WHITE_DETECTION_OUTPUT)
      .toBool();
}

void ApplicationSettings::setBlackOnWhiteDetectionOutputEnabled(bool enabled) {
  m_settings.setValue(getKey(BLACK_ON_WHITE_DETECTION_OUTPUT_KEY), enabled);
}

bool ApplicationSettings::isHighlightDeviationEnabled() const {
  return m_settings.value(getKey(HIGHLIGHT_DEVIATION_KEY), DEFAULT_HIGHLIGHT_DEVIATION).toBool();
}

void ApplicationSettings::setHighlightDeviationEnabled(bool enabled) {
  m_settings.setValue(getKey(HIGHLIGHT_DEVIATION_KEY), enabled);
}

double ApplicationSettings::getDeskewDeviationCoef() const {
  return m_settings.value(getKey(DESKEW_DEVIATION_COEF_KEY), DEFAULT_DESKEW_DEVIATION_COEF).toDouble();
}

void ApplicationSettings::setDeskewDeviationCoef(double value) {
  m_settings.setValue(getKey(DESKEW_DEVIATION_COEF_KEY), value);
}

double ApplicationSettings::getDeskewDeviationThreshold() const {
  return m_settings.value(getKey(DESKEW_DEVIATION_THRESHOLD_KEY), DEFAULT_DESKEW_DEVIATION_THRESHOLD).toDouble();
}

void ApplicationSettings::setDeskewDeviationThreshold(double value) {
  m_settings.setValue(getKey(DESKEW_DEVIATION_THRESHOLD_KEY), value);
}

double ApplicationSettings::getSelectContentDeviationCoef() const {
  return m_settings.value(getKey(SELECT_CONTENT_DEVIATION_COEF_KEY), DEFAULT_SELECT_CONTENT_DEVIATION_COEF).toDouble();
}

void ApplicationSettings::setSelectContentDeviationCoef(double value) {
  m_settings.setValue(getKey(SELECT_CONTENT_DEVIATION_COEF_KEY), value);
}

double ApplicationSettings::getSelectContentDeviationThreshold() const {
  return m_settings.value(getKey(SELECT_CONTENT_DEVIATION_THRESHOLD_KEY), DEFAULT_SELECT_CONTENT_DEVIATION_THRESHOLD)
      .toDouble();
}

void ApplicationSettings::setSelectContentDeviationThreshold(double value) {
  m_settings.setValue(getKey(SELECT_CONTENT_DEVIATION_THRESHOLD_KEY), value);
}

double ApplicationSettings::getMarginsDeviationCoef() const {
  return m_settings.value(getKey(MARGINS_DEVIATION_COEF_KEY), DEFAULT_MARGINS_DEVIATION_COEF).toDouble();
}

void ApplicationSettings::setMarginsDeviationCoef(double value) {
  m_settings.setValue(getKey(MARGINS_DEVIATION_COEF_KEY), value);
}

double ApplicationSettings::getMarginsDeviationThreshold() const {
  return m_settings.value(getKey(MARGINS_DEVIATION_THRESHOLD_KEY), DEFAULT_MARGINS_DEVIATION_THRESHOLD).toDouble();
}

void ApplicationSettings::setMarginsDeviationThreshold(double value) {
  m_settings.setValue(getKey(MARGINS_DEVIATION_THRESHOLD_KEY), value);
}

QSize ApplicationSettings::getThumbnailQuality() const {
  return m_settings.value(getKey(THUMBNAIL_QUALITY_KEY), DEFAULT_THUMBNAIL_QUALITY).toSize();
}

void ApplicationSettings::setThumbnailQuality(const QSize& quality) {
  m_settings.setValue(getKey(THUMBNAIL_QUALITY_KEY), quality);
}

QSizeF ApplicationSettings::getMaxLogicalThumbnailSize() const {
  return m_settings.value(getKey(MAX_LOGICAL_THUMBNAIL_SIZE_KEY), DEFAULT_MAX_LOGICAL_THUMBNAIL_SIZE).toSizeF();
}

void ApplicationSettings::setMaxLogicalThumbnailSize(const QSizeF& size) {
  m_settings.setValue(getKey(MAX_LOGICAL_THUMBNAIL_SIZE_KEY), size);
}

bool ApplicationSettings::isSingleColumnThumbnailDisplayEnabled() const {
  return m_settings.value(getKey(SINGLE_COLUMN_THUMBNAIL_DISPLAY_KEY), DEFAULT_SINGLE_COLUMN_THUMBNAIL_DISPLAY)
      .toBool();
}

void ApplicationSettings::setSingleColumnThumbnailDisplayEnabled(bool enabled) {
  m_settings.setValue(getKey(SINGLE_COLUMN_THUMBNAIL_DISPLAY_KEY), enabled);
}

QString ApplicationSettings::getLanguage() const {
  return m_settings.value(getKey(LANGUAGE_KEY), DEFAULT_LANGUAGE).toString();
}

void ApplicationSettings::setLanguage(const QString& language) {
  m_settings.setValue(getKey(LANGUAGE_KEY), language);
}

QString ApplicationSettings::getUnits() const {
  return m_settings.value(getKey(UNITS_KEY), DEFAULT_UNITS).toString();
}

void ApplicationSettings::setUnits(const QString& units) {
  m_settings.setValue(getKey(UNITS_KEY), units);
}

QString ApplicationSettings::getCurrentProfile() const {
  return m_settings.value(getKey(CURRENT_PROFILE_KEY), DEFAULT_PROFILE).toString();
}

void ApplicationSettings::setCurrentProfile(const QString& profile) {
  m_settings.setValue(getKey(CURRENT_PROFILE_KEY), profile);
}

bool ApplicationSettings::isCancelingSelectionQuestionEnabled() {
  return m_settings.value(getKey(SHOW_CANCELING_SELECTION_QUESTION_KEY), DEFAULT_SHOW_CANCELING_SELECTION_QUESTION)
      .toBool();
}

void ApplicationSettings::setCancelingSelectionQuestionEnabled(bool enabled) {
  m_settings.setValue(getKey(SHOW_CANCELING_SELECTION_QUESTION_KEY), enabled);
}
