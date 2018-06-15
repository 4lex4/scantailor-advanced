
#ifndef SCANTAILOR_IMAGESETTINGS_H
#define SCANTAILOR_IMAGESETTINGS_H


#include <foundation/ref_countable.h>
#include <imageproc/BinaryThreshold.h>
#include <QtCore/QMutex>
#include <QtXml/QDomDocument>
#include <memory>
#include <unordered_map>
#include "PageId.h"

class AbstractRelinker;

class ImageSettings : public ref_countable {
 public:
  class PageParams {
   public:
    PageParams();

    PageParams(const imageproc::BinaryThreshold& bwThreshold, bool blackOnWhite);

    explicit PageParams(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    const imageproc::BinaryThreshold& getBwThreshold() const;

    void setBwThreshold(const imageproc::BinaryThreshold& bwThreshold);

    bool isBlackOnWhite() const;

    void setBlackOnWhite(bool blackOnWhite);

   private:
    imageproc::BinaryThreshold bwThreshold;
    bool blackOnWhite;
  };

  ImageSettings() = default;

  ~ImageSettings() override = default;

  void clear();

  void performRelinking(const AbstractRelinker& relinker);

  void setPageParams(const PageId& page_id, const PageParams& params);

  std::unique_ptr<PageParams> getPageParams(const PageId& page_id) const;

 private:
  typedef std::unordered_map<PageId, PageParams> PerPageParams;

  mutable QMutex mutex;
  PerPageParams perPageParams;
};


#endif  // SCANTAILOR_IMAGESETTINGS_H
