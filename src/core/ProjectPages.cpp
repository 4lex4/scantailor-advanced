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

#include "ProjectPages.h"
#include <QDebug>
#include <boost/foreach.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <unordered_map>
#include "AbstractRelinker.h"
#include "ImageFileInfo.h"
#include "ImageInfo.h"
#include "OrthogonalRotation.h"
#include "PageSequence.h"
#include "RelinkablePath.h"

ProjectPages::ProjectPages(const Qt::LayoutDirection layout_direction) {
  initSubPagesInOrder(layout_direction);
}

ProjectPages::ProjectPages(const std::vector<ImageInfo>& info, const Qt::LayoutDirection layout_direction) {
  initSubPagesInOrder(layout_direction);

  for (const ImageInfo& image : info) {
    ImageDesc image_desc(image);
    // Enforce some rules.
    if (image_desc.numLogicalPages == 2) {
      image_desc.leftHalfRemoved = false;
      image_desc.rightHalfRemoved = false;
    } else if (image_desc.numLogicalPages != 1) {
      continue;
    } else if (image_desc.leftHalfRemoved && image_desc.rightHalfRemoved) {
      image_desc.leftHalfRemoved = false;
      image_desc.rightHalfRemoved = false;
    }

    m_images.push_back(image_desc);
  }
}

ProjectPages::ProjectPages(const std::vector<ImageFileInfo>& files,
                           const Pages pages,
                           const Qt::LayoutDirection layout_direction) {
  initSubPagesInOrder(layout_direction);

  for (const ImageFileInfo& file : files) {
    const QString& file_path = file.fileInfo().absoluteFilePath();
    const std::vector<ImageMetadata>& images = file.imageInfo();
    const auto num_images = static_cast<const int>(images.size());
    const int multi_page_base = num_images > 1 ? 1 : 0;
    for (int i = 0; i < num_images; ++i) {
      const ImageMetadata& metadata = images[i];
      const ImageId id(file_path, multi_page_base + i);
      m_images.emplace_back(id, metadata, pages);
    }
  }
}

ProjectPages::~ProjectPages() = default;

Qt::LayoutDirection ProjectPages::layoutDirection() const {
  if (m_subPagesInOrder[0] == PageId::LEFT_PAGE) {
    return Qt::LeftToRight;
  } else {
    assert(m_subPagesInOrder[0] == PageId::RIGHT_PAGE);

    return Qt::RightToLeft;
  }
}

void ProjectPages::initSubPagesInOrder(const Qt::LayoutDirection layout_direction) {
  if (layout_direction == Qt::LeftToRight) {
    m_subPagesInOrder[0] = PageId::LEFT_PAGE;
    m_subPagesInOrder[1] = PageId::RIGHT_PAGE;
  } else {
    m_subPagesInOrder[0] = PageId::RIGHT_PAGE;
    m_subPagesInOrder[1] = PageId::LEFT_PAGE;
  }
}

PageSequence ProjectPages::toPageSequence(const PageView view) const {
  PageSequence pages;

  if (view == PAGE_VIEW) {
    QMutexLocker locker(&m_mutex);

    const auto num_images = static_cast<const int>(m_images.size());
    for (int i = 0; i < num_images; ++i) {
      const ImageDesc& image = m_images[i];
      assert(image.numLogicalPages >= 1 && image.numLogicalPages <= 2);
      for (int j = 0; j < image.numLogicalPages; ++j) {
        const PageId id(image.id, image.logicalPageToSubPage(j, m_subPagesInOrder));
        pages.append(
            PageInfo(id, image.metadata, image.numLogicalPages, image.leftHalfRemoved, image.rightHalfRemoved));
      }
    }
  } else {
    assert(view == IMAGE_VIEW);

    QMutexLocker locker(&m_mutex);

    const auto num_images = static_cast<const int>(m_images.size());
    for (int i = 0; i < num_images; ++i) {
      const ImageDesc& image = m_images[i];
      const PageId id(image.id, PageId::SINGLE_PAGE);
      pages.append(PageInfo(id, image.metadata, image.numLogicalPages, image.leftHalfRemoved, image.rightHalfRemoved));
    }
  }

  return pages;
}  // ProjectPages::toPageSequence

