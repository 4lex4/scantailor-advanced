// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_SELECT_CONTENT_PARAMS_H_
#define SCANTAILOR_SELECT_CONTENT_PARAMS_H_

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

  Params(const QRectF& contentRect,
         const QSizeF& sizeMm,
         const QRectF& pageRect,
         const Dependencies& deps,
         AutoManualMode contentDetectionMode,
         AutoManualMode pageDetectionMode,
         bool fineTuneCorners);

  explicit Params(const QDomElement& filterEl);

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

  void setFineTuneCornersEnabled(bool fineTuneCorners);

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
#endif  // ifndef SCANTAILOR_SELECT_CONTENT_PARAMS_H_
