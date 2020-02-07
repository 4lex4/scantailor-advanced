// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Filter.h"

#include <filters/deskew/CacheDrivenTask.h>
#include <filters/deskew/Task.h>

#include <utility>

#include "CacheDrivenTask.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "OrderBySplitTypeProvider.h"
#include "ProjectPages.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Settings.h"
#include "Task.h"
#include "Utils.h"

namespace page_split {
Filter::Filter(intrusive_ptr<ProjectPages> pageSequence, const PageSelectionAccessor& pageSelectionAccessor)
    : m_pages(std::move(pageSequence)), m_settings(new Settings), m_selectedPageOrder(0) {
  m_optionsWidget.reset(new OptionsWidget(m_settings, m_pages, pageSelectionAccessor));

  const PageOrderOption::ProviderPtr defaultOrder;
  const auto orderBySplitType = make_intrusive<OrderBySplitTypeProvider>(m_settings);
  m_pageOrderOptions.emplace_back(tr("Natural order"), defaultOrder);
  m_pageOrderOptions.emplace_back(tr("Order by split type"), orderBySplitType);
}

Filter::~Filter() = default;

QString Filter::getName() const {
  return QCoreApplication::translate("page_split::Filter", "Split Pages");
}

PageView Filter::getView() const {
  return IMAGE_VIEW;
}

void Filter::performRelinking(const AbstractRelinker& relinker) {
  m_settings->performRelinking(relinker);
}

void Filter::preUpdateUI(FilterUiInterface* ui, const PageInfo& pageInfo) {
  m_optionsWidget->preUpdateUI(pageInfo.id());
  ui->setOptionsWidget(m_optionsWidget.get(), ui->KEEP_OWNERSHIP);
}

QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filterEl(doc.createElement("page-split"));
  filterEl.setAttribute("defaultLayoutType", layoutTypeToString(m_settings->defaultLayoutType()));

  writer.enumImages([&](const ImageId& imageId, const int numericId) {
    this->writeImageSettings(doc, filterEl, imageId, numericId);
  });
  return filterEl;
}

void Filter::loadSettings(const ProjectReader& reader, const QDomElement& filtersEl) {
  m_settings->clear();

  const QDomElement filterEl(filtersEl.namedItem("page-split").toElement());
  const QString defaultLayoutType(filterEl.attribute("defaultLayoutType"));
  m_settings->setLayoutTypeForAllPages(layoutTypeFromString(defaultLayoutType));

  const QString imageTagName("image");
  QDomNode node(filterEl.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != imageTagName) {
      continue;
    }
    QDomElement el(node.toElement());

    bool ok = true;
    const int id = el.attribute("id").toInt(&ok);
    if (!ok) {
      continue;
    }

    const ImageId imageId(reader.imageId(id));
    if (imageId.isNull()) {
      continue;
    }

    Settings::UpdateAction update;

    const QString layoutType(el.attribute("layoutType"));
    if (!layoutType.isEmpty()) {
      update.setLayoutType(layoutTypeFromString(layoutType));
    }

    QDomElement paramsEl(el.namedItem("params").toElement());
    if (!paramsEl.isNull()) {
      update.setParams(Params(paramsEl));
    }

    m_settings->updatePage(imageId, update);
  }
}  // Filter::loadSettings

void Filter::pageOrientationUpdate(const ImageId& imageId, const OrthogonalRotation& orientation) {
  const Settings::Record record(m_settings->getPageRecord(imageId));

  if (record.layoutType() && (*record.layoutType() != AUTO_LAYOUT_TYPE)) {
    // The layout type was set manually, so we don't care about orientation.
    return;
  }

  if (record.params() && (record.params()->dependencies().orientation() == orientation)) {
    // We've already estimated the number of pages for this orientation.
    return;
  }

  // Use orientation to update the number of logical pages in an image.
  m_pages->autoSetLayoutTypeFor(imageId, orientation);
}

void Filter::writeImageSettings(QDomDocument& doc,
                                QDomElement& filterEl,
                                const ImageId& imageId,
                                const int numericId) const {
  const Settings::Record record(m_settings->getPageRecord(imageId));

  QDomElement imageEl(doc.createElement("image"));
  imageEl.setAttribute("id", numericId);
  if (const LayoutType* layoutType = record.layoutType()) {
    imageEl.setAttribute("layoutType", layoutTypeToString(*layoutType));
  }

  if (const Params* params = record.params()) {
    imageEl.appendChild(params->toXml(doc, "params"));
    filterEl.appendChild(imageEl);
  }
}

intrusive_ptr<Task> Filter::createTask(const PageInfo& pageInfo,
                                       intrusive_ptr<deskew::Task> nextTask,
                                       const bool batchProcessing,
                                       const bool debug) {
  return make_intrusive<Task>(intrusive_ptr<Filter>(this), m_settings, m_pages, std::move(nextTask), pageInfo,
                              batchProcessing, debug);
}

intrusive_ptr<CacheDrivenTask> Filter::createCacheDrivenTask(intrusive_ptr<deskew::CacheDrivenTask> nextTask) {
  return make_intrusive<CacheDrivenTask>(m_settings, m_pages, std::move(nextTask));
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
  if (!m_settings->getPageRecord(pageInfo.id().imageId()).isNull())
    return;

  m_settings->updatePage(pageInfo.id().imageId(), Utils::buildDefaultUpdateAction());
}

OptionsWidget* Filter::optionsWidget() {
  return m_optionsWidget.get();
}
}  // namespace page_split
