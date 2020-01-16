// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CacheDrivenTask.h"

#include <utility>

#include "IncompleteThumbnail.h"
#include "PageInfo.h"
#include "PageLayoutAdapter.h"
#include "ProjectPages.h"
#include "Settings.h"
#include "Thumbnail.h"
#include "core/AbstractFilterDataCollector.h"
#include "core/ThumbnailCollector.h"
#include "filters/deskew/CacheDrivenTask.h"

namespace page_split {
CacheDrivenTask::CacheDrivenTask(intrusive_ptr<Settings> settings,
                                 intrusive_ptr<ProjectPages> projectPages,
                                 intrusive_ptr<deskew::CacheDrivenTask> nextTask)
    : m_nextTask(std::move(nextTask)), m_settings(std::move(settings)), m_projectPages(std::move(projectPages)) {}

CacheDrivenTask::~CacheDrivenTask() = default;

static ProjectPages::LayoutType toPageLayoutType(const PageLayout& layout) {
  switch (layout.type()) {
    case PageLayout::SINGLE_PAGE_UNCUT:
    case PageLayout::SINGLE_PAGE_CUT:
      return ProjectPages::ONE_PAGE_LAYOUT;
    case PageLayout::TWO_PAGES:
      return ProjectPages::TWO_PAGE_LAYOUT;
  }

  assert(!"Unreachable");
  return ProjectPages::ONE_PAGE_LAYOUT;
}

void CacheDrivenTask::process(const PageInfo& pageInfo,
                              AbstractFilterDataCollector* collector,
                              const ImageTransformation& xform) {
  const Settings::Record record(m_settings->getPageRecord(pageInfo.imageId()));

  const OrthogonalRotation preRotation(xform.preRotation());
  const Dependencies deps(pageInfo.metadata().size(), preRotation, record.combinedLayoutType());

  const Params* params = record.params();

  if (!params || !deps.compatibleWith(*params)) {
    if (auto* thumbCol = dynamic_cast<ThumbnailCollector*>(collector)) {
      thumbCol->processThumbnail(std::unique_ptr<QGraphicsItem>(new IncompleteThumbnail(
          thumbCol->thumbnailCache(), thumbCol->maxLogicalThumbSize(), pageInfo.imageId(), xform)));
    }
    return;
  }

  PageLayout layout(params->pageLayout());
  PageLayoutAdapter::correctPageLayoutType(&layout);
  // m_projectPages controls number of pages displayed in thumbnail list
  // usually this is set in Task, but if user changed layout with Apply To..
  // and just jumped to next stage - the Task::process isn't invoked for all pages
  // so we must additionally ensure here that we display right number of pages.
  m_projectPages->setLayoutTypeFor(pageInfo.id().imageId(), toPageLayoutType(layout));

  if (m_nextTask) {
    ImageTransformation newXform(xform);
    newXform.setPreCropArea(layout.pageOutline(pageInfo.id().subPage()).toPolygon());
    m_nextTask->process(pageInfo, collector, newXform);
    return;
  }

  if (auto* thumbCol = dynamic_cast<ThumbnailCollector*>(collector)) {
    thumbCol->processThumbnail(std::unique_ptr<QGraphicsItem>(
        new Thumbnail(thumbCol->thumbnailCache(), thumbCol->maxLogicalThumbSize(), pageInfo.imageId(), xform, layout,
                      pageInfo.leftHalfRemoved(), pageInfo.rightHalfRemoved())));
  }
}  // CacheDrivenTask::process
}  // namespace page_split