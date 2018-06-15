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
#include <tiff.h>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <utility>
#include "CacheDrivenTask.h"
#include "CommandLine.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Settings.h"
#include "Task.h"
#include "ThumbnailPixmapCache.h"

namespace output {
Filter::Filter(const PageSelectionAccessor& page_selection_accessor) : m_ptrSettings(new Settings) {
  if (CommandLine::get().isGui()) {
    m_ptrOptionsWidget.reset(new OptionsWidget(m_ptrSettings, page_selection_accessor));
  }
}

Filter::~Filter() = default;

QString Filter::getName() const {
  return QCoreApplication::translate("output::Filter", "Output");
}

PageView Filter::getView() const {
  return PAGE_VIEW;
}

void Filter::performRelinking(const AbstractRelinker& relinker) {
  m_ptrSettings->performRelinking(relinker);
}

void Filter::preUpdateUI(FilterUiInterface* ui, const PageInfo& page_info) {
  m_ptrOptionsWidget->preUpdateUI(page_info.id());
  ui->setOptionsWidget(m_ptrOptionsWidget.get(), ui->KEEP_OWNERSHIP);
}

QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filter_el(doc.createElement("output"));

  writer.enumPages(
      [&](const PageId& page_id, int numeric_id) { this->writePageSettings(doc, filter_el, page_id, numeric_id); });

  return filter_el;
}

void Filter::writePageSettings(QDomDocument& doc, QDomElement& filter_el, const PageId& page_id, int numeric_id) const {
  const Params params(m_ptrSettings->getParams(page_id));

  QDomElement page_el(doc.createElement("page"));
  page_el.setAttribute("id", numeric_id);

  page_el.appendChild(m_ptrSettings->pictureZonesForPage(page_id).toXml(doc, "zones"));
  page_el.appendChild(m_ptrSettings->fillZonesForPage(page_id).toXml(doc, "fill-zones"));
  page_el.appendChild(params.toXml(doc, "params"));
  page_el.appendChild(m_ptrSettings->getOutputProcessingParams(page_id).toXml(doc, "processing-params"));

  std::unique_ptr<OutputParams> output_params(m_ptrSettings->getOutputParams(page_id));
  if (output_params) {
    page_el.appendChild(output_params->toXml(doc, "output-params"));
  }

  filter_el.appendChild(page_el);
}

void Filter::loadSettings(const ProjectReader& reader, const QDomElement& filters_el) {
  m_ptrSettings->clear();

  const QDomElement filter_el(filters_el.namedItem("output").toElement());

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

    const ZoneSet picture_zones(el.namedItem("zones").toElement(), m_pictureZonePropFactory);
    if (!picture_zones.empty()) {
      m_ptrSettings->setPictureZones(page_id, picture_zones);
    }

    const ZoneSet fill_zones(el.namedItem("fill-zones").toElement(), m_fillZonePropFactory);
    if (!fill_zones.empty()) {
      m_ptrSettings->setFillZones(page_id, fill_zones);
    }

    const QDomElement params_el(el.namedItem("params").toElement());
    if (!params_el.isNull()) {
      const Params params(params_el);
      m_ptrSettings->setParams(page_id, params);
    }

    const QDomElement output_processing_params_el(el.namedItem("processing-params").toElement());
    if (!output_processing_params_el.isNull()) {
      const OutputProcessingParams output_processing_params(output_processing_params_el);
      m_ptrSettings->setOutputProcessingParams(page_id, output_processing_params);
    }

    const QDomElement output_params_el(el.namedItem("output-params").toElement());
    if (!output_params_el.isNull()) {
      const OutputParams output_params(output_params_el);
      m_ptrSettings->setOutputParams(page_id, output_params);
    }
  }
}  // Filter::loadSettings

intrusive_ptr<Task> Filter::createTask(const PageId& page_id,
                                       intrusive_ptr<ThumbnailPixmapCache> thumbnail_cache,
                                       const OutputFileNameGenerator& out_file_name_gen,
                                       const bool batch,
                                       const bool debug) {
  ImageViewTab lastTab(TAB_OUTPUT);
  if (m_ptrOptionsWidget.get() != nullptr) {
    lastTab = m_ptrOptionsWidget->lastTab();
  }

  return make_intrusive<Task>(intrusive_ptr<Filter>(this), m_ptrSettings, std::move(thumbnail_cache), page_id,
                              out_file_name_gen, lastTab, batch, debug);
}

intrusive_ptr<CacheDrivenTask> Filter::createCacheDrivenTask(const OutputFileNameGenerator& out_file_name_gen) {
  return make_intrusive<CacheDrivenTask>(m_ptrSettings, out_file_name_gen);
}

void Filter::loadDefaultSettings(const PageInfo& page_info) {
  if (!m_ptrSettings->isParamsNull(page_info.id())) {
    return;
  }
  const DefaultParams defaultParams = DefaultParamsProvider::getInstance()->getParams();
  const DefaultParams::OutputParams& outputParams = defaultParams.getOutputParams();

  m_ptrSettings->setParams(
      page_info.id(),
      Params(outputParams.getDpi(), outputParams.getColorParams(), outputParams.getSplittingOptions(),
             outputParams.getPictureShapeOptions(), dewarping::DistortionModel(), outputParams.getDepthPerception(),
             outputParams.getDewarpingOptions(), outputParams.getDespeckleLevel()));
}

OptionsWidget* Filter::optionsWidget() {
  return m_ptrOptionsWidget.get();
}
}  // namespace output