void ProjectPages::listRelinkablePaths(const VirtualFunction<void, const RelinkablePath&>& sink) const {
  // It's generally a bad idea to do callbacks while holding an internal mutex,
  // so we accumulate results into this vector first.
  std::vector<QString> files;

  {
    QMutexLocker locker(&m_mutex);

    files.reserve(m_images.size());
    for (const ImageDesc& image : m_images) {
      files.push_back(image.id.filePath());
    }
  }

  for (const QString& file : files) {
    sink(RelinkablePath(file, RelinkablePath::File));
  }
}

void ProjectPages::performRelinking(const AbstractRelinker& relinker) {
  QMutexLocker locker(&m_mutex);

  for (ImageDesc& image : m_images) {
    const RelinkablePath old_path(image.id.filePath(), RelinkablePath::File);
    const QString new_path(relinker.substitutionPathFor(old_path));
    image.id.setFilePath(new_path);
  }
}

void ProjectPages::setLayoutTypeFor(const ImageId& image_id, const LayoutType layout) {
  bool was_modified = false;

  {
    QMutexLocker locker(&m_mutex);
    setLayoutTypeForImpl(image_id, layout, &was_modified);
  }

  if (was_modified) {
    emit modified();
  }
}

void ProjectPages::setLayoutTypeForAllPages(const LayoutType layout) {
  bool was_modified = false;

  {
    QMutexLocker locker(&m_mutex);
    setLayoutTypeForAllPagesImpl(layout, &was_modified);
  }

  if (was_modified) {
    emit modified();
  }
}

void ProjectPages::autoSetLayoutTypeFor(const ImageId& image_id, const OrthogonalRotation rotation) {
  bool was_modified = false;

  {
    QMutexLocker locker(&m_mutex);
    autoSetLayoutTypeForImpl(image_id, rotation, &was_modified);
  }

  if (was_modified) {
    emit modified();
  }
}

void ProjectPages::updateImageMetadata(const ImageId& image_id, const ImageMetadata& metadata) {
  bool was_modified = false;

  {
    QMutexLocker locker(&m_mutex);
    updateImageMetadataImpl(image_id, metadata, &was_modified);
  }

  if (was_modified) {
    emit modified();
  }
}

int ProjectPages::adviseNumberOfLogicalPages(const ImageMetadata& metadata, const OrthogonalRotation rotation) {
  const QSize size(rotation.rotate(metadata.size()));
  const QSize dpi(rotation.rotate(metadata.dpi().toSize()));

  if (size.width() * dpi.height() > size.height() * dpi.width()) {
    return 2;
  } else {
    return 1;
  }
}

int ProjectPages::numImages() const {
  QMutexLocker locker(&m_mutex);

  return static_cast<int>(m_images.size());
}

std::vector<PageInfo> ProjectPages::insertImage(const ImageInfo& new_image,
                                                BeforeOrAfter before_or_after,
                                                const ImageId& existing,
                                                const PageView view) {
  bool was_modified = false;

  {
    QMutexLocker locker(&m_mutex);

    return insertImageImpl(new_image, before_or_after, existing, view, was_modified);
  }

  if (was_modified) {
    emit modified();
  }
}

void ProjectPages::removePages(const std::set<PageId>& pages) {
  bool was_modified = false;

  {
    QMutexLocker locker(&m_mutex);
    removePagesImpl(pages, was_modified);
  }

  if (was_modified) {
    emit modified();
  }
}

