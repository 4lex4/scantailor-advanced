
#ifndef SCANTAILOR_DEFAULTPARAMSPROFILE_H
#define SCANTAILOR_DEFAULTPARAMSPROFILE_H


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
   private:
    OrthogonalRotation imageRotation;

   public:
    FixOrientationParams() = default;

    explicit FixOrientationParams(const OrthogonalRotation& imageRotation);

    explicit FixOrientationParams(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    const OrthogonalRotation& getImageRotation() const;

    void setImageRotation(const OrthogonalRotation& imageRotation);
  };

  class DeskewParams {
   private:
    double deskewAngleDeg;
    AutoManualMode mode;

   public:
    DeskewParams();

    explicit DeskewParams(double deskewAngleDeg, AutoManualMode mode);

    explicit DeskewParams(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    double getDeskewAngleDeg() const;

    void setDeskewAngleDeg(double deskewAngleDeg);

    AutoManualMode getMode() const;

    void setMode(AutoManualMode mode);
  };

  class PageSplitParams {
   private:
    page_split::LayoutType layoutType;

   public:
    PageSplitParams();

    explicit PageSplitParams(page_split::LayoutType layoutType);

    explicit PageSplitParams(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    page_split::LayoutType getLayoutType() const;

    void setLayoutType(page_split::LayoutType layoutType);
  };

  class SelectContentParams {
   private:
    QSizeF pageRectSize;
    AutoManualMode pageDetectMode;
    bool contentDetectEnabled;
    bool pageDetectEnabled;
    bool fineTuneCorners;

   public:
    SelectContentParams();

    SelectContentParams(const QSizeF& pageRectSize,
                        AutoManualMode pageDetectMode,
                        bool contentDetectEnabled,
                        bool pageDetectEnabled,
                        bool fineTuneCorners);

    explicit SelectContentParams(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    const QSizeF& getPageRectSize() const;

    void setPageRectSize(const QSizeF& pageRectSize);

    bool isContentDetectEnabled() const;

    void setContentDetectEnabled(bool contentDetectEnabled);

    bool isPageDetectEnabled() const;

    void setPageDetectEnabled(bool pageDetectEnabled);

    bool isFineTuneCorners() const;

    void setFineTuneCorners(bool fineTuneCorners);

    AutoManualMode getPageDetectMode() const;

    void setPageDetectMode(AutoManualMode pageDetectMode);
  };

  class PageLayoutParams {
   private:
    Margins hardMargins;
    page_layout::Alignment alignment;
    bool autoMargins;

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
  };

  class OutputParams {
   private:
    Dpi dpi;
    output::ColorParams colorParams;
    output::SplittingOptions splittingOptions;
    output::PictureShapeOptions pictureShapeOptions;
    output::DepthPerception depthPerception;
    output::DewarpingOptions dewarpingOptions;
    double despeckleLevel;

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
  FixOrientationParams fixOrientationParams;
  DeskewParams deskewParams;
  PageSplitParams pageSplitParams;
  SelectContentParams selectContentParams;
  PageLayoutParams pageLayoutParams;
  OutputParams outputParams;
  Units units;
};


#endif  // SCANTAILOR_DEFAULTPARAMSPROFILE_H
