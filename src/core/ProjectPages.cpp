// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ProjectPages.h"

#include <QDebug>
#include <boost/foreach.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <cassert>
#include <unordered_map>

#include "AbstractRelinker.h"
#include "ImageFileInfo.h"
#include "ImageInfo.h"
#include "OrthogonalRotation.h"
#include "PageSequence.h"
#include "RelinkablePath.h"

ProjectPages::ProjectPages(const Qt::LayoutDirection layoutDirection) {
  initSubPagesInOrder(layoutDirection);
}

ProjectPages::ProjectPages(const std::vector<ImageInfo>& info, const Qt::LayoutDirection layoutDirection) {
  initSubPagesInOrder(layoutDirection);

  for (const ImageInfo& image : info) {
    ImageDesc imageDesc(image);
    // Enforce some rules.
    if (imageDesc.numLogicalPages == 2) {
      imageDesc.leftHalfRemoved = false;
      imageDesc.rightHalfRemoved = false;
    } else if (imageDesc.numLogicalPages != 1) {
      continue;
    } else if (imageDesc.leftHalfRemoved && imageDesc.rightHalfRemoved) {
      imageDesc.leftHalfRemoved = false;
      imageDesc.rightHalfRemoved = false;
    }

    m_images.push_back(imageDesc);
  }
}

