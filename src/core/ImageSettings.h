// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_IMAGESETTINGS_H_
#define SCANTAILOR_CORE_IMAGESETTINGS_H_

#include <foundation/NonCopyable.h>
#include <foundation/ref_countable.h>
#include <imageproc/BinaryThreshold.h>
#include <QtCore/QMutex>
#include <QtXml/QDomDocument>
#include <memory>
#include <unordered_map>
#include "PageId.h"

class AbstractRelinker;

class ImageSettings : public ref_countable {
  DECLARE_NON_COPYABLE(ImageSettings)
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
    imageproc::BinaryThreshold m_bwThreshold;
    bool m_blackOnWhite;
  };

  ImageSettings() = default;

  ~ImageSettings() override = default;

  void clear();

  void performRelinking(const AbstractRelinker& relinker);

  void setPageParams(const PageId& pageId, const PageParams& params);

  std::unique_ptr<PageParams> getPageParams(const PageId& pageId) const;

 private:
  using PerPageParams = std::unordered_map<PageId, PageParams>;

  mutable QMutex m_mutex;
  PerPageParams m_perPageParams;
};


#endif  // SCANTAILOR_CORE_IMAGESETTINGS_H_