PageInfo ProjectPages::unremovePage(const PageId& page_id) {
  bool was_modified = false;

  PageInfo page_info;

  {
    QMutexLocker locker(&m_mutex);
    page_info = unremovePageImpl(page_id, was_modified);
  }

  if (was_modified) {
    emit modified();
  }

  return page_info;
}

bool ProjectPages::validateDpis() const {
  QMutexLocker locker(&m_mutex);

  for (const ImageDesc& image : m_images) {
    if (!image.metadata.isDpiOK()) {
      return false;
    }
  }

  return true;
}

namespace {
struct File {
  QString fileName;
  mutable std::vector<ImageMetadata> metadata;

  explicit File(const QString& fname) : fileName(fname) {}

  explicit operator ImageFileInfo() const { return ImageFileInfo(fileName, metadata); }
};
}  // anonymous namespace

std::vector<ImageFileInfo> ProjectPages::toImageFileInfo() const {
  using namespace boost::multi_index;

  multi_index_container<File, indexed_by<ordered_unique<member<File, QString, &File::fileName>>, sequenced<>>> files;

  {
    QMutexLocker locker(&m_mutex);

    for (const ImageDesc& image : m_images) {
      const File file(image.id.filePath());
      files.insert(file).first->metadata.push_back(image.metadata);
    }
  }

  return std::vector<ImageFileInfo>(files.get<1>().begin(), files.get<1>().end());
}

void ProjectPages::updateMetadataFrom(const std::vector<ImageFileInfo>& files) {
  typedef std::unordered_map<ImageId, ImageMetadata> MetadataMap;
  MetadataMap metadata_map;

  for (const ImageFileInfo& file : files) {
    const QString file_path(file.fileInfo().absoluteFilePath());
    int page = 0;
    for (const ImageMetadata& metadata : file.imageInfo()) {
      metadata_map[ImageId(file_path, page)] = metadata;
      ++page;
    }
  }

  QMutexLocker locker(&m_mutex);

  for (ImageDesc& image : m_images) {
    const MetadataMap::const_iterator it(metadata_map.find(image.id));
    if (it != metadata_map.end()) {
      image.metadata = it->second;
    }
  }
}

void ProjectPages::setLayoutTypeForImpl(const ImageId& image_id, const LayoutType layout, bool* modified) {
  const int num_pages = (layout == TWO_PAGE_LAYOUT ? 2 : 1);
  const auto num_images = static_cast<const int>(m_images.size());
  for (int i = 0; i < num_images; ++i) {
    ImageDesc& image = m_images[i];
    if (image.id == image_id) {
      int adjusted_num_pages = num_pages;
      if ((num_pages == 2) && (image.leftHalfRemoved != image.rightHalfRemoved)) {
        // Both can't be removed, but we handle that case anyway
        // by treating it like none are removed.
        --adjusted_num_pages;
      }

      const int delta = adjusted_num_pages - image.numLogicalPages;
      if (delta == 0) {
        break;
      }

      image.numLogicalPages = adjusted_num_pages;

      *modified = true;
      break;
    }
  }
}

void ProjectPages::setLayoutTypeForAllPagesImpl(const LayoutType layout, bool* modified) {
  const int num_pages = (layout == TWO_PAGE_LAYOUT ? 2 : 1);
  const auto num_images = static_cast<const int>(m_images.size());
  for (int i = 0; i < num_images; ++i) {
    ImageDesc& image = m_images[i];

    int adjusted_num_pages = num_pages;
    if ((num_pages == 2) && (image.leftHalfRemoved != image.rightHalfRemoved)) {
      // Both can't be removed, but we handle that case anyway
      // by treating it like none are removed.
      --adjusted_num_pages;
    }

    const int delta = adjusted_num_pages - image.numLogicalPages;
    if (delta == 0) {
      continue;
    }

    image.numLogicalPages = adjusted_num_pages;
    *modified = true;
  }
}

