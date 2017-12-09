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

#include "Params.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

#include "CommandLine.h"

namespace select_content {
    Params::Params(Dependencies const& deps)
            : m_deps(deps),
              m_contentDetectionMode(MODE_AUTO),
              m_pageDetectionMode(MODE_AUTO),
              m_contentDetectEnabled(true),
              m_pageDetectEnabled(false),
              m_fineTuneCorners(false),
              m_deviation(0.0) {
    }

    Params::Params(QRectF const& content_rect,
                   QSizeF const& content_size_mm,
                   QRectF const& page_rect,
                   Dependencies const& deps,
                   AutoManualMode const content_detection_mode,
                   AutoManualMode const page_detection_mode,
                   bool const contentDetect,
                   bool const pageDetect,
                   bool const fineTuning)
            : m_contentRect(content_rect),
              m_pageRect(page_rect),
              m_contentSizeMM(content_size_mm),
              m_deps(deps),
              m_contentDetectionMode(content_detection_mode),
              m_pageDetectionMode(page_detection_mode),
              m_contentDetectEnabled(contentDetect),
              m_pageDetectEnabled(pageDetect),
              m_fineTuneCorners(fineTuning),
              m_deviation(0.0) {
    }

    Params::Params(QDomElement const& filter_el)
            : m_contentRect(
            XmlUnmarshaller::rectF(
                    filter_el.namedItem("content-rect").toElement()
            )
    ),
              m_pageRect(
                      XmlUnmarshaller::rectF(
                              filter_el.namedItem("page-rect").toElement()
                      )
              ),
              m_contentSizeMM(
                      XmlUnmarshaller::sizeF(
                              filter_el.namedItem("content-size-mm").toElement()
                      )
              ),
              m_deps(filter_el.namedItem("dependencies").toElement()),
              m_contentDetectionMode(filter_el.attribute("contentDetectionMode") == "manual" ? MODE_MANUAL : MODE_AUTO),
              m_pageDetectionMode(filter_el.attribute("pageDetectionMode") == "manual" ? MODE_MANUAL : MODE_AUTO),
              m_contentDetectEnabled(filter_el.attribute("content-detect") == "1"),
              m_pageDetectEnabled(filter_el.attribute("page-detect") == "1"),
              m_fineTuneCorners(filter_el.attribute("fine-tune-corners") == "1"),
              m_deviation(filter_el.attribute("deviation").toDouble()) {
        if (m_pageDetectEnabled && !m_contentDetectEnabled && CommandLine::get().isForcePageDetectionDisabled()) {
            m_pageDetectEnabled = false;
            m_contentRect = m_pageRect;
            m_contentDetectEnabled = true;
            m_contentDetectionMode = MODE_MANUAL;
        }
    }

    Params::~Params() {
    }

    QDomElement Params::toXml(QDomDocument& doc, QString const& name) const {
        XmlMarshaller marshaller(doc);

        QDomElement el(doc.createElement(name));
        el.setAttribute("contentDetectionMode", (m_contentDetectionMode == MODE_AUTO) ? "auto" : "manual");
        el.setAttribute("pageDetectionMode", (m_pageDetectionMode == MODE_AUTO) ? "auto" : "manual");
        el.setAttribute("content-detect", m_contentDetectEnabled ? "1" : "0");
        el.setAttribute("page-detect", m_pageDetectEnabled ? "1" : "0");
        el.setAttribute("fine-tune-corners", m_fineTuneCorners ? "1" : "0");
        el.setAttribute("deviation", m_deviation);
        el.appendChild(marshaller.rectF(m_contentRect, "content-rect"));
        el.appendChild(marshaller.rectF(m_pageRect, "page-rect"));
        el.appendChild(marshaller.sizeF(m_contentSizeMM, "content-size-mm"));
        el.appendChild(m_deps.toXml(doc, "dependencies"));

        return el;
    }
}  // namespace content_rect
