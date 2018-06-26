/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
