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

namespace output {
    Params::Params()
            : m_dpi(600, 600),
              m_despeckleLevel(DESPECKLE_CAUTIOUS) {
    }

    Params::Params(const Dpi& m_dpi,
                   const ColorParams& m_colorParams,
                   const SplittingOptions& m_splittingOptions,
                   const PictureShapeOptions& m_pictureShapeOptions,
                   const dewarping::DistortionModel& m_distortionModel,
                   const DepthPerception& m_depthPerception,
                   const DewarpingOptions& m_dewarpingOptions,
                   DespeckleLevel m_despeckleLevel) : m_dpi(m_dpi),
                                                      m_colorParams(m_colorParams),
                                                      m_splittingOptions(m_splittingOptions),
                                                      m_pictureShapeOptions(m_pictureShapeOptions),
                                                      m_distortionModel(m_distortionModel),
                                                      m_depthPerception(m_depthPerception),
                                                      m_dewarpingOptions(m_dewarpingOptions),
                                                      m_despeckleLevel(m_despeckleLevel) {
    }

    Params::Params(QDomElement const& el)
            : m_dpi(XmlUnmarshaller::dpi(el.namedItem("dpi").toElement())),
              m_distortionModel(el.namedItem("distortion-model").toElement()),
              m_depthPerception(el.attribute("depthPerception")),
              m_dewarpingOptions(el.namedItem("dewarping-options").toElement()),
              m_despeckleLevel(despeckleLevelFromString(el.attribute("despeckleLevel"))),
              m_pictureShapeOptions(el.namedItem("picture-shape-options").toElement()),
              m_splittingOptions(el.namedItem("splitting").toElement()) {
        QDomElement const cp(el.namedItem("color-params").toElement());
        m_colorParams.setColorMode(parseColorMode(cp.attribute("colorMode")));
        m_colorParams.setColorCommonOptions(
                ColorCommonOptions(
                        cp.namedItem("color-or-grayscale").toElement()
                )
        );
        m_colorParams.setBlackWhiteOptions(
                BlackWhiteOptions(cp.namedItem("bw").toElement())
        );
    }

    QDomElement Params::toXml(QDomDocument& doc, QString const& name) const {
        XmlMarshaller marshaller(doc);

        QDomElement el(doc.createElement(name));
        el.appendChild(m_distortionModel.toXml(doc, "distortion-model"));
        el.appendChild(m_pictureShapeOptions.toXml(doc, "picture-shape-options"));
        el.setAttribute("depthPerception", m_depthPerception.toString());
        el.appendChild(m_dewarpingOptions.toXml(doc, "dewarping-options"));
        el.setAttribute("despeckleLevel", despeckleLevelToString(m_despeckleLevel));
        el.appendChild(marshaller.dpi(m_dpi, "dpi"));
        el.appendChild(m_splittingOptions.toXml(doc, "splitting"));
        
        QDomElement cp(doc.createElement("color-params"));
        cp.setAttribute(
                "colorMode",
                formatColorMode(m_colorParams.colorMode())
        );

        cp.appendChild(
                m_colorParams.colorCommonOptions().toXml(
                        doc, "color-or-grayscale"
                )
        );
        cp.appendChild(m_colorParams.blackWhiteOptions().toXml(doc, "bw"));

        el.appendChild(cp);

        return el;
    }

    ColorParams::ColorMode Params::parseColorMode(QString const& str) {
        if (str == "bw") {
            return ColorParams::BLACK_AND_WHITE;
        } else if (str == "colorOrGray") {
            return ColorParams::COLOR_GRAYSCALE;
        } else if (str == "mixed") {
            return ColorParams::MIXED;
        } else {
            return ColorParams::DefaultColorMode();
        }
    }

    QString Params::formatColorMode(ColorParams::ColorMode const mode) {
        char const* str = "";
        switch (mode) {
            case ColorParams::BLACK_AND_WHITE:
                str = "bw";
                break;
            case ColorParams::COLOR_GRAYSCALE:
                str = "colorOrGray";
                break;
            case ColorParams::MIXED:
                str = "mixed";
                break;
        }

        return QString::fromLatin1(str);
    }
}  // namespace output