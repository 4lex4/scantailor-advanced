// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_DEFAULTPARAMS_H_
#define SCANTAILOR_CORE_DEFAULTPARAMS_H_


#include <filters/output/ColorParams.h>
#include <filters/output/DepthPerception.h>
#include <filters/output/DespeckleLevel.h>
#include <filters/output/DewarpingOptions.h>
#include <filters/output/PictureShapeOptions.h>
#include <filters/page_layout/Alignment.h>
#include <filters/page_split/LayoutType.h>

#include <QtCore/QRectF>

#include "AutoManualMode.h"
#include "Dpi.h"
#include "Margins.h"
#include "OrthogonalRotation.h"
#include "Units.h"

class DefaultParams {
 public:
  class FixOrientationParams {
   public:
    FixOrientationParams() = default;

    explicit FixOrientationParams(const OrthogonalRotation& imageRotation);

    explicit FixOrientationParams(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    const OrthogonalRotation& getImageRotation() const;

    void setImageRotation(const OrthogonalRotation& imageRotation);

   private:
    OrthogonalRotation m_imageRotation;
  };

  class DeskewParams {
   public:
    DeskewParams();

    explicit DeskewParams(double deskewAngleDeg, AutoManualMode mode);

    explicit DeskewParams(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    double getDeskewAngleDeg() const;

    void setDeskewAngleDeg(double deskewAngleDeg);

    AutoManualMode getMode() const;

    void setMode(AutoManualMode mode);

   private:
    double m_deskewAngleDeg;
    AutoManualMode m_mode;
  };

  class PageSplitParams {
   public:
    PageSplitParams();

    explicit PageSplitParams(page_split::LayoutType layoutType);

    explicit PageSplitParams(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    page_split::LayoutType getLayoutType() const;

    void setLayoutType(page_split::LayoutType layoutType);

   private:
    page_split::LayoutType m_layoutType;
  };

  class SelectContentParams {
   public:
    SelectContentParams();

    SelectContentParams(const QSizeF& pageRectSize,
                        bool contentDetectEnabled,
                        AutoManualMode pageDetectMode,
                        bool fineTuneCorners);

    explicit SelectContentParams(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    const QSizeF& getPageRectSize() const;

    void setPageRectSize(const QSizeF& pageRectSize);

    bool isContentDetectEnabled() const;

    void setContentDetectEnabled(bool contentDetectEnabled);

    bool isFineTuneCorners() const;

    void setFineTuneCorners(bool fineTuneCorners);

    AutoManualMode getPageDetectMode() const;

    void setPageDetectMode(AutoManualMode pageDetectMode);

   private:
    QSizeF m_pageRectSize;
    bool m_contentDetectEnabled;
    AutoManualMode m_pageDetectMode;
    bool m_fineTuneCorners;
  };

  class PageLayoutParams {
   public:
    PageLayoutParams();

    PageLayoutParams(const Margins& hardMargins, const page_layout::Alignment& alignment, bool autoMargins);

    explicit PageLayoutParams(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    const Margins& getHardMargins() const;

    void setHardMargins(const Margins& hardMargins);

    const page_layout::Alignment& getAlignment() const;

    void setAlignment(const page_layout::Alignment& alignment);

    bool isAutoMargins() const;

    void setAutoMargins(bool autoMargins);

   private:
    Margins m_hardMargins;
    page_layout::Alignment m_alignment;
    bool m_autoMargins;
  };

  class OutputParams {
   public:
    OutputParams();

    OutputParams(const Dpi& dpi,
                 const output::ColorParams& colorParams,
                 const output::SplittingOptions& splittingOptions,
                 const output::PictureShapeOptions& pictureShapeOptions,
                 const output::DepthPerception& depthPerception,
                 const output::DewarpingOptions& dewarpingOptions,
                 double despeckleLevel);

    explicit OutputParams(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    const Dpi& getDpi() const;

    void setDpi(const Dpi& dpi);

    const output::ColorParams& getColorParams() const;

    void setColorParams(const output::ColorParams& colorParams);

    const output::SplittingOptions& getSplittingOptions() const;

    void setSplittingOptions(const output::SplittingOptions& splittingOptions);

    const output::PictureShapeOptions& getPictureShapeOptions() const;

    void setPictureShapeOptions(const output::PictureShapeOptions& pictureShapeOptions);

    const output::DepthPerception& getDepthPerception() const;

    void setDepthPerception(const output::DepthPerception& depthPerception);

    const output::DewarpingOptions& getDewarpingOptions() const;

    void setDewarpingOptions(const output::DewarpingOptions& dewarpingOptions);

    double getDespeckleLevel() const;

    void setDespeckleLevel(double despeckleLevel);

   private:
    Dpi m_dpi;
    output::ColorParams m_colorParams;
    output::SplittingOptions m_splittingOptions;
    output::PictureShapeOptions m_pictureShapeOptions;
    output::DepthPerception m_depthPerception;
    output::DewarpingOptions m_dewarpingOptions;
    double m_despeckleLevel;
  };

