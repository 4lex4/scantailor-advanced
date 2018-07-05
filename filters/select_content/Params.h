/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef SELECT_CONTENT_PARAMS_H_
#define SELECT_CONTENT_PARAMS_H_

#include <AutoManualMode.h>
#include <QRectF>
#include <QSizeF>
#include <cmath>
#include "Dependencies.h"
#include "Margins.h"

class QDomDocument;
class QDomElement;
class QString;

namespace select_content {
class Params {
 public:
  // Member-wise copying is OK.

  explicit Params(const Dependencies& deps);

  Params(const QRectF& content_rect,
         const QSizeF& size_mm,
         const QRectF& page_rect,
         const Dependencies& deps,
         AutoManualMode content_detection_mode,
         AutoManualMode page_detection_mode,
         bool fine_tune_corners);

  explicit Params(const QDomElement& filter_el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  ~Params();

  const QRectF& contentRect() const;

  const QRectF& pageRect() const;

  const QSizeF& contentSizeMM() const;

  const Dependencies& dependencies() const;

  AutoManualMode contentDetectionMode() const;

  AutoManualMode pageDetectionMode() const;

  bool isFineTuningEnabled() const;

  void setContentDetectionMode(AutoManualMode mode);

  void setPageDetectionMode(AutoManualMode mode);

  void setContentRect(const QRectF& rect);

  void setPageRect(const QRectF& rect);

  void setContentSizeMM(const QSizeF& size);

  void setDependencies(const Dependencies& deps);

  void setFineTuneCornersEnabled(bool fine_tune_corners);

 private:
  QRectF m_contentRect;
  QRectF m_pageRect;
  QSizeF m_contentSizeMM;
  Dependencies m_deps;
  AutoManualMode m_contentDetectionMode;
  AutoManualMode m_pageDetectionMode;
  bool m_fineTuneCorners;
};
}  // namespace select_content
#endif  // ifndef SELECT_CONTENT_PARAMS_H_
