// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Dependencies.h"
#include <PolygonUtils.h>
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

using namespace imageproc;

namespace select_content {
Dependencies::Dependencies(const QPolygonF& rotatedPageOutline) : m_rotatedPageOutline(rotatedPageOutline) {}

Dependencies::Dependencies(const QPolygonF& rotatedPageOutline,
                           const AutoManualMode contentDetectionMode,
                           const AutoManualMode pageDetectionMode,
                           const bool fineTuneCorners)
    : m_rotatedPageOutline(rotatedPageOutline), m_params(contentDetectionMode, pageDetectionMode, fineTuneCorners) {}

Dependencies::Dependencies(const QDomElement& depsEl)
    : m_rotatedPageOutline(XmlUnmarshaller::polygonF(depsEl.namedItem("rotated-page-outline").toElement())),
      m_params(depsEl.namedItem("params").toElement()) {}

bool Dependencies::compatibleWith(const Dependencies& other) const {
  if (!m_params.compatibleWith(other.m_params)) {
    return false;
  }

  return PolygonUtils::fuzzyCompare(m_rotatedPageOutline, other.m_rotatedPageOutline);
}

bool Dependencies::compatibleWith(const Dependencies& other, bool* updateContentBox, bool* updatePageBox) const {
  bool isCompatible;
  bool needUpdateContentBox;
  bool needUpdatePageBox;
  if (!PolygonUtils::fuzzyCompare(m_rotatedPageOutline, other.m_rotatedPageOutline)) {
    isCompatible = false;
    needUpdateContentBox = true;
    needUpdatePageBox = true;
  } else {
    needUpdateContentBox = m_params.needUpdateContentBox(other.m_params);
    needUpdatePageBox = m_params.needUpdatePageBox(other.m_params);
    isCompatible = !(needUpdateContentBox || needUpdatePageBox);
  }

  if (updateContentBox) {
    *updateContentBox = needUpdateContentBox;
  }
  if (updatePageBox) {
    *updatePageBox = needUpdatePageBox;
  }

  return isCompatible;
}

QDomElement Dependencies::toXml(QDomDocument& doc, const QString& name) const {
  XmlMarshaller marshaller(doc);

  QDomElement el(doc.createElement(name));
  el.appendChild(marshaller.polygonF(m_rotatedPageOutline, "rotated-page-outline"));
  el.appendChild(m_params.toXml(doc, "params"));

  return el;
}

const QPolygonF& Dependencies::rotatedPageOutline() const {
  return m_rotatedPageOutline;
}

void Dependencies::setContentDetectionMode(AutoManualMode contentDetectionMode) {
  m_params.setContentDetectionMode(contentDetectionMode);
}

void Dependencies::setPageDetectionMode(AutoManualMode pageDetectionMode) {
  m_params.setPageDetectionMode(pageDetectionMode);
}

/* ================================= Dependencies::Params ================================= */

Dependencies::Params::Params()
    : m_contentDetectionMode(MODE_AUTO), m_pageDetectionMode(MODE_DISABLED), m_fineTuneCorners(false) {}

Dependencies::Params::Params(const AutoManualMode contentDetectionMode,
                             const AutoManualMode pageDetectionMode,
                             const bool fineTuneCorners)
    : m_contentDetectionMode(contentDetectionMode),
      m_pageDetectionMode(pageDetectionMode),
      m_fineTuneCorners(fineTuneCorners) {}

Dependencies::Params::Params(const QDomElement& el)
    : m_contentDetectionMode(stringToAutoManualMode(el.attribute("contentDetectionMode"))),
      m_pageDetectionMode(stringToAutoManualMode(el.attribute("pageDetectionMode"))),
      m_fineTuneCorners(el.attribute("fineTuneCorners") == "1") {}

QDomElement Dependencies::Params::toXml(QDomDocument& doc, const QString& name) const {
  QDomElement el(doc.createElement(name));
  el.setAttribute("contentDetectionMode", autoManualModeToString(m_contentDetectionMode));
  el.setAttribute("pageDetectionMode", autoManualModeToString(m_pageDetectionMode));
  el.setAttribute("fineTuneCorners", m_fineTuneCorners ? "1" : "0");

  return el;
}

bool Dependencies::Params::compatibleWith(const Dependencies::Params& other) const {
  if ((m_contentDetectionMode != MODE_MANUAL) && (m_contentDetectionMode != other.m_contentDetectionMode)) {
    return false;
  }
  if ((m_pageDetectionMode != MODE_MANUAL) && (m_pageDetectionMode != other.m_pageDetectionMode)) {
    return false;
  }
  if ((m_pageDetectionMode == MODE_AUTO) && (m_fineTuneCorners != other.m_fineTuneCorners)) {
    return false;
  }

  return true;
}

bool Dependencies::Params::needUpdateContentBox(const Dependencies::Params& other) const {
  return (m_contentDetectionMode != MODE_MANUAL) && (m_contentDetectionMode != other.m_contentDetectionMode);
}

bool Dependencies::Params::needUpdatePageBox(const Dependencies::Params& other) const {
  if ((m_pageDetectionMode != MODE_MANUAL) && (m_pageDetectionMode != other.m_pageDetectionMode)) {
    return true;
  }
  if ((m_pageDetectionMode == MODE_AUTO) && (m_fineTuneCorners != other.m_fineTuneCorners)) {
    return true;
  }
  return false;
}

void Dependencies::Params::setContentDetectionMode(AutoManualMode contentDetectionMode) {
  m_contentDetectionMode = contentDetectionMode;
}

void Dependencies::Params::setPageDetectionMode(AutoManualMode pageDetectionMode) {
  m_pageDetectionMode = pageDetectionMode;
}
}  // namespace select_content