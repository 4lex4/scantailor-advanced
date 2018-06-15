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
#include <DefaultParams.h>
#include <DefaultParamsProvider.h>
#include <filters/deskew/CacheDrivenTask.h>
#include <filters/deskew/Task.h>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <utility>
#include "CacheDrivenTask.h"
#include "CommandLine.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "OrderBySplitTypeProvider.h"
#include "ProjectPages.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Settings.h"
#include "Task.h"

namespace page_split {
Filter::Filter(intrusive_ptr<ProjectPages> page_sequence, const PageSelectionAccessor& page_selection_accessor)
    : m_ptrPages(std::move(page_sequence)), m_ptrSettings(new Settings), m_selectedPageOrder(0) {
  if (CommandLine::get().isGui()) {
    m_ptrOptionsWidget.reset(new OptionsWidget(m_ptrSettings, m_ptrPages, page_selection_accessor));
  }

  typedef PageOrderOption::ProviderPtr ProviderPtr;

  const ProviderPtr default_order;
  const auto order_by_split_type = make_intrusive<OrderBySplitTypeProvider>(m_ptrSettings);
  m_pageOrderOptions.emplace_back(tr("Natural order"), default_order);
  m_pageOrderOptions.emplace_back(tr("Order by split type"), order_by_split_type);
}

Filter::~Filter() = default;

QString Filter::getName() const {
  return QCoreApplication::translate("page_split::Filter", "Split Pages");
}

PageView Filter::getView() const {
  return IMAGE_VIEW;
}

void Filter::performRelinking(const AbstractRelinker& relinker) {
  m_ptrSettings->performRelinking(relinker);
}

void Filter::preUpdateUI(FilterUiInterface* ui, const PageInfo& page_info) {
  m_ptrOptionsWidget->preUpdateUI(page_info.id());
  ui->setOptionsWidget(m_ptrOptionsWidget.get(), ui->KEEP_OWNERSHIP);
}

QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filter_el(doc.createElement("page-split"));
  filter_el.setAttribute("defaultLayoutType", layoutTypeToString(m_ptrSettings->defaultLayoutType()));

  writer.enumImages([&](const ImageId& image_id, const int numeric_id) {
    this->writeImageSettings(doc, filter_el, image_id, numeric_id);
  });

  return filter_el;
}

void Filter::loadSettings(const ProjectReader& reader, const QDomElement& filters_el) {
  m_ptrSettings->clear();

  const QDomElement filter_el(filters_el.namedItem("page-split").toElement());
  const QString default_layout_type(filter_el.attribute("defaultLayoutType"));
  m_ptrSettings->setLayoutTypeForAllPages(layoutTypeFromString(default_layout_type));

  const QString image_tag_name("image");
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
    const int id = el.attribute("id").toInt(&ok);
    if (!ok) {
      continue;
    }

    const ImageId image_id(reader.imageId(id));
    if (image_id.isNull()) {
      continue;
    }

    Settings::UpdateAction update;

    const QString layout_type(el.attribute("layoutType"));
    if (!layout_type.isEmpty()) {
      update.setLayoutType(layoutTypeFromString(layout_type));
    }

    QDomElement params_el(el.namedItem("params").toElement());
    if (!params_el.isNull()) {
      update.setParams(Params(params_el));
    }

    m_ptrSettings->updatePage(image_id, update);
  }
}  // Filter::loadSettings

void Filter::pageOrientationUpdate(const ImageId& image_id, const OrthogonalRotation& orientation) {
  const Settings::Record record(m_ptrSettings->getPageRecord(image_id));

  if (record.layoutType() && (*record.layoutType() != AUTO_LAYOUT_TYPE)) {
    // The layout type was set manually, so we don't care about orientation.
    return;
  }

  if (record.params() && (record.params()->dependencies().orientation() == orientation)) {
    // We've already estimated the number of pages for this orientation.
    return;
  }

  // Use orientation to update the number of logical pages in an image.
  m_ptrPages->autoSetLayoutTypeFor(image_id, orientation);
}

void Filter::writeImageSettings(QDomDocument& doc,
                                QDomElement& filter_el,
                                const ImageId& image_id,
                                const int numeric_id) const {
  const Settings::Record record(m_ptrSettings->getPageRecord(image_id));

  QDomElement image_el(doc.createElement("image"));
  image_el.setAttribute("id", numeric_id);
  if (const LayoutType* layout_type = record.layoutType()) {
    image_el.setAttribute("layoutType", layoutTypeToString(*layout_type));
  }

  if (const Params* params = record.params()) {
    image_el.appendChild(params->toXml(doc, "params"));
    filter_el.appendChild(image_el);
  }
}

intrusive_ptr<Task> Filter::createTask(const PageInfo& page_info,
                                       intrusive_ptr<deskew::Task> next_task,
                                       const bool batch_processing,
                                       const bool debug) {
  return make_intrusive<Task>(intrusive_ptr<Filter>(this), m_ptrSettings, m_ptrPages, std::move(next_task), page_info,
                              batch_processing, debug);
}

intrusive_ptr<CacheDrivenTask> Filter::createCacheDrivenTask(intrusive_ptr<deskew::CacheDrivenTask> next_task) {
  return make_intrusive<CacheDrivenTask>(m_ptrSettings, m_ptrPages, std::move(next_task));
}

std::vector<PageOrderOption> Filter::pageOrderOptions() const {
  return m_pageOrderOptions;
}

int Filter::selectedPageOrder() const {
  return m_selectedPageOrder;
}

void Filter::selectPageOrder(int option) {
  assert((unsigned) option < m_pageOrderOptions.size());
  m_selectedPageOrder = option;
}

void Filter::loadDefaultSettings(const PageInfo& page_info) {
  if (!m_ptrSettings->getPageRecord(page_info.id().imageId()).isNull()) {
    return;
  }
  const DefaultParams defaultParams = DefaultParamsProvider::getInstance()->getParams();
  const DefaultParams::PageSplitParams& pageSplitParams = defaultParams.getPageSplitParams();

  Settings::UpdateAction update;
  update.setLayoutType(pageSplitParams.getLayoutType());
  m_ptrSettings->updatePage(page_info.id().imageId(), update);
}

OptionsWidget* Filter::optionsWidget() {
  return m_ptrOptionsWidget.get();
}
}  // namespace page_split