ProjectPages::ProjectPages(const std::vector<ImageFileInfo>& files,
                           const Pages pages,
                           const Qt::LayoutDirection layoutDirection) {
  initSubPagesInOrder(layoutDirection);

  for (const ImageFileInfo& file : files) {
    const QString& filePath = file.fileInfo().absoluteFilePath();
    const std::vector<ImageMetadata>& images = file.imageInfo();
    const auto numImages = static_cast<int>(images.size());
    const int multiPageBase = numImages > 1 ? 1 : 0;
    for (int i = 0; i < numImages; ++i) {
      const ImageMetadata& metadata = images[i];
      const ImageId id(filePath, multiPageBase + i);
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

void ProjectPages::initSubPagesInOrder(const Qt::LayoutDirection layoutDirection) {
  if (layoutDirection == Qt::LeftToRight) {
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

    const auto numImages = static_cast<int>(m_images.size());
    for (int i = 0; i < numImages; ++i) {
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

    const auto numImages = static_cast<int>(m_images.size());
    for (int i = 0; i < numImages; ++i) {
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
    const RelinkablePath oldPath(image.id.filePath(), RelinkablePath::File);
    const QString newPath(relinker.substitutionPathFor(oldPath));
    image.id.setFilePath(newPath);
  }
}

void ProjectPages::setLayoutTypeFor(const ImageId& imageId, const LayoutType layout) {
  bool wasModified = false;

  {
    QMutexLocker locker(&m_mutex);
    setLayoutTypeForImpl(imageId, layout, &wasModified);
  }

  if (wasModified) {
    emit modified();
  }
}

void ProjectPages::setLayoutTypeForAllPages(const LayoutType layout) {
  bool wasModified = false;

  {
    QMutexLocker locker(&m_mutex);
    setLayoutTypeForAllPagesImpl(layout, &wasModified);
  }

  if (wasModified) {
    emit modified();
  }
}

void ProjectPages::autoSetLayoutTypeFor(const ImageId& imageId, const OrthogonalRotation rotation) {
  bool wasModified = false;

  {
    QMutexLocker locker(&m_mutex);
    autoSetLayoutTypeForImpl(imageId, rotation, &wasModified);
  }

  if (wasModified) {
    emit modified();
  }
}

void ProjectPages::updateImageMetadata(const ImageId& imageId, const ImageMetadata& metadata) {
  bool wasModified = false;

  {
    QMutexLocker locker(&m_mutex);
    updateImageMetadataImpl(imageId, metadata, &wasModified);
  }

  if (wasModified) {
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

std::vector<PageInfo> ProjectPages::insertImage(const ImageInfo& newImage,
                                                BeforeOrAfter beforeOrAfter,
                                                const ImageId& existing,
                                                const PageView view) {
  bool wasModified = false;

  {
    QMutexLocker locker(&m_mutex);
    return insertImageImpl(newImage, beforeOrAfter, existing, view, wasModified);
  }

  if (wasModified) {
    emit modified();
  }
}

void ProjectPages::removePages(const std::set<PageId>& pages) {
  bool wasModified = false;

  {
    QMutexLocker locker(&m_mutex);
    removePagesImpl(pages, wasModified);
  }

  if (wasModified) {
    emit modified();
  }
}

PageInfo ProjectPages::unremovePage(const PageId& pageId) {
  bool wasModified = false;

  PageInfo pageInfo;

  {
    QMutexLocker locker(&m_mutex);
    pageInfo = unremovePageImpl(pageId, wasModified);
  }

  if (wasModified) {
    emit modified();
  }
  return pageInfo;
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
  using MetadataMap = std::unordered_map<ImageId, ImageMetadata>;
  MetadataMap metadataMap;

  for (const ImageFileInfo& file : files) {
    const QString filePath(file.fileInfo().absoluteFilePath());
    int page = 0;
    for (const ImageMetadata& metadata : file.imageInfo()) {
      metadataMap[ImageId(filePath, page)] = metadata;
      ++page;
    }
  }

  QMutexLocker locker(&m_mutex);

  for (ImageDesc& image : m_images) {
    const MetadataMap::const_iterator it(metadataMap.find(image.id));
    if (it != metadataMap.end()) {
      image.metadata = it->second;
    }
  }
}

void ProjectPages::setLayoutTypeForImpl(const ImageId& imageId, const LayoutType layout, bool* modified) {
  const int numPages = (layout == TWO_PAGE_LAYOUT ? 2 : 1);
  const auto numImages = static_cast<int>(m_images.size());
  for (int i = 0; i < numImages; ++i) {
    ImageDesc& image = m_images[i];
    if (image.id == imageId) {
      int adjustedNumPages = numPages;
      if ((numPages == 2) && (image.leftHalfRemoved != image.rightHalfRemoved)) {
        // Both can't be removed, but we handle that case anyway
        // by treating it like none are removed.
        --adjustedNumPages;
      }

      const int delta = adjustedNumPages - image.numLogicalPages;
      if (delta == 0) {
        break;
      }

      image.numLogicalPages = adjustedNumPages;

      *modified = true;
      break;
    }
  }
}

void ProjectPages::setLayoutTypeForAllPagesImpl(const LayoutType layout, bool* modified) {
  const int numPages = (layout == TWO_PAGE_LAYOUT ? 2 : 1);
  const auto numImages = static_cast<int>(m_images.size());
  for (int i = 0; i < numImages; ++i) {
    ImageDesc& image = m_images[i];

    int adjustedNumPages = numPages;
    if ((numPages == 2) && (image.leftHalfRemoved != image.rightHalfRemoved)) {
      // Both can't be removed, but we handle that case anyway
      // by treating it like none are removed.
      --adjustedNumPages;
    }

    const int delta = adjustedNumPages - image.numLogicalPages;
    if (delta == 0) {
      continue;
    }

    image.numLogicalPages = adjustedNumPages;
    *modified = true;
  }
}

void ProjectPages::autoSetLayoutTypeForImpl(const ImageId& imageId, const OrthogonalRotation rotation, bool* modified) {
  const auto numImages = static_cast<int>(m_images.size());
  for (int i = 0; i < numImages; ++i) {
    ImageDesc& image = m_images[i];
    if (image.id == imageId) {
      int numPages = adviseNumberOfLogicalPages(image.metadata, rotation);
      if ((numPages == 2) && (image.leftHalfRemoved != image.rightHalfRemoved)) {
        // Both can't be removed, but we handle that case anyway
        // by treating it like none are removed.
        --numPages;
      }

      const int delta = numPages - image.numLogicalPages;
      if (delta == 0) {
        break;
      }

      image.numLogicalPages = numPages;

      *modified = true;
      break;
    }
  }
}

void ProjectPages::updateImageMetadataImpl(const ImageId& imageId, const ImageMetadata& metadata, bool* modified) {
  const auto numImages = static_cast<int>(m_images.size());
  for (int i = 0; i < numImages; ++i) {
    ImageDesc& image = m_images[i];
    if (image.id == imageId) {
      if (image.metadata != metadata) {
        image.metadata = metadata;
        *modified = true;
      }
      break;
    }
  }
}

std::vector<PageInfo> ProjectPages::insertImageImpl(const ImageInfo& newImage,
                                                    BeforeOrAfter beforeOrAfter,
                                                    const ImageId& existing,
                                                    const PageView view,
                                                    bool& modified) {
  std::vector<PageInfo> logicalPages;

  auto it(m_images.begin());
  const auto end(m_images.end());
  for (; it != end && it->id != existing; ++it) {
    // Continue until we find the existing image.
  }
  if (it == end) {
    // Existing image not found.
    if (!((beforeOrAfter == BEFORE) && existing.isNull())) {
      return logicalPages;
    }  // Otherwise we can still handle that case.
  }
  if (beforeOrAfter == AFTER) {
    ++it;
  }

  ImageDesc imageDesc(newImage);

  // Enforce some rules.
  if (imageDesc.numLogicalPages == 2) {
    imageDesc.leftHalfRemoved = false;
    imageDesc.rightHalfRemoved = false;
  } else if (imageDesc.numLogicalPages != 1) {
    return logicalPages;
  } else if (imageDesc.leftHalfRemoved && imageDesc.rightHalfRemoved) {
    imageDesc.leftHalfRemoved = false;
    imageDesc.rightHalfRemoved = false;
  }

  m_images.insert(it, imageDesc);

  PageInfo pageInfoTempl(PageId(newImage.id(), PageId::SINGLE_PAGE), imageDesc.metadata, imageDesc.numLogicalPages,
                         imageDesc.leftHalfRemoved, imageDesc.rightHalfRemoved);

  if ((view == IMAGE_VIEW)
      || ((imageDesc.numLogicalPages == 1) && (imageDesc.leftHalfRemoved == imageDesc.rightHalfRemoved))) {
    logicalPages.push_back(pageInfoTempl);
  } else {
    if ((imageDesc.numLogicalPages == 2) || ((imageDesc.numLogicalPages == 1) && imageDesc.rightHalfRemoved)) {
      pageInfoTempl.setId(PageId(newImage.id(), m_subPagesInOrder[0]));
      logicalPages.push_back(pageInfoTempl);
    }
    if ((imageDesc.numLogicalPages == 2) || ((imageDesc.numLogicalPages == 1) && imageDesc.leftHalfRemoved)) {
      pageInfoTempl.setId(PageId(newImage.id(), m_subPagesInOrder[1]));
      logicalPages.push_back(pageInfoTempl);
    }
  }
  return logicalPages;
}  // ProjectPages::insertImageImpl

void ProjectPages::removePagesImpl(const std::set<PageId>& toRemove, bool& modified) {
  const auto toRemoveEnd(toRemove.end());

  std::vector<ImageDesc> newImages;
  newImages.reserve(m_images.size());
  int newTotalLogicalPages = 0;

  const auto numOldImages = static_cast<int>(m_images.size());
  for (int i = 0; i < numOldImages; ++i) {
    ImageDesc image(m_images[i]);

    if (toRemove.find(PageId(image.id, PageId::SINGLE_PAGE)) != toRemoveEnd) {
      image.numLogicalPages = 0;
      modified = true;
    } else {
      if (toRemove.find(PageId(image.id, PageId::LEFT_PAGE)) != toRemoveEnd) {
        image.leftHalfRemoved = true;
        --image.numLogicalPages;
        modified = true;
      }
      if (toRemove.find(PageId(image.id, PageId::RIGHT_PAGE)) != toRemoveEnd) {
        image.rightHalfRemoved = true;
        --image.numLogicalPages;
        modified = true;
      }
    }

    if (image.numLogicalPages > 0) {
      newImages.push_back(image);
      newTotalLogicalPages += newImages.back().numLogicalPages;
    }
  }

  newImages.swap(m_images);
}  // ProjectPages::removePagesImpl

PageInfo ProjectPages::unremovePageImpl(const PageId& pageId, bool& modified) {
  if (pageId.subPage() == PageId::SINGLE_PAGE) {
    // These can't be unremoved.
    return PageInfo();
  }

  auto it(m_images.begin());
  const auto end(m_images.end());
  for (; it != end && it->id != pageId.imageId(); ++it) {
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

  if ((pageId.subPage() == PageId::LEFT_PAGE) && image.leftHalfRemoved) {
    image.leftHalfRemoved = false;
  } else if ((pageId.subPage() == PageId::RIGHT_PAGE) && image.rightHalfRemoved) {
    image.rightHalfRemoved = false;
  } else {
    return PageInfo();
  }

  image.numLogicalPages = 2;
  return PageInfo(pageId, image.metadata, image.numLogicalPages, image.leftHalfRemoved, image.rightHalfRemoved);
}  // ProjectPages::unremovePageImpl

/*========================= ProjectPages::ImageDesc ======================*/

ProjectPages::ImageDesc::ImageDesc(const ImageInfo& imageInfo)
    : id(imageInfo.id()),
      metadata(imageInfo.metadata()),
      numLogicalPages(imageInfo.numSubPages()),
      leftHalfRemoved(imageInfo.leftHalfRemoved()),
      rightHalfRemoved(imageInfo.rightHalfRemoved()) {}

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

PageId::SubPage ProjectPages::ImageDesc::logicalPageToSubPage(const int logicalPage,
                                                              const PageId::SubPage* subPagesInOrder) const {
  assert(numLogicalPages >= 1 && numLogicalPages <= 2);
  assert(logicalPage >= 0 && logicalPage < numLogicalPages);

  if (numLogicalPages == 1) {
    if (leftHalfRemoved && !rightHalfRemoved) {
      return PageId::RIGHT_PAGE;
    } else if (rightHalfRemoved && !leftHalfRemoved) {
      return PageId::LEFT_PAGE;
    } else {
      return PageId::SINGLE_PAGE;
    }
  } else {
    return subPagesInOrder[logicalPage];
  }
}
