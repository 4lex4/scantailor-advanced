// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Dependencies.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include <PolygonUtils.h>

using namespace imageproc;

namespace select_content {
Dependencies::Dependencies(const QPolygonF& rotated_page_outline) : m_rotatedPageOutline(rotated_page_outline) {}

Dependencies::Dependencies(const QPolygonF& rotated_page_outline,
                           const AutoManualMode content_detection_mode,
                           const AutoManualMode page_detection_mode,
                           const bool fine_tune_corners)
    : m_rotatedPageOutline(rotated_page_outline),
      m_params(content_detection_mode, page_detection_mode, fine_tune_corners) {}

Dependencies::Dependencies(const QDomElement& deps_el)
    : m_rotatedPageOutline(XmlUnmarshaller::polygonF(deps_el.namedItem("rotated-page-outline").toElement())),
      m_params(deps_el.namedItem("params").toElement()) {}

bool Dependencies::compatibleWith(const Dependencies& other) const {
  if (!m_params.compatibleWith(other.m_params)) {
    return false;
  }

  return PolygonUtils::fuzzyCompare(m_rotatedPageOutline, other.m_rotatedPageOutline);
}

bool Dependencies::compatibleWith(const Dependencies& other, bool* update_content_box, bool* update_page_box) const {
  bool is_compatible;
  bool need_update_content_box;
  bool need_update_page_box;
  if (!PolygonUtils::fuzzyCompare(m_rotatedPageOutline, other.m_rotatedPageOutline)) {
    is_compatible = false;
    need_update_content_box = true;
    need_update_page_box = true;
  } else {
    need_update_content_box = m_params.needUpdateContentBox(other.m_params);
    need_update_page_box = m_params.needUpdatePageBox(other.m_params);
    is_compatible = !(need_update_content_box || need_update_page_box);
  }

  if (update_content_box) {
    *update_content_box = need_update_content_box;
  }
  if (update_page_box) {
    *update_page_box = need_update_page_box;
  }

  return is_compatible;
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

void Dependencies::setContentDetectionMode(AutoManualMode content_detection_mode) {
  m_params.setContentDetectionMode(content_detection_mode);
}

void Dependencies::setPageDetectionMode(AutoManualMode page_detection_mode) {
  m_params.setPageDetectionMode(page_detection_mode);
}

/* ================================= Dependencies::Params ================================= */

Dependencies::Params::Params()
    : m_contentDetectionMode(MODE_AUTO), m_pageDetectionMode(MODE_DISABLED), m_fineTuneCorners(false) {}

Dependencies::Params::Params(const AutoManualMode content_detection_mode,
                             const AutoManualMode page_detection_mode,
                             const bool fine_tune_corners)
    : m_contentDetectionMode(content_detection_mode),
      m_pageDetectionMode(page_detection_mode),
      m_fineTuneCorners(fine_tune_corners) {}

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

void Dependencies::Params::setContentDetectionMode(AutoManualMode content_detection_mode) {
  m_contentDetectionMode = content_detection_mode;
}

void Dependencies::Params::setPageDetectionMode(AutoManualMode page_detection_mode) {
  m_pageDetectionMode = page_detection_mode;
}
}  // namespace select_content