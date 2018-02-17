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
#include "Task.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "CacheDrivenTask.h"
#include "OrderByWidthProvider.h"
#include "OrderByHeightProvider.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <utility>
#include <DefaultParams.h>
#include <DefaultParamsProvider.h>
#include "CommandLine.h"
#include <filters/page_layout/Task.h>
#include <filters/page_layout/CacheDrivenTask.h>
#include <UnitsConverter.h>

namespace select_content {
    Filter::Filter(const PageSelectionAccessor& page_selection_accessor)
            : m_ptrSettings(new Settings),
              m_selectedPageOrder(0) {
        if (CommandLine::get().isGui()) {
            m_ptrOptionsWidget.reset(
                    new OptionsWidget(m_ptrSettings, page_selection_accessor)
            );
        }

        typedef PageOrderOption::ProviderPtr ProviderPtr;

        const ProviderPtr default_order;
        const ProviderPtr order_by_width(new OrderByWidthProvider(m_ptrSettings));
        const ProviderPtr order_by_height(new OrderByHeightProvider(m_ptrSettings));
        m_pageOrderOptions.emplace_back(tr("Natural order"), default_order);
        m_pageOrderOptions.emplace_back(tr("Order by increasing width"), order_by_width);
        m_pageOrderOptions.emplace_back(tr("Order by increasing height"), order_by_height);
    }

    Filter::~Filter() = default;

    QString Filter::getName() const {
        return tr("Select Content");
    }

    PageView Filter::getView() const {
        return PAGE_VIEW;
    }

    int Filter::selectedPageOrder() const {
        return m_selectedPageOrder;
    }

    void Filter::selectPageOrder(int option) {
        assert((unsigned) option < m_pageOrderOptions.size());
        m_selectedPageOrder = option;
    }

    std::vector<PageOrderOption>
    Filter::pageOrderOptions() const {
        return m_pageOrderOptions;
    }

    void Filter::performRelinking(const AbstractRelinker& relinker) {
        m_ptrSettings->performRelinking(relinker);
    }

    void Filter::preUpdateUI(FilterUiInterface* ui, const PageInfo& page_info) {
        m_ptrOptionsWidget->preUpdateUI(page_info);
        ui->setOptionsWidget(m_ptrOptionsWidget.get(), ui->KEEP_OWNERSHIP);
    }

    QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
        using namespace boost::lambda;

        QDomElement filter_el(doc.createElement("select-content"));

        filter_el.setAttribute("pageDetectionBoxWidth", m_ptrSettings->pageDetectionBox().width());
        filter_el.setAttribute("pageDetectionBoxHeight", m_ptrSettings->pageDetectionBox().height());
        filter_el.setAttribute("pageDetectionTolerance", m_ptrSettings->pageDetectionTolerance());

        writer.enumPages(
                [&](const PageId& page_id, int numeric_id) {
                    this->writePageSettings(doc, filter_el, page_id, numeric_id);
                }
        );

        return filter_el;
    }

    void
    Filter::writePageSettings(QDomDocument& doc, QDomElement& filter_el, const PageId& page_id, int numeric_id) const {
        const std::unique_ptr<Params> params(m_ptrSettings->getPageParams(page_id));
        if (!params) {
            return;
        }

        QDomElement page_el(doc.createElement("page"));
        page_el.setAttribute("id", numeric_id);
        page_el.appendChild(params->toXml(doc, "params"));

        filter_el.appendChild(page_el);
    }

    void Filter::loadSettings(const ProjectReader& reader, const QDomElement& filters_el) {
        m_ptrSettings->clear();

        const QDomElement filter_el(
                filters_el.namedItem("select-content").toElement()
        );

        QSizeF box(0.0, 0.0);
        box.setWidth(filter_el.attribute("pageDetectionBoxWidth", "0.0").toDouble());
        box.setHeight(filter_el.attribute("pageDetectionBoxHeight", "0.0").toDouble());
        m_ptrSettings->setPageDetectionBox(box);

        m_ptrSettings->setPageDetectionTolerance(filter_el.attribute("pageDetectionTolerance", "0.1").toDouble());

        const QString page_tag_name("page");
        QDomNode node(filter_el.firstChild());
        for (; !node.isNull(); node = node.nextSibling()) {
            if (!node.isElement()) {
                continue;
            }
            if (node.nodeName() != page_tag_name) {
                continue;
            }
            const QDomElement el(node.toElement());

            bool ok = true;
            const int id = el.attribute("id").toInt(&ok);
            if (!ok) {
                continue;
            }

            const PageId page_id(reader.pageId(id));
            if (page_id.isNull()) {
                continue;
            }

            const QDomElement params_el(el.namedItem("params").toElement());
            if (params_el.isNull()) {
                continue;
            }

            const Params params(params_el);
            m_ptrSettings->setPageParams(page_id, params);
        }
    }      // Filter::loadSettings

    intrusive_ptr<Task>
    Filter::createTask(const PageId& page_id, intrusive_ptr<page_layout::Task> next_task, bool batch,
                       bool debug) {
        return intrusive_ptr<Task>(
                new Task(
                        intrusive_ptr<Filter>(this), std::move(next_task),
                        m_ptrSettings, page_id, batch, debug
                )
        );
    }

    intrusive_ptr<CacheDrivenTask>
    Filter::createCacheDrivenTask(intrusive_ptr<page_layout::CacheDrivenTask> next_task) {
        return intrusive_ptr<CacheDrivenTask>(
                new CacheDrivenTask(m_ptrSettings, std::move(next_task))
        );
    }

    void Filter::loadDefaultSettings(const PageInfo& page_info) {
        if (!m_ptrSettings->isParamsNull(page_info.id())) {
            return;
        }
        const DefaultParams defaultParams = DefaultParamsProvider::getInstance()->getParams();
        const DefaultParams::SelectContentParams& selectContentParams = defaultParams.getSelectContentParams();

        const UnitsConverter unitsConverter(page_info.metadata().dpi());

        const QSizeF& pageRectSize = selectContentParams.getPageRectSize();
        double pageRectWidth = pageRectSize.width();
        double pageRectHeight = pageRectSize.height();
        unitsConverter.convert(pageRectWidth, pageRectHeight, defaultParams.getUnits(), PIXELS);

        m_ptrSettings->setPageParams(
                page_info.id(),
                Params(QRectF(),
                       QSizeF(),
                       QRectF(QPointF(0, 0), QSizeF(pageRectWidth, pageRectHeight)),
                       Dependencies(),
                       MODE_AUTO,
                       selectContentParams.getPageDetectMode(),
                       selectContentParams.isContentDetectEnabled(),
                       selectContentParams.isPageDetectEnabled(),
                       selectContentParams.isFineTuneCorners())
        );
    }

    OptionsWidget* Filter::optionsWidget() {
        return m_ptrOptionsWidget.get();
    }

}  // namespace select_content