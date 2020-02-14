// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_LAYOUT_PARAMS_H_
#define SCANTAILOR_PAGE_LAYOUT_PARAMS_H_

#include <QRectF>
#include <QSizeF>

#include "Alignment.h"
#include "Margins.h"

class QDomDocument;
class QDomElement;
class QString;

namespace page_layout {
class Params {
  // Member-wise copying is OK.
 public:
  Params(const Margins& hardMarginsMm,
         const QRectF& pageRect,
         const QRectF& contentRect,
         const QSizeF& contentSizeMm,
         const Alignment& alignment,
         bool autoMargins);

  explicit Params(const QDomElement& el);

  const Margins& hardMarginsMM() const;

  const QRectF& contentRect() const;

  const QRectF& pageRect() const;

  const QSizeF& contentSizeMM() const;

  const Alignment& alignment() const;

  bool isAutoMarginsEnabled() const;

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

 private:
  Margins m_hardMarginsMM;
  QRectF m_contentRect;
  QRectF m_pageRect;
  QSizeF m_contentSizeMM;
  Alignment m_alignment;
  bool m_autoMargins;
};


inline const Margins& Params::hardMarginsMM() const {
  return m_hardMarginsMM;
}

inline const QRectF& Params::contentRect() const {
  return m_contentRect;
}

inline const QRectF& Params::pageRect() const {
  return m_pageRect;
}

inline const QSizeF& Params::contentSizeMM() const {
  return m_contentSizeMM;
}

inline const Alignment& Params::alignment() const {
  return m_alignment;
}

inline bool Params::isAutoMarginsEnabled() const {
  return m_autoMargins;
}
}  // namespace page_layout
#endif  // ifndef SCANTAILOR_PAGE_LAYOUT_PARAMS_H_
