// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Filter.h"

#include <DefaultParams.h>
#include <DefaultParamsProvider.h>
#include <OrderByCompletenessProvider.h>
#include <tiff.h>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <utility>

#include "CacheDrivenTask.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Settings.h"
#include "Task.h"
#include "ThumbnailPixmapCache.h"

namespace output {
Filter::Filter(const PageSelectionAccessor& pageSelectionAccessor) : m_settings(new Settings), m_selectedPageOrder(0) {
  m_optionsWidget.reset(new OptionsWidget(m_settings, pageSelectionAccessor));

  const PageOrderOption::ProviderPtr defaultOrder;
  const auto orderByCompleteness = make_intrusive<OrderByCompletenessProvider>();
  m_pageOrderOptions.emplace_back(tr("Natural order"), defaultOrder);
  m_pageOrderOptions.emplace_back(tr("Order by completeness"), orderByCompleteness);
}

Filter::~Filter() = default;

QString Filter::getName() const {
  return QCoreApplication::translate("output::Filter", "Output");
}

PageView Filter::getView() const {
  return PAGE_VIEW;
}

void Filter::performRelinking(const AbstractRelinker& relinker) {
  m_settings->performRelinking(relinker);
}

void Filter::preUpdateUI(FilterUiInterface* ui, const PageInfo& pageInfo) {
  m_optionsWidget->preUpdateUI(pageInfo.id());
  ui->setOptionsWidget(m_optionsWidget.get(), ui->KEEP_OWNERSHIP);
}

QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filterEl(doc.createElement("output"));

  writer.enumPages(
      [&](const PageId& pageId, int numericId) { this->writePageSettings(doc, filterEl, pageId, numericId); });
  return filterEl;
}

void Filter::writePageSettings(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const {
  const Params params(m_settings->getParams(pageId));

  QDomElement pageEl(doc.createElement("page"));
  pageEl.setAttribute("id", numericId);

  pageEl.appendChild(m_settings->pictureZonesForPage(pageId).toXml(doc, "zones"));
  pageEl.appendChild(m_settings->fillZonesForPage(pageId).toXml(doc, "fill-zones"));
  pageEl.appendChild(params.toXml(doc, "params"));
  pageEl.appendChild(m_settings->getOutputProcessingParams(pageId).toXml(doc, "processing-params"));

  std::unique_ptr<OutputParams> outputParams(m_settings->getOutputParams(pageId));
  if (outputParams) {
    pageEl.appendChild(outputParams->toXml(doc, "output-params"));
  }

  filterEl.appendChild(pageEl);
}

void Filter::loadSettings(const ProjectReader& reader, const QDomElement& filtersEl) {
  m_settings->clear();

  const QDomElement filterEl(filtersEl.namedItem("output").toElement());

  const QString pageTagName("page");
  QDomNode node(filterEl.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != pageTagName) {
      continue;
    }
    const QDomElement el(node.toElement());

    bool ok = true;
    const int id = el.attribute("id").toInt(&ok);
    if (!ok) {
      continue;
    }

    const PageId pageId(reader.pageId(id));
    if (pageId.isNull()) {
      continue;
    }

    const ZoneSet pictureZones(el.namedItem("zones").toElement(), m_pictureZonePropFactory);
    if (!pictureZones.empty()) {
      m_settings->setPictureZones(pageId, pictureZones);
    }

    const ZoneSet fillZones(el.namedItem("fill-zones").toElement(), m_fillZonePropFactory);
    if (!fillZones.empty()) {
      m_settings->setFillZones(pageId, fillZones);
    }

    const QDomElement paramsEl(el.namedItem("params").toElement());
    if (!paramsEl.isNull()) {
      const Params params(paramsEl);
      m_settings->setParams(pageId, params);
    }

    const QDomElement outputProcessingParamsEl(el.namedItem("processing-params").toElement());
    if (!outputProcessingParamsEl.isNull()) {
      const OutputProcessingParams outputProcessingParams(outputProcessingParamsEl);
      m_settings->setOutputProcessingParams(pageId, outputProcessingParams);
    }

    const QDomElement outputParamsEl(el.namedItem("output-params").toElement());
    if (!outputParamsEl.isNull()) {
      const OutputParams outputParams(outputParamsEl);
      m_settings->setOutputParams(pageId, outputParams);
    }
  }
}  // Filter::loadSettings

intrusive_ptr<Task> Filter::createTask(const PageId& pageId,
                                       intrusive_ptr<ThumbnailPixmapCache> thumbnailCache,
                                       const OutputFileNameGenerator& outFileNameGen,
                                       const bool batch,
                                       const bool debug) {
  ImageViewTab lastTab(TAB_OUTPUT);
  if (m_optionsWidget.get() != nullptr) {
    lastTab = m_optionsWidget->lastTab();
  }
  return make_intrusive<Task>(intrusive_ptr<Filter>(this), m_settings, std::move(thumbnailCache), pageId,
                              outFileNameGen, lastTab, batch, debug);
}

intrusive_ptr<CacheDrivenTask> Filter::createCacheDrivenTask(const OutputFileNameGenerator& outFileNameGen) {
  return make_intrusive<CacheDrivenTask>(m_settings, outFileNameGen);
}

void Filter::loadDefaultSettings(const PageInfo& pageInfo) {
  if (!m_settings->isParamsNull(pageInfo.id())) {
    return;
  }
  const DefaultParams defaultParams = DefaultParamsProvider::getInstance().getParams();
  const DefaultParams::OutputParams& outputParams = defaultParams.getOutputParams();

  m_settings->setParams(pageInfo.id(), Params(outputParams.getDpi(), outputParams.getColorParams(),
                                              outputParams.getSplittingOptions(), outputParams.getPictureShapeOptions(),
                                              dewarping::DistortionModel(), outputParams.getDepthPerception(),
                                              outputParams.getDewarpingOptions(), outputParams.getDespeckleLevel()));
}

OptionsWidget* Filter::optionsWidget() {
  return m_optionsWidget.get();
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
}  // namespace output