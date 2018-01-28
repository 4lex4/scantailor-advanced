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

#include "Filter.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "Task.h"
#include "CacheDrivenTask.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <utility>
#include <DefaultParamsProvider.h>
#include <DefaultParams.h>
#include "CommandLine.h"
#include <filters/page_split/Task.h>
#include <filters/page_split/CacheDrivenTask.h>

namespace fix_orientation {
    Filter::Filter(PageSelectionAccessor const& page_selection_accessor)
            : m_ptrSettings(new Settings) {
        if (CommandLine::get().isGui()) {
            m_ptrOptionsWidget.reset(
                    new OptionsWidget(m_ptrSettings, page_selection_accessor)
            );
        }
    }

    Filter::~Filter() = default;

    QString Filter::getName() const {
        return QCoreApplication::translate(
                "fix_orientation::Filter", "Fix Orientation"
        );
    }

    PageView Filter::getView() const {
        return IMAGE_VIEW;
    }

    void Filter::performRelinking(AbstractRelinker const& relinker) {
        m_ptrSettings->performRelinking(relinker);
    }

    void Filter::preUpdateUI(FilterUiInterface* ui, PageId const& page_id) {
        if (m_ptrOptionsWidget.get()) {
            OrthogonalRotation const rotation(
                    m_ptrSettings->getRotationFor(page_id.imageId())
            );
            m_ptrOptionsWidget->preUpdateUI(page_id, rotation);
            ui->setOptionsWidget(m_ptrOptionsWidget.get(), ui->KEEP_OWNERSHIP);
        }
    }

    QDomElement Filter::saveSettings(ProjectWriter const& writer, QDomDocument& doc) const {
        using namespace boost::lambda;

        QDomElement filter_el(doc.createElement("fix-orientation"));
        writer.enumImages(
                [&](ImageId const& image_id, int const numeric_id) {
                    this->writeImageSettings(doc, filter_el, image_id, numeric_id);
                }
        );

        return filter_el;
    }

    void Filter::loadSettings(ProjectReader const& reader, QDomElement const& filters_el) {
        m_ptrSettings->clear();

        QDomElement filter_el(filters_el.namedItem("fix-orientation").toElement());

        QString const image_tag_name("image");
        QDomNode node(filter_el.firstChild());
        for (; !node.isNull(); node = node.nextSibling()) {
            if (!node.isElement()) {
                continue;
            }
            if (node.nodeName() != image_tag_name) {
                continue;
            }
            QDomElement el(node.toElement());

            bool ok = true;
            int const id = el.attribute("id").toInt(&ok);
            if (!ok) {
                continue;
            }

            ImageId const image_id(reader.imageId(id));
            if (image_id.isNull()) {
                continue;
            }

            OrthogonalRotation const rotation(
                    XmlUnmarshaller::rotation(
                            el.namedItem("rotation").toElement()
                    )
            );

            m_ptrSettings->applyRotation(image_id, rotation);
        }
    }      // Filter::loadSettings

    intrusive_ptr<Task>
    Filter::createTask(PageId const& page_id, intrusive_ptr<page_split::Task> next_task,
                       bool const batch_processing) {
        return intrusive_ptr<Task>(
                new Task(
                        page_id.imageId(), intrusive_ptr<Filter>(this),
                        m_ptrSettings, std::move(next_task), batch_processing
                )
        );
    }

    intrusive_ptr<CacheDrivenTask>
    Filter::createCacheDrivenTask(intrusive_ptr<page_split::CacheDrivenTask> next_task) {
        return intrusive_ptr<CacheDrivenTask>(
                new CacheDrivenTask(m_ptrSettings, std::move(next_task))
        );
    }

    void Filter::writeImageSettings(QDomDocument& doc, QDomElement& filter_el, ImageId const& image_id,
                                    int const numeric_id) const {
        OrthogonalRotation const rotation(m_ptrSettings->getRotationFor(image_id));
        if (rotation.toDegrees() == 0) {
            return;
        }

        XmlMarshaller marshaller(doc);

        QDomElement image_el(doc.createElement("image"));
        image_el.setAttribute("id", numeric_id);
        image_el.appendChild(marshaller.rotation(rotation, "rotation"));
        filter_el.appendChild(image_el);
    }

    void Filter::loadDefaultSettings(PageId const& page_id) {
        if (!m_ptrSettings->isRotationNull(page_id.imageId())) {
            return;
        }
        const DefaultParams defaultParams = DefaultParamsProvider::getInstance()->getParams();
        const DefaultParams::FixOrientationParams& fixOrientationParams = defaultParams.getFixOrientationParams();

        m_ptrSettings->applyRotation(page_id.imageId(), fixOrientationParams.getImageRotation());
    }

    OptionsWidget* Filter::optionsWidget() {
        return m_ptrOptionsWidget.get();
    }
}  // namespace fix_orientation