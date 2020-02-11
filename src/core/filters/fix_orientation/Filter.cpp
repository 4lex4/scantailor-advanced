// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Filter.h"

#include <filters/page_split/CacheDrivenTask.h>
#include <filters/page_split/Task.h>

#include <QCoreApplication>
#include <utility>

#include "CacheDrivenTask.h"
#include "FilterUiInterface.h"
#include "ImageSettings.h"
#include "OptionsWidget.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "Settings.h"
#include "Task.h"
#include "Utils.h"
#include "XmlMarshaller.h"

namespace fix_orientation {
Filter::Filter(const PageSelectionAccessor& pageSelectionAccessor)
    : m_settings(std::make_shared<Settings>()), m_imageSettings(std::make_shared<ImageSettings>()) {
  m_optionsWidget.reset(new OptionsWidget(m_settings, pageSelectionAccessor));
}

Filter::~Filter() = default;

QString Filter::getName() const {
  return QCoreApplication::translate("fix_orientation::Filter", "Fix Orientation");
}

PageView Filter::getView() const {
  return IMAGE_VIEW;
}

void Filter::performRelinking(const AbstractRelinker& relinker) {
  m_settings->performRelinking(relinker);
  m_imageSettings->performRelinking(relinker);
}

void Filter::preUpdateUI(FilterUiInterface* ui, const PageInfo& pageInfo) {
  if (m_optionsWidget.get()) {
    const OrthogonalRotation rotation(m_settings->getRotationFor(pageInfo.id().imageId()));
    m_optionsWidget->preUpdateUI(pageInfo.id(), rotation);
    ui->setOptionsWidget(m_optionsWidget.get(), ui->KEEP_OWNERSHIP);
  }
}

QDomElement Filter::saveSettings(const ProjectWriter& writer, QDomDocument& doc) const {
  QDomElement filterEl(doc.createElement("fix-orientation"));
  writer.enumImages(
      [&](const ImageId& imageId, const int numericId) { this->writeParams(doc, filterEl, imageId, numericId); });

  saveImageSettings(writer, doc, filterEl);
  return filterEl;
}

void Filter::loadSettings(const ProjectReader& reader, const QDomElement& filtersEl) {
  m_settings->clear();

  QDomElement filterEl(filtersEl.namedItem("fix-orientation").toElement());

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

    const OrthogonalRotation rotation(el.namedItem("rotation").toElement());

    m_settings->applyRotation(imageId, rotation);
  }

  loadImageSettings(reader, filterEl.namedItem("image-settings").toElement());
}  // Filter::loadSettings

std::shared_ptr<Task> Filter::createTask(const PageId& pageId,
                                         std::shared_ptr<page_split::Task> nextTask,
                                         const bool batchProcessing) {
  return std::make_shared<Task>(pageId, std::static_pointer_cast<Filter>(shared_from_this()), m_settings,
                                m_imageSettings, std::move(nextTask), batchProcessing);
}

std::shared_ptr<CacheDrivenTask> Filter::createCacheDrivenTask(std::shared_ptr<page_split::CacheDrivenTask> nextTask) {
  return std::make_shared<CacheDrivenTask>(m_settings, std::move(nextTask));
}

void Filter::writeParams(QDomDocument& doc, QDomElement& filterEl, const ImageId& imageId, int numericId) const {
  const OrthogonalRotation rotation(m_settings->getRotationFor(imageId));
  if (rotation.toDegrees() == 0) {
    return;
  }

  XmlMarshaller marshaller(doc);

  QDomElement imageEl(doc.createElement("image"));
  imageEl.setAttribute("id", numericId);
  imageEl.appendChild(rotation.toXml(doc, "rotation"));
  filterEl.appendChild(imageEl);
}

void Filter::loadDefaultSettings(const PageInfo& pageInfo) {
  if (!m_settings->isRotationNull(pageInfo.id().imageId()))
    return;

  m_settings->applyRotation(pageInfo.id().imageId(), Utils::getDefaultOrthogonalRotation());
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
}  // namespace fix_orientation