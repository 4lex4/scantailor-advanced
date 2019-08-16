// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PROJECTWRITER_H_
#define PROJECTWRITER_H_

#include <foundation/Hashes.h>
#include <QString>
#include <Qt>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include <unordered_map>
#include <vector>
#include "ImageId.h"
#include "OutputFileNameGenerator.h"
#include "PageId.h"
#include "PageSequence.h"
#include "SelectedPage.h"
#include "VirtualFunction.h"
#include "intrusive_ptr.h"

class AbstractFilter;
class ProjectPages;
class PageInfo;
class QDomDocument;
class QDomElement;

class ProjectWriter {
  DECLARE_NON_COPYABLE(ProjectWriter)

 public:
  typedef intrusive_ptr<AbstractFilter> FilterPtr;

  ProjectWriter(const intrusive_ptr<ProjectPages>& page_sequence,
                const SelectedPage& selected_page,
                const OutputFileNameGenerator& out_file_name_gen);

  ~ProjectWriter();

  bool write(const QString& file_path, const std::vector<FilterPtr>& filters) const;

  /**
   * \p out will be called like this: out(ImageId, numeric_image_id)
   */
  template <typename OutFunc>
  void enumImages(OutFunc out) const;

  /**
   * \p out will be called like this: out(LogicalPageId, numeric_page_id)
   */
  template <typename OutFunc>
  void enumPages(OutFunc out) const;

 private:
  struct Directory {
    QString path;
    int numericId;

    Directory(const QString& path, int numeric_id) : path(path), numericId(numeric_id) {}
  };

  struct File {
    QString path;
    int numericId;

    File(const QString& path, int numeric_id) : path(path), numericId(numeric_id) {}
  };

  struct Image {
    ImageId id;
    int numericId;
    int numSubPages;
    bool leftHalfRemoved;
    bool rightHalfRemoved;

    Image(const PageInfo& page_info, int numeric_id);
  };

  struct Page {
    PageId id;
    int numericId;

    Page(const PageId& id, int numeric_id) : id(id), numericId(numeric_id) {}
  };

  class Sequenced;

  typedef std::unordered_map<ImageId, ImageMetadata> MetadataByImage;

  typedef boost::multi_index::multi_index_container<
      Directory,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_unique<boost::multi_index::member<Directory, QString, &Directory::path>,
                                            hashes::hash<QString>>,
          boost::multi_index::sequenced<boost::multi_index::tag<Sequenced>>>>
      Directories;

  typedef boost::multi_index::multi_index_container<
      File,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_unique<boost::multi_index::member<File, QString, &File::path>,
                                            hashes::hash<QString>>,
          boost::multi_index::sequenced<boost::multi_index::tag<Sequenced>>>>
      Files;

  typedef boost::multi_index::multi_index_container<
      Image,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_unique<boost::multi_index::member<Image, ImageId, &Image::id>, std::hash<ImageId>>,
          boost::multi_index::sequenced<boost::multi_index::tag<Sequenced>>>>
      Images;

  typedef boost::multi_index::multi_index_container<
      Page,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_unique<boost::multi_index::member<Page, PageId, &Page::id>, std::hash<PageId>>,
          boost::multi_index::sequenced<boost::multi_index::tag<Sequenced>>>>
      Pages;

  QDomElement processDirectories(QDomDocument& doc) const;

  QDomElement processFiles(QDomDocument& doc) const;

  QDomElement processImages(QDomDocument& doc) const;

  QDomElement processPages(QDomDocument& doc) const;

  void writeImageMetadata(QDomDocument& doc, QDomElement& image_el, const ImageId& image_id) const;

  int dirId(const QString& dir_path) const;

  int fileId(const QString& file_path) const;

  QString packFilePath(const QString& file_path) const;

  int imageId(const ImageId& image_id) const;

  int pageId(const PageId& page_id) const;

  void enumImagesImpl(const VirtualFunction<void, const ImageId&, int>& out) const;

  void enumPagesImpl(const VirtualFunction<void, const PageId&, int>& out) const;

  PageSequence m_pageSequence;
  OutputFileNameGenerator m_outFileNameGen;
  SelectedPage m_selectedPage;
  Directories m_dirs;
  Files m_files;
  Images m_images;
  Pages m_pages;
  MetadataByImage m_metadataByImage;
  Qt::LayoutDirection m_layoutDirection;
};


template <typename Callable>
void ProjectWriter::enumImages(Callable out) const {
  enumImagesImpl(ProxyFunction<Callable, void, const ImageId&, int>(out));
}

template <typename Callable>
void ProjectWriter::enumPages(Callable out) const {
  enumPagesImpl(ProxyFunction<Callable, void, const PageId&, int>(out));
}

#endif  // ifndef PROJECTWRITER_H_