void ProjectPages::autoSetLayoutTypeForImpl(const ImageId& image_id,
                                            const OrthogonalRotation rotation,
                                            bool* modified) {
  const auto num_images = static_cast<const int>(m_images.size());
  for (int i = 0; i < num_images; ++i) {
    ImageDesc& image = m_images[i];
    if (image.id == image_id) {
      int num_pages = adviseNumberOfLogicalPages(image.metadata, rotation);
      if ((num_pages == 2) && (image.leftHalfRemoved != image.rightHalfRemoved)) {
        // Both can't be removed, but we handle that case anyway
        // by treating it like none are removed.
        --num_pages;
      }

      const int delta = num_pages - image.numLogicalPages;
      if (delta == 0) {
        break;
      }

      image.numLogicalPages = num_pages;

      *modified = true;
      break;
    }
  }
}

void ProjectPages::updateImageMetadataImpl(const ImageId& image_id, const ImageMetadata& metadata, bool* modified) {
  const auto num_images = static_cast<const int>(m_images.size());
  for (int i = 0; i < num_images; ++i) {
    ImageDesc& image = m_images[i];
    if (image.id == image_id) {
      if (image.metadata != metadata) {
        image.metadata = metadata;
        *modified = true;
      }
      break;
    }
  }
}

std::vector<PageInfo> ProjectPages::insertImageImpl(const ImageInfo& new_image,
                                                    BeforeOrAfter before_or_after,
                                                    const ImageId& existing,
                                                    const PageView view,
                                                    bool& modified) {
  std::vector<PageInfo> logical_pages;

  auto it(m_images.begin());
  const auto end(m_images.end());
  for (; it != end && it->id != existing; ++it) {
    // Continue until we find the existing image.
  }
  if (it == end) {
    // Existing image not found.
    if (!((before_or_after == BEFORE) && existing.isNull())) {
      return logical_pages;
    }  // Otherwise we can still handle that case.
  }
  if (before_or_after == AFTER) {
    ++it;
  }

  ImageDesc image_desc(new_image);

  // Enforce some rules.
  if (image_desc.numLogicalPages == 2) {
    image_desc.leftHalfRemoved = false;
    image_desc.rightHalfRemoved = false;
  } else if (image_desc.numLogicalPages != 1) {
    return logical_pages;
  } else if (image_desc.leftHalfRemoved && image_desc.rightHalfRemoved) {
    image_desc.leftHalfRemoved = false;
    image_desc.rightHalfRemoved = false;
  }

  m_images.insert(it, image_desc);

  PageInfo page_info_templ(PageId(new_image.id(), PageId::SINGLE_PAGE), image_desc.metadata, image_desc.numLogicalPages,
                           image_desc.leftHalfRemoved, image_desc.rightHalfRemoved);

  if ((view == IMAGE_VIEW)
      || ((image_desc.numLogicalPages == 1) && (image_desc.leftHalfRemoved == image_desc.rightHalfRemoved))) {
    logical_pages.push_back(page_info_templ);
  } else {
    if ((image_desc.numLogicalPages == 2) || ((image_desc.numLogicalPages == 1) && image_desc.rightHalfRemoved)) {
      page_info_templ.setId(PageId(new_image.id(), m_subPagesInOrder[0]));
      logical_pages.push_back(page_info_templ);
    }
    if ((image_desc.numLogicalPages == 2) || ((image_desc.numLogicalPages == 1) && image_desc.leftHalfRemoved)) {
      page_info_templ.setId(PageId(new_image.id(), m_subPagesInOrder[1]));
      logical_pages.push_back(page_info_templ);
    }
  }

  return logical_pages;
}  // ProjectPages::insertImageImpl

