// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ProjectReader.h"

#include <QDir>
#include <boost/bind.hpp>

#include "AbstractFilter.h"
#include "FileNameDisambiguator.h"
#include "ProjectPages.h"
#include "XmlUnmarshaller.h"
#include "version.h"

ProjectReader::ProjectReader(const QDomDocument& doc)
    : m_doc(doc), m_disambiguator(std::make_shared<FileNameDisambiguator>()) {
  QDomElement projectEl(m_doc.documentElement());

  m_version = projectEl.attribute("version");
  if (m_version.isNull() || (m_version.toInt() != PROJECT_VERSION)) {
    return;
  }

  m_outDir = projectEl.attribute("outputDirectory");

  Qt::LayoutDirection layoutDirection = Qt::LeftToRight;
  if (projectEl.attribute("layoutDirection") == "RTL") {
    layoutDirection = Qt::RightToLeft;
  }

  const QDomElement dirsEl(projectEl.namedItem("directories").toElement());
  if (dirsEl.isNull()) {
    return;
  }
  processDirectories(dirsEl);

  const QDomElement filesEl(projectEl.namedItem("files").toElement());
  if (filesEl.isNull()) {
    return;
  }
  processFiles(filesEl);

  const QDomElement imagesEl(projectEl.namedItem("images").toElement());
  if (imagesEl.isNull()) {
    return;
  }
  processImages(imagesEl, layoutDirection);

  const QDomElement pagesEl(projectEl.namedItem("pages").toElement());
  if (pagesEl.isNull()) {
    return;
  }
  processPages(pagesEl);
  // Load naming disambiguator.  This needs to be done after processing pages.
  const QDomElement disambigEl(projectEl.namedItem("file-name-disambiguation").toElement());
  m_disambiguator
      = std::make_shared<FileNameDisambiguator>(disambigEl, boost::bind(&ProjectReader::expandFilePath, this, _1));
}

ProjectReader::~ProjectReader() = default;

void ProjectReader::readFilterSettings(const std::vector<FilterPtr>& filters) const {
  QDomElement projectEl(m_doc.documentElement());
  QDomElement filtersEl(projectEl.namedItem("filters").toElement());

  auto it(filters.begin());
  const auto end(filters.end());
  for (; it != end; ++it) {
    (*it)->loadSettings(*this, filtersEl);
  }
}

void ProjectReader::processDirectories(const QDomElement& dirsEl) {
  const QString dirTagName("directory");

  QDomNode node(dirsEl.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != dirTagName) {
      continue;
    }
    QDomElement el(node.toElement());

    bool ok = true;
    const int id = el.attribute("id").toInt(&ok);
    if (!ok) {
      continue;
    }

    const QString path(el.attribute("path"));
    if (path.isEmpty()) {
      continue;
    }

    m_dirMap.insert(DirMap::value_type(id, path));
  }
}

void ProjectReader::processFiles(const QDomElement& filesEl) {
  const QString fileTagName("file");

  QDomNode node(filesEl.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != fileTagName) {
      continue;
    }
    QDomElement el(node.toElement());

    bool ok = true;
    const int id = el.attribute("id").toInt(&ok);
    if (!ok) {
      continue;
    }
    const int dirId = el.attribute("dirId").toInt(&ok);
    if (!ok) {
      continue;
    }

    const QString name(el.attribute("name"));
    if (name.isEmpty()) {
      continue;
    }

    const QString dirPath(getDirPath(dirId));
    if (dirPath.isEmpty()) {
      continue;
    }

    // Backwards compatibility.
    const bool compatMultiPage = (el.attribute("multiPage") == "1");

    const QString filePath(QDir(dirPath).filePath(name));
    const FileRecord rec(filePath, compatMultiPage);
    m_fileMap.insert(FileMap::value_type(id, rec));
  }
}  // ProjectReader::processFiles

