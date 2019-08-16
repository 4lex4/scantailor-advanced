// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PROJECTREADER_H_
#define PROJECTREADER_H_

#include <QDomDocument>
#include <QString>
#include <Qt>
#include <unordered_map>
#include <vector>
#include "ImageId.h"
#include "ImageInfo.h"
#include "ImageMetadata.h"
#include "PageId.h"
#include "SelectedPage.h"
#include "intrusive_ptr.h"

class QDomElement;
class ProjectPages;
class FileNameDisambiguator;
class AbstractFilter;

class ProjectReader {
 public:
  typedef intrusive_ptr<AbstractFilter> FilterPtr;

  explicit ProjectReader(const QDomDocument& doc);

  ~ProjectReader();

  void readFilterSettings(const std::vector<FilterPtr>& filters) const;

  bool success() const { return (m_pages != nullptr); }

  const QString& outputDirectory() const { return m_outDir; }

  const QString& getVersion() const { return m_version; }

  const intrusive_ptr<ProjectPages>& pages() const { return m_pages; }

  const SelectedPage& selectedPage() const { return m_selectedPage; }

  const intrusive_ptr<FileNameDisambiguator>& namingDisambiguator() const { return m_disambiguator; }

  ImageId imageId(int numeric_id) const;

  PageId pageId(int numeric_id) const;

 private:
  struct FileRecord {
    QString filePath;
    bool compatMultiPage;  // Backwards compatibility.

    FileRecord() : compatMultiPage(false) {}

    FileRecord(const QString& file_path, bool compat_multi_page)
        : filePath(file_path), compatMultiPage(compat_multi_page) {}
  };

  typedef std::unordered_map<int, QString> DirMap;
  typedef std::unordered_map<int, FileRecord> FileMap;
  typedef std::unordered_map<int, ImageInfo> ImageMap;
  typedef std::unordered_map<int, PageId> PageMap;

  void processDirectories(const QDomElement& dirs_el);

  void processFiles(const QDomElement& files_el);

  void processImages(const QDomElement& images_el, Qt::LayoutDirection layout_direction);

  ImageMetadata processImageMetadata(const QDomElement& image_el);

  void processPages(const QDomElement& pages_el);

  QString getDirPath(int id) const;

  FileRecord getFileRecord(int id) const;

  QString expandFilePath(const QString& path_shorthand) const;

  ImageInfo getImageInfo(int id) const;

  QDomDocument m_doc;
  QString m_outDir;
  QString m_version;
  DirMap m_dirMap;
  FileMap m_fileMap;
  ImageMap m_imageMap;
  PageMap m_pageMap;
  SelectedPage m_selectedPage;
  intrusive_ptr<ProjectPages> m_pages;
  intrusive_ptr<FileNameDisambiguator> m_disambiguator;
};


#endif  // ifndef PROJECTREADER_H_