void ProjectPages::removePagesImpl(const std::set<PageId>& to_remove, bool& modified) {
  const auto to_remove_end(to_remove.end());

  std::vector<ImageDesc> new_images;
  new_images.reserve(m_images.size());
  int new_total_logical_pages = 0;

  const auto num_old_images = static_cast<const int>(m_images.size());
  for (int i = 0; i < num_old_images; ++i) {
    ImageDesc image(m_images[i]);

    if (to_remove.find(PageId(image.id, PageId::SINGLE_PAGE)) != to_remove_end) {
      image.numLogicalPages = 0;
      modified = true;
    } else {
      if (to_remove.find(PageId(image.id, PageId::LEFT_PAGE)) != to_remove_end) {
        image.leftHalfRemoved = true;
        --image.numLogicalPages;
        modified = true;
      }
      if (to_remove.find(PageId(image.id, PageId::RIGHT_PAGE)) != to_remove_end) {
        image.rightHalfRemoved = true;
        --image.numLogicalPages;
        modified = true;
      }
    }

    if (image.numLogicalPages > 0) {
      new_images.push_back(image);
      new_total_logical_pages += new_images.back().numLogicalPages;
    }
  }

  new_images.swap(m_images);
}  // ProjectPages::removePagesImpl

PageInfo ProjectPages::unremovePageImpl(const PageId& page_id, bool& modified) {
  if (page_id.subPage() == PageId::SINGLE_PAGE) {
    // These can't be unremoved.
    return PageInfo();
  }

  auto it(m_images.begin());
  const auto end(m_images.end());
  for (; it != end && it->id != page_id.imageId(); ++it) {
    // Continue until we find the corresponding image.
  }
  if (it == end) {
    // The corresponding image wasn't found.
    return PageInfo();
  }

  ImageDesc& image = *it;

  if (image.numLogicalPages != 1) {
    return PageInfo();
  }

  if ((page_id.subPage() == PageId::LEFT_PAGE) && image.leftHalfRemoved) {
    image.leftHalfRemoved = false;
  } else if ((page_id.subPage() == PageId::RIGHT_PAGE) && image.rightHalfRemoved) {
    image.rightHalfRemoved = false;
  } else {
    return PageInfo();
  }

  image.numLogicalPages = 2;

  return PageInfo(page_id, image.metadata, image.numLogicalPages, image.leftHalfRemoved, image.rightHalfRemoved);
}  // ProjectPages::unremovePageImpl

/*========================= ProjectPages::ImageDesc ======================*/

ProjectPages::ImageDesc::ImageDesc(const ImageInfo& image_info)
    : id(image_info.id()),
      metadata(image_info.metadata()),
      numLogicalPages(image_info.numSubPages()),
      leftHalfRemoved(image_info.leftHalfRemoved()),
      rightHalfRemoved(image_info.rightHalfRemoved()) {}

ProjectPages::ImageDesc::ImageDesc(const ImageId& id, const ImageMetadata& metadata, const Pages pages)
    : id(id), metadata(metadata), leftHalfRemoved(false), rightHalfRemoved(false) {
  switch (pages) {
    case ONE_PAGE:
      numLogicalPages = 1;
      break;
    case TWO_PAGES:
      numLogicalPages = 2;
      break;
    case AUTO_PAGES:
      numLogicalPages = adviseNumberOfLogicalPages(metadata, OrthogonalRotation());
      break;
  }
}

PageId::SubPage ProjectPages::ImageDesc::logicalPageToSubPage(const int logical_page,
                                                              const PageId::SubPage* sub_pages_in_order) const {
  assert(numLogicalPages >= 1 && numLogicalPages <= 2);
  assert(logical_page >= 0 && logical_page < numLogicalPages);

  if (numLogicalPages == 1) {
    if (leftHalfRemoved && !rightHalfRemoved) {
      return PageId::RIGHT_PAGE;
    } else if (rightHalfRemoved && !leftHalfRemoved) {
      return PageId::LEFT_PAGE;
    } else {
      return PageId::SINGLE_PAGE;
    }
  } else {
    return sub_pages_in_order[logical_page];
  }
}