void ProjectReader::processImages(const QDomElement& imagesEl, const Qt::LayoutDirection layoutDirection) {
  const QString imageTagName("image");

  std::vector<ImageInfo> images;

  QDomNode node(imagesEl.firstChild());
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
    const int subPages = el.attribute("subPages").toInt(&ok);
    if (!ok) {
      continue;
    }
    const int fileId = el.attribute("fileId").toInt(&ok);
    if (!ok) {
      continue;
    }
    const int fileImage = el.attribute("fileImage").toInt(&ok);
    if (!ok) {
      continue;
    }

    const QString removed(el.attribute("removed"));
    const bool leftHalfRemoved = (removed == "L");
    const bool rightHalfRemoved = (removed == "R");

    const FileRecord fileRecord(getFileRecord(fileId));
    if (fileRecord.filePath.isEmpty()) {
      continue;
    }
    const ImageId imageId(fileRecord.filePath, fileImage + int(fileRecord.compatMultiPage));
    const ImageMetadata metadata(processImageMetadata(el));
    const ImageInfo imageInfo(imageId, metadata, subPages, leftHalfRemoved, rightHalfRemoved);

    images.push_back(imageInfo);
    m_imageMap.insert(ImageMap::value_type(id, imageInfo));
  }

  if (!images.empty()) {
    m_pages = std::make_shared<ProjectPages>(images, layoutDirection);
  }
}  // ProjectReader::processImages

ImageMetadata ProjectReader::processImageMetadata(const QDomElement& imageEl) {
  QSize size;
  Dpi dpi;

  const QDomElement sizeEl(imageEl.namedItem("size").toElement());
  if (!sizeEl.isNull()) {
    size = XmlUnmarshaller::size(sizeEl);
  }
  const QDomElement dpiEl(imageEl.namedItem("dpi").toElement());
  if (!dpiEl.isNull()) {
    dpi = Dpi(dpiEl);
  }
  return ImageMetadata(size, dpi);
}

void ProjectReader::processPages(const QDomElement& pagesEl) {
  const QString pageTagName("page");

  QDomNode node(pagesEl.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != pageTagName) {
      continue;
    }
    QDomElement el(node.toElement());

    bool ok = true;

    const int id = el.attribute("id").toInt(&ok);
    if (!ok) {
      continue;
    }

    const int imageId = el.attribute("imageId").toInt(&ok);
    if (!ok) {
      continue;
    }

    const PageId::SubPage subPage = PageId::subPageFromString(el.attribute("subPage"), &ok);
    if (!ok) {
      continue;
    }

    const ImageInfo image(getImageInfo(imageId));
    if (image.id().filePath().isEmpty()) {
      continue;
    }

    const PageId pageId(image.id(), subPage);
    m_pageMap.insert(PageMap::value_type(id, pageId));

    if (el.attribute("selected") == "selected") {
      m_selectedPage.set(pageId, PAGE_VIEW);
    }
  }
}  // ProjectReader::processPages

QString ProjectReader::getDirPath(const int id) const {
  const auto it(m_dirMap.find(id));
  if (it != m_dirMap.end()) {
    return it->second;
  }
  return QString();
}

ProjectReader::FileRecord ProjectReader::getFileRecord(int id) const {
  const auto it(m_fileMap.find(id));
  if (it != m_fileMap.end()) {
    return it->second;
  }
  return FileRecord();
}

QString ProjectReader::expandFilePath(const QString& pathShorthand) const {
  bool ok = false;
  const int fileId = pathShorthand.toInt(&ok);
  if (!ok) {
    return QString();
  }
  return getFileRecord(fileId).filePath;
}

ImageInfo ProjectReader::getImageInfo(int id) const {
  auto it(m_imageMap.find(id));
  if (it != m_imageMap.end()) {
    return it->second;
  }
  return ImageInfo();
}

ImageId ProjectReader::imageId(const int numericId) const {
  auto it(m_imageMap.find(numericId));
  if (it != m_imageMap.end()) {
    return it->second.id();
  }
  return ImageId();
}

PageId ProjectReader::pageId(int numericId) const {
  auto it(m_pageMap.find(numericId));
  if (it != m_pageMap.end()) {
    return it->second;
  }
  return PageId();
}
