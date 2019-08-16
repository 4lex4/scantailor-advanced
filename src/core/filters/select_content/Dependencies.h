// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SELECT_CONTENT_DEPENDENCIES_H_
#define SELECT_CONTENT_DEPENDENCIES_H_

#include <AutoManualMode.h>
#include <QPolygonF>

class QDomDocument;
class QDomElement;
class QString;

namespace select_content {
/**
 * \brief Dependencies of the content box.
 *
 * Once dependencies change, the content box is no longer valid.
 */
class Dependencies {
  // Member-wise copying is OK.
 public:
  Dependencies() = default;

  explicit Dependencies(const QPolygonF& rotated_page_outline);

  explicit Dependencies(const QPolygonF& rotated_page_outline,
                        AutoManualMode content_detection_mode,
                        AutoManualMode page_detection_mode,
                        bool fine_tune_corners);

  explicit Dependencies(const QDomElement& deps_el);

  ~Dependencies() = default;

  const QPolygonF& rotatedPageOutline() const;

  bool compatibleWith(const Dependencies& other) const;

  bool compatibleWith(const Dependencies& other, bool* update_content_box, bool* update_page_box) const;

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  void setContentDetectionMode(AutoManualMode content_detection_mode);

  void setPageDetectionMode(AutoManualMode page_detection_mode);

 private:
  class Params {
   public:
    Params();

    Params(AutoManualMode content_detection_mode, AutoManualMode page_detection_mode, bool fine_tune_corners);

    explicit Params(const QDomElement& el);

    QDomElement toXml(QDomDocument& doc, const QString& name) const;

    bool compatibleWith(const Params& other) const;

    bool needUpdateContentBox(const Params& other) const;

    bool needUpdatePageBox(const Params& other) const;

    void setContentDetectionMode(AutoManualMode content_detection_mode);

    void setPageDetectionMode(AutoManualMode page_detection_mode);

   private:
    AutoManualMode m_contentDetectionMode;
    AutoManualMode m_pageDetectionMode;
    bool m_fineTuneCorners;
  };

  QPolygonF m_rotatedPageOutline;
  Params m_params;
};
}  // namespace select_content
#endif  // ifndef SELECT_CONTENT_DEPENDENCIES_H_
