// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Filter.h"

#include <OrderByDeviationProvider.h>
#include <filters/select_content/CacheDrivenTask.h>
#include <filters/select_content/Task.h>

#include <utility>

#include "AbstractRelinker.h"
#include "CacheDrivenTask.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Task.h"
#include "Utils.h"

namespace deskew {
Filter::Filter(const PageSelectionAccessor& pageSelectionAccessor)
    : m_settings(new Settings), m_imageSettings(new ImageSettings), m_selectedPageOrder(0) {
  m_optionsWidget.reset(new OptionsWidget(m_settings, pageSelectionAccessor));

  const PageOrderOption::ProviderPtr defaultOrder;
  const auto orderByDeviation = make_intrusive<OrderByDeviationProvider>(m_settings->deviationProvider());
  m_pageOrderOptions.emplace_back(tr("Natural order"), defaultOrder);
  m_pageOrderOptions.emplace_back(tr("Order by decreasing deviation"), orderByDeviation);
}

Filter::~Filter() = default;

QString Filter::getName() const {
  return QCoreApplication::translate("deskew::Filter", "Deskew");
}

PageView Filter::getView() const {
  return PAGE_VIEW;
}

void Filter::performRelinking(const AbstractRelinker& relinker) {
  m_settings->performRelinking(relinker);
  m_imageSettings->performRelinking(relinker);
}

void Filter::preUpdateUI(FilterUiInterface* const ui, const PageInfo& pageInfo) {
  m_optionsWidget->preUpdateUI(pageInfo.id());
  ui->setOptionsWidget(m_optionsWidget.get(), ui->KEEP_OWNERSHIP);
}

QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filterEl(doc.createElement("deskew"));

  writer.enumPages(
      [&](const PageId& pageId, const int numericId) { this->writeParams(doc, filterEl, pageId, numericId); });

  saveImageSettings(writer, doc, filterEl);
  return filterEl;
}

void Filter::loadSettings(const ProjectReader& reader, const QDomElement& filtersEl) {
  m_settings->clear();

  const QDomElement filterEl(filtersEl.namedItem("deskew").toElement());

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

    const QDomElement paramsEl(el.namedItem("params").toElement());
    if (paramsEl.isNull()) {
      continue;
    }

    const Params params(paramsEl);
    m_settings->setPageParams(pageId, params);
  }

  loadImageSettings(reader, filterEl.namedItem("image-settings").toElement());
}  // Filter::loadSettings

void Filter::writeParams(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const {
  const std::unique_ptr<Params> params(m_settings->getPageParams(pageId));
  if (!params) {
    return;
  }

  QDomElement pageEl(doc.createElement("page"));
  pageEl.setAttribute("id", numericId);
  pageEl.appendChild(params->toXml(doc, "params"));

  filterEl.appendChild(pageEl);
}

intrusive_ptr<Task> Filter::createTask(const PageId& pageId,
                                       intrusive_ptr<select_content::Task> nextTask,
                                       const bool batchProcessing,
                                       const bool debug) {
  return make_intrusive<Task>(intrusive_ptr<Filter>(this), m_settings, m_imageSettings, std::move(nextTask), pageId,
                              batchProcessing, debug);
}

intrusive_ptr<CacheDrivenTask> Filter::createCacheDrivenTask(intrusive_ptr<select_content::CacheDrivenTask> nextTask) {
  return make_intrusive<CacheDrivenTask>(m_settings, std::move(nextTask));
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

void Filter::loadDefaultSettings(const PageInfo& pageInfo) {
  if (!m_settings->isParamsNull(pageInfo.id()))
    return;

  m_settings->setPageParams(pageInfo.id(), Utils::buildDefaultParams());
}

OptionsWidget* Filter::optionsWidget() {
  return m_optionsWidget.get();
}

void Filter::saveImageSettings(const ProjectWriter& writer, QDomDocument& doc, QDomElement& filterEl) const {
  QDomElement imageSettingsEl(doc.createElement("image-settings"));
  writer.enumPages([&](const PageId& pageId, const int numericId) {
    this->writeImageParams(doc, imageSettingsEl, pageId, numericId);
  });

  filterEl.appendChild(imageSettingsEl);
}

void Filter::writeImageParams(QDomDocument& doc, QDomElement& filterEl, const PageId& pageId, int numericId) const {
  const std::unique_ptr<ImageSettings::PageParams> params(m_imageSettings->getPageParams(pageId));
  if (!params) {
    return;
  }

  QDomElement pageEl(doc.createElement("page"));
  pageEl.setAttribute("id", numericId);
  pageEl.appendChild(params->toXml(doc, "image-params"));

  filterEl.appendChild(pageEl);
}

void Filter::loadImageSettings(const ProjectReader& reader, const QDomElement& imageSettingsEl) {
  m_imageSettings->clear();

  const QString pageTagName("page");
  QDomNode node(imageSettingsEl.firstChild());
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

    const QDomElement paramsEl(el.namedItem("image-params").toElement());
    if (paramsEl.isNull()) {
      continue;
    }

    const ImageSettings::PageParams params(paramsEl);
    m_imageSettings->setPageParams(pageId, params);
  }
}
}  // namespace deskew