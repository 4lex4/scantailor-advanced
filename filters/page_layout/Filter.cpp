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
#include "Settings.h"
#include "Params.h"
#include "ProjectPages.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "CacheDrivenTask.h"
#include "OrderByWidthProvider.h"
#include "OrderByHeightProvider.h"
#include "Utils.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <utility>
#include <XmlMarshaller.h>
#include <XmlUnmarshaller.h>
#include <DefaultParams.h>
#include <DefaultParamsProvider.h>
#include "CommandLine.h"
#include <filters/output/Task.h>
#include <filters/output/CacheDrivenTask.h>
#include <UnitsConverter.h>

namespace page_layout {
    Filter::Filter(intrusive_ptr<ProjectPages> pages, const PageSelectionAccessor& page_selection_accessor)
            : m_ptrPages(std::move(pages)),
              m_ptrSettings(new Settings),
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
        return tr("Margins");
    }

    PageView Filter::getView() const {
        return PAGE_VIEW;
    }

    void Filter::selected() {
        m_ptrSettings->removePagesMissingFrom(m_ptrPages->toPageSequence(getView()));
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

    void Filter::preUpdateUI(FilterUiInterface* ui, const PageId& page_id) {
        const Margins margins_mm(m_ptrSettings->getHardMarginsMM(page_id));
        const Alignment alignment(m_ptrSettings->getPageAlignment(page_id));
        m_ptrOptionsWidget->preUpdateUI(page_id, margins_mm, alignment);
        ui->setOptionsWidget(m_ptrOptionsWidget.get(), ui->KEEP_OWNERSHIP);
    }

    QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
        using namespace boost::lambda;

        QDomElement filter_el(doc.createElement("page-layout"));

        XmlMarshaller marshaller(doc);
        filter_el.appendChild(marshaller.rectF(m_ptrSettings->getContentRect(), "contentRect"));

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
                filters_el.namedItem("page-layout").toElement()
        );

        const QDomElement rect_el = filter_el.namedItem("contentRect").toElement();
        if (!rect_el.isNull()) {
            m_ptrSettings->setContentRect(XmlUnmarshaller::rectF(rect_el));
        }

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

    void Filter::setContentBox(const PageId& page_id, const ImageTransformation& xform, const QRectF& content_rect) {
        const QSizeF content_size_mm(Utils::calcRectSizeMM(xform, content_rect));
        m_ptrSettings->setContentSizeMM(page_id, content_size_mm);
    }

    void Filter::invalidateContentBox(const PageId& page_id) {
        m_ptrSettings->invalidateContentSize(page_id);
    }

    bool Filter::checkReadyForOutput(const ProjectPages& pages, const PageId* ignore) {
        const PageSequence snapshot(pages.toPageSequence(PAGE_VIEW));

        return m_ptrSettings->checkEverythingDefined(snapshot, ignore);
    }

    intrusive_ptr<Task>
    Filter::createTask(const PageId& page_id,
                       intrusive_ptr<output::Task> next_task,
                       const bool batch,
                       const bool debug) {
        return intrusive_ptr<Task>(
                new Task(
                        intrusive_ptr<Filter>(this), std::move(next_task),
                        m_ptrSettings, page_id, batch, debug
                )
        );
    }

    intrusive_ptr<CacheDrivenTask>
    Filter::createCacheDrivenTask(intrusive_ptr<output::CacheDrivenTask> next_task) {
        return intrusive_ptr<CacheDrivenTask>(
                new CacheDrivenTask(std::move(next_task), m_ptrSettings)
        );
    }

    void Filter::loadDefaultSettings(const PageInfo& page_info) {
        if (!m_ptrSettings->isParamsNull(page_info.id())) {
            return;
        }
        const DefaultParams defaultParams = DefaultParamsProvider::getInstance()->getParams();
        const DefaultParams::PageLayoutParams& pageLayoutParams = defaultParams.getPageLayoutParams();

        const UnitsConverter unitsConverter(page_info.metadata().dpi());

        const Margins& margins = pageLayoutParams.getHardMargins();
        double leftMargin = margins.left();
        double topMargin = margins.top();
        double rightMargin = margins.right();
        double bottomMargin = margins.bottom();
        unitsConverter.convert(leftMargin, topMargin, defaultParams.getUnits(), MILLIMETRES);
        unitsConverter.convert(rightMargin, bottomMargin, defaultParams.getUnits(), MILLIMETRES);

        m_ptrSettings->setPageParams(
                page_info.id(),
                Params(Margins(leftMargin, topMargin, rightMargin, bottomMargin),
                       QRectF(),
                       QRectF(),
                       QSizeF(),
                       pageLayoutParams.getAlignment(),
                       pageLayoutParams.isAutoMargins()
                )
        );
    }

    OptionsWidget* Filter::optionsWidget() {
        return m_ptrOptionsWidget.get();
    }
}  // namespace page_layout