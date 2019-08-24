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


#endif  // SCANTAILOR_CORE_DEFAULTPARAMS_H_
