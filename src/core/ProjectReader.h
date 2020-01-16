// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PROJECTREADER_H_
#define SCANTAILOR_CORE_PROJECTREADER_H_

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
  using FilterPtr = intrusive_ptr<AbstractFilter>;

  explicit ProjectReader(const QDomDocument& doc);

  ~ProjectReader();

  void readFilterSettings(const std::vector<FilterPtr>& filters) const;

  bool success() const { return (m_pages != nullptr); }

  const QString& outputDirectory() const { return m_outDir; }

  const QString& getVersion() const { return m_version; }

  const intrusive_ptr<ProjectPages>& pages() const { return m_pages; }

  const SelectedPage& selectedPage() const { return m_selectedPage; }

  const intrusive_ptr<FileNameDisambiguator>& namingDisambiguator() const { return m_disambiguator; }

  ImageId imageId(int numericId) const;

  PageId pageId(int numericId) const;

 private:
  struct FileRecord {
    QString filePath;
    bool compatMultiPage;  // Backwards compatibility.

    FileRecord() : compatMultiPage(false) {}

    FileRecord(const QString& filePath, bool compatMultiPage) : filePath(filePath), compatMultiPage(compatMultiPage) {}
  };

  using DirMap = std::unordered_map<int, QString>;
  using FileMap = std::unordered_map<int, FileRecord>;
  using ImageMap = std::unordered_map<int, ImageInfo>;
  using PageMap = std::unordered_map<int, PageId>;

  void processDirectories(const QDomElement& dirsEl);

  void processFiles(const QDomElement& filesEl);

  void processImages(const QDomElement& imagesEl, Qt::LayoutDirection layoutDirection);

  ImageMetadata processImageMetadata(const QDomElement& imageEl);

  void processPages(const QDomElement& pagesEl);

  QString getDirPath(int id) const;

  FileRecord getFileRecord(int id) const;

  QString expandFilePath(const QString& pathShorthand) const;

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


#endif  // ifndef SCANTAILOR_CORE_PROJECTREADER_H_
