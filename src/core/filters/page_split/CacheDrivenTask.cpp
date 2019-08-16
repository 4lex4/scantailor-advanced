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
                                 intrusive_ptr<deskew::CacheDrivenTask> next_task)
    : m_nextTask(std::move(next_task)), m_settings(std::move(settings)), m_projectPages(std::move(projectPages)) {}

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

void CacheDrivenTask::process(const PageInfo& page_info,
                              AbstractFilterDataCollector* collector,
                              const ImageTransformation& xform) {
  const Settings::Record record(m_settings->getPageRecord(page_info.imageId()));

  const OrthogonalRotation pre_rotation(xform.preRotation());
  const Dependencies deps(page_info.metadata().size(), pre_rotation, record.combinedLayoutType());

  const Params* params = record.params();

  if (!params || !deps.compatibleWith(*params)) {
    if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
      thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(new IncompleteThumbnail(
          thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), xform)));
    }

    return;
  }

  PageLayout layout(params->pageLayout());
  PageLayoutAdapter::correctPageLayoutType(&layout);
  // m_projectPages controls number of pages displayed in thumbnail list
  // usually this is set in Task, but if user changed layout with Apply To..
  // and just jumped to next stage - the Task::process isn't invoked for all pages
  // so we must additionally ensure here that we display right number of pages.
  m_projectPages->setLayoutTypeFor(page_info.id().imageId(), toPageLayoutType(layout));

  if (m_nextTask) {
    ImageTransformation new_xform(xform);
    new_xform.setPreCropArea(layout.pageOutline(page_info.id().subPage()).toPolygon());
    m_nextTask->process(page_info, collector, new_xform);

    return;
  }

  if (auto* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
    thumb_col->processThumbnail(std::unique_ptr<QGraphicsItem>(
        new Thumbnail(thumb_col->thumbnailCache(), thumb_col->maxLogicalThumbSize(), page_info.imageId(), xform, layout,
                      page_info.leftHalfRemoved(), page_info.rightHalfRemoved())));
  }
}  // CacheDrivenTask::process
}  // namespace page_split