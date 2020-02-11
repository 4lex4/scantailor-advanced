// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PROJECTPAGES_H_
#define SCANTAILOR_CORE_PROJECTPAGES_H_

#include <QMutex>
#include <QObject>
#include <QString>
#include <Qt>
#include <cstddef>
#include <set>
#include <vector>

#include "BeforeOrAfter.h"
#include "ImageId.h"
#include "ImageMetadata.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "PageInfo.h"
#include "PageView.h"
#include "VirtualFunction.h"

class ImageFileInfo;
class ImageInfo;
class OrthogonalRotation;
class PageSequence;
class RelinkablePath;
class AbstractRelinker;
class QDomElement;

class ProjectPages : public QObject {
  Q_OBJECT
  DECLARE_NON_COPYABLE(ProjectPages)

 public:
  enum Pages { ONE_PAGE, TWO_PAGES, AUTO_PAGES };

  enum LayoutType { ONE_PAGE_LAYOUT, TWO_PAGE_LAYOUT };

  explicit ProjectPages(Qt::LayoutDirection layoutDirection = Qt::LeftToRight);

  ProjectPages(const std::vector<ImageInfo>& images, Qt::LayoutDirection layoutDirection);

  ProjectPages(const std::vector<ImageFileInfo>& files, Pages pages, Qt::LayoutDirection layoutDirection);

  ~ProjectPages() override;

  Qt::LayoutDirection layoutDirection() const;

  PageSequence toPageSequence(PageView view) const;

  void listRelinkablePaths(const VirtualFunction<void, const RelinkablePath&>& sink) const;

  /**
   * \note It's up to the caller to make sure different paths aren't collapsed into one.
   *       Having the same page more the once in a project is not supported by Scan Tailor.
   */
  void performRelinking(const AbstractRelinker& relinker);

  void setLayoutTypeFor(const ImageId& imageId, LayoutType layout);

  void setLayoutTypeForAllPages(LayoutType layout);

  void autoSetLayoutTypeFor(const ImageId& imageId, OrthogonalRotation rotation);

  void updateImageMetadata(const ImageId& imageId, const ImageMetadata& metadata);

  static int adviseNumberOfLogicalPages(const ImageMetadata& metadata, OrthogonalRotation rotation);

  int numImages() const;

  /**
   * \brief Insert an image before or after the existing one.
   *
   * The caller has to make sure he is not inserting an image that already
   * exists in this ProjectPages.  Requesting to insert a new image
   * BEFORE the null one is legal and means inserting it at the end.
   *
   * \param newImage The image to insert.
   * \param beforeOrAfter Whether to insert before or after another image.
   * \param existing The image we are inserting before or after.
   * \param view This one only affects what is returned.
   * \return One or two (or zero, if existing image wasn't found) logical
   *         page descriptors.  If two are returned, they will be returned
   *         in the order dependent on the layout direction specified
   *         at construction time.
   */
  std::vector<PageInfo> insertImage(const ImageInfo& newImage,
                                    BeforeOrAfter beforeOrAfter,
                                    const ImageId& existing,
                                    PageView view);

  void removePages(const std::set<PageId>& pages);

  /**
   * \brief Unremoves half-a-page, if the other half is still present.
   *
   * \param pageId Left or right sub-page to restore.
   * \return A PageInfo corresponding to the page restored or
   *         a null PageInfo if restoring failed.
   */
  PageInfo unremovePage(const PageId& pageId);

  /**
   * \brief Check if all DPIs are OK, in terms of ImageMetadata::isDpiOK()
   *
   * \return true if all DPIs are OK, false if not.
   */
  bool validateDpis() const;

  std::vector<ImageFileInfo> toImageFileInfo() const;

  void updateMetadataFrom(const std::vector<ImageFileInfo>& files);

 signals:

  void modified();

 private:
  struct ImageDesc {
    ImageId id;
    ImageMetadata metadata;
    int numLogicalPages;    // 1 or 2
    bool leftHalfRemoved;   // Both can't be true, and if one is true,
    bool rightHalfRemoved;  // then numLogicalPages is 1.

    explicit ImageDesc(const ImageInfo& imageInfo);

    ImageDesc(const ImageId& id, const ImageMetadata& metadata, Pages pages);

    PageId::SubPage logicalPageToSubPage(int logicalPage, const PageId::SubPage* subPagesInOrder) const;
  };

  void initSubPagesInOrder(Qt::LayoutDirection layoutDirection);

  void setLayoutTypeForImpl(const ImageId& imageId, LayoutType layout, bool* modified);

  void setLayoutTypeForAllPagesImpl(LayoutType layout, bool* modified);

  void autoSetLayoutTypeForImpl(const ImageId& imageId, OrthogonalRotation rotation, bool* modified);

  void updateImageMetadataImpl(const ImageId& imageId, const ImageMetadata& metadata, bool* modified);

  std::vector<PageInfo> insertImageImpl(const ImageInfo& newImage,
                                        BeforeOrAfter beforeOrAfter,
                                        const ImageId& existing,
                                        PageView view,
                                        bool& modified);

  void removePagesImpl(const std::set<PageId>& pages, bool& modified);

  PageInfo unremovePageImpl(const PageId& pageId, bool& modified);

  mutable QMutex m_mutex;
  std::vector<ImageDesc> m_images;
  PageId::SubPage m_subPagesInOrder[2];
};


#endif  // ifndef SCANTAILOR_CORE_PROJECTPAGES_H_