 public:
  DefaultParams();

  DefaultParams(const FixOrientationParams& fixOrientationParams,
                const DeskewParams& deskewParams,
                const PageSplitParams& pageSplitParams,
                const SelectContentParams& selectContentParams,
                const PageLayoutParams& pageLayoutParams,
                const OutputParams& outputParams);

  explicit DefaultParams(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  const FixOrientationParams& getFixOrientationParams() const;

  void setFixOrientationParams(const FixOrientationParams& fixOrientationParams);

  const DeskewParams& getDeskewParams() const;

  void setDeskewParams(const DeskewParams& deskewParams);

  const PageSplitParams& getPageSplitParams() const;

  void setPageSplitParams(const PageSplitParams& pageSplitParams);

  const SelectContentParams& getSelectContentParams() const;

  void setSelectContentParams(const SelectContentParams& selectContentParams);

  const PageLayoutParams& getPageLayoutParams() const;

  void setPageLayoutParams(const PageLayoutParams& pageLayoutParams);

  const OutputParams& getOutputParams() const;

  void setOutputParams(const OutputParams& outputParams);

  Units getUnits() const;

  void setUnits(Units units);

 private:
  FixOrientationParams m_fixOrientationParams;
  DeskewParams m_deskewParams;
  PageSplitParams m_pageSplitParams;
  SelectContentParams m_selectContentParams;
  PageLayoutParams m_pageLayoutParams;
  OutputParams m_outputParams;
  Units m_units;
};


inline Units DefaultParams::getUnits() const {
  return m_units;
}

inline void DefaultParams::setUnits(Units units) {
  DefaultParams::m_units = units;
}

inline const DefaultParams::FixOrientationParams& DefaultParams::getFixOrientationParams() const {
  return m_fixOrientationParams;
}

inline void DefaultParams::setFixOrientationParams(const DefaultParams::FixOrientationParams& fixOrientationParams) {
  DefaultParams::m_fixOrientationParams = fixOrientationParams;
}

inline const DefaultParams::DeskewParams& DefaultParams::getDeskewParams() const {
  return m_deskewParams;
}

inline void DefaultParams::setDeskewParams(const DefaultParams::DeskewParams& deskewParams) {
  DefaultParams::m_deskewParams = deskewParams;
}

inline const DefaultParams::PageSplitParams& DefaultParams::getPageSplitParams() const {
  return m_pageSplitParams;
}

inline void DefaultParams::setPageSplitParams(const DefaultParams::PageSplitParams& pageSplitParams) {
  DefaultParams::m_pageSplitParams = pageSplitParams;
}

inline const DefaultParams::SelectContentParams& DefaultParams::getSelectContentParams() const {
  return m_selectContentParams;
}

inline void DefaultParams::setSelectContentParams(const DefaultParams::SelectContentParams& selectContentParams) {
  DefaultParams::m_selectContentParams = selectContentParams;
}

inline const DefaultParams::PageLayoutParams& DefaultParams::getPageLayoutParams() const {
  return m_pageLayoutParams;
}

inline void DefaultParams::setPageLayoutParams(const DefaultParams::PageLayoutParams& pageLayoutParams) {
  DefaultParams::m_pageLayoutParams = pageLayoutParams;
}

inline const DefaultParams::OutputParams& DefaultParams::getOutputParams() const {
  return m_outputParams;
}

inline void DefaultParams::setOutputParams(const DefaultParams::OutputParams& outputParams) {
  DefaultParams::m_outputParams = outputParams;
}

inline const OrthogonalRotation& DefaultParams::FixOrientationParams::getImageRotation() const {
  return m_imageRotation;
}

inline void DefaultParams::FixOrientationParams::setImageRotation(const OrthogonalRotation& imageRotation) {
  FixOrientationParams::m_imageRotation = imageRotation;
}

inline double DefaultParams::DeskewParams::getDeskewAngleDeg() const {
  return m_deskewAngleDeg;
}

inline void DefaultParams::DeskewParams::setDeskewAngleDeg(double deskewAngleDeg) {
  DeskewParams::m_deskewAngleDeg = deskewAngleDeg;
}

inline AutoManualMode DefaultParams::DeskewParams::getMode() const {
  return m_mode;
}

inline void DefaultParams::DeskewParams::setMode(AutoManualMode mode) {
  DeskewParams::m_mode = mode;
}

inline page_split::LayoutType DefaultParams::PageSplitParams::getLayoutType() const {
  return m_layoutType;
}

inline void DefaultParams::PageSplitParams::setLayoutType(page_split::LayoutType layoutType) {
  PageSplitParams::m_layoutType = layoutType;
}

inline const QSizeF& DefaultParams::SelectContentParams::getPageRectSize() const {
  return m_pageRectSize;
}

inline void DefaultParams::SelectContentParams::setPageRectSize(const QSizeF& pageRectSize) {
  SelectContentParams::m_pageRectSize = pageRectSize;
}

inline bool DefaultParams::SelectContentParams::isContentDetectEnabled() const {
  return m_contentDetectEnabled;
}

inline void DefaultParams::SelectContentParams::setContentDetectEnabled(bool contentDetectEnabled) {
  SelectContentParams::m_contentDetectEnabled = contentDetectEnabled;
}

inline bool DefaultParams::SelectContentParams::isFineTuneCorners() const {
  return m_fineTuneCorners;
}

inline void DefaultParams::SelectContentParams::setFineTuneCorners(bool fineTuneCorners) {
  SelectContentParams::m_fineTuneCorners = fineTuneCorners;
}

inline AutoManualMode DefaultParams::SelectContentParams::getPageDetectMode() const {
  return m_pageDetectMode;
}

inline void DefaultParams::SelectContentParams::setPageDetectMode(AutoManualMode pageDetectMode) {
  SelectContentParams::m_pageDetectMode = pageDetectMode;
}

inline const Margins& DefaultParams::PageLayoutParams::getHardMargins() const {
  return m_hardMargins;
}

inline void DefaultParams::PageLayoutParams::setHardMargins(const Margins& hardMargins) {
  PageLayoutParams::m_hardMargins = hardMargins;
}

inline const page_layout::Alignment& DefaultParams::PageLayoutParams::getAlignment() const {
  return m_alignment;
}

inline void DefaultParams::PageLayoutParams::setAlignment(const page_layout::Alignment& alignment) {
  PageLayoutParams::m_alignment = alignment;
}

inline bool DefaultParams::PageLayoutParams::isAutoMargins() const {
  return m_autoMargins;
}

inline void DefaultParams::PageLayoutParams::setAutoMargins(bool autoMargins) {
  PageLayoutParams::m_autoMargins = autoMargins;
}

inline const Dpi& DefaultParams::OutputParams::getDpi() const {
  return m_dpi;
}

inline void DefaultParams::OutputParams::setDpi(const Dpi& dpi) {
  OutputParams::m_dpi = dpi;
}

inline const output::ColorParams& DefaultParams::OutputParams::getColorParams() const {
  return m_colorParams;
}

inline void DefaultParams::OutputParams::setColorParams(const output::ColorParams& colorParams) {
  OutputParams::m_colorParams = colorParams;
}

inline const output::SplittingOptions& DefaultParams::OutputParams::getSplittingOptions() const {
  return m_splittingOptions;
}

inline void DefaultParams::OutputParams::setSplittingOptions(const output::SplittingOptions& splittingOptions) {
  OutputParams::m_splittingOptions = splittingOptions;
}

inline const output::PictureShapeOptions& DefaultParams::OutputParams::getPictureShapeOptions() const {
  return m_pictureShapeOptions;
}

inline void DefaultParams::OutputParams::setPictureShapeOptions(
    const output::PictureShapeOptions& pictureShapeOptions) {
  OutputParams::m_pictureShapeOptions = pictureShapeOptions;
}

inline const output::DepthPerception& DefaultParams::OutputParams::getDepthPerception() const {
  return m_depthPerception;
}

inline void DefaultParams::OutputParams::setDepthPerception(const output::DepthPerception& depthPerception) {
  OutputParams::m_depthPerception = depthPerception;
}

inline const output::DewarpingOptions& DefaultParams::OutputParams::getDewarpingOptions() const {
  return m_dewarpingOptions;
}

inline void DefaultParams::OutputParams::setDewarpingOptions(const output::DewarpingOptions& dewarpingOptions) {
  OutputParams::m_dewarpingOptions = dewarpingOptions;
}

inline double DefaultParams::OutputParams::getDespeckleLevel() const {
  return m_despeckleLevel;
}

inline void DefaultParams::OutputParams::setDespeckleLevel(double despeckleLevel) {
  OutputParams::m_despeckleLevel = despeckleLevel;
}


#endif  // SCANTAILOR_CORE_DEFAULTPARAMS_H_
