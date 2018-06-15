/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "ProjectReader.h"
#include <QDir>
#include <boost/bind.hpp>
#include "AbstractFilter.h"
#include "FileNameDisambiguator.h"
#include "ProjectPages.h"
#include "XmlUnmarshaller.h"
#include "version.h"

ProjectReader::ProjectReader(const QDomDocument& doc) : m_doc(doc), m_ptrDisambiguator(new FileNameDisambiguator) {
  QDomElement project_el(m_doc.documentElement());

  m_version = project_el.attribute("version");
  if (m_version.isNull() || (m_version.toInt() != PROJECT_VERSION)) {
    return;
  }

  m_outDir = project_el.attribute("outputDirectory");

  Qt::LayoutDirection layout_direction = Qt::LeftToRight;
  if (project_el.attribute("layoutDirection") == "RTL") {
    layout_direction = Qt::RightToLeft;
  }

  const QDomElement dirs_el(project_el.namedItem("directories").toElement());
  if (dirs_el.isNull()) {
    return;
  }
  processDirectories(dirs_el);

  const QDomElement files_el(project_el.namedItem("files").toElement());
  if (files_el.isNull()) {
    return;
  }
  processFiles(files_el);

  const QDomElement images_el(project_el.namedItem("images").toElement());
  if (images_el.isNull()) {
    return;
  }
  processImages(images_el, layout_direction);

  const QDomElement pages_el(project_el.namedItem("pages").toElement());
  if (pages_el.isNull()) {
    return;
  }
  processPages(pages_el);
  // Load naming disambiguator.  This needs to be done after processing pages.
  const QDomElement disambig_el(project_el.namedItem("file-name-disambiguation").toElement());
  m_ptrDisambiguator
      = make_intrusive<FileNameDisambiguator>(disambig_el, boost::bind(&ProjectReader::expandFilePath, this, _1));
}

ProjectReader::~ProjectReader() = default;

void ProjectReader::readFilterSettings(const std::vector<FilterPtr>& filters) const {
  QDomElement project_el(m_doc.documentElement());
  QDomElement filters_el(project_el.namedItem("filters").toElement());

  auto it(filters.begin());
  const auto end(filters.end());
  for (; it != end; ++it) {
    (*it)->loadSettings(*this, filters_el);
  }
}

void ProjectReader::processDirectories(const QDomElement& dirs_el) {
  const QString dir_tag_name("directory");

  QDomNode node(dirs_el.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != dir_tag_name) {
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

void ProjectReader::processFiles(const QDomElement& files_el) {
  const QString file_tag_name("file");

  QDomNode node(files_el.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != file_tag_name) {
      continue;
    }
    QDomElement el(node.toElement());

    bool ok = true;
    const int id = el.attribute("id").toInt(&ok);
    if (!ok) {
      continue;
    }
    const int dir_id = el.attribute("dirId").toInt(&ok);
    if (!ok) {
      continue;
    }

    const QString name(el.attribute("name"));
    if (name.isEmpty()) {
      continue;
    }

    const QString dir_path(getDirPath(dir_id));
    if (dir_path.isEmpty()) {
      continue;
    }

    // Backwards compatibility.
    const bool compat_multi_page = (el.attribute("multiPage") == "1");

    const QString file_path(QDir(dir_path).filePath(name));
    const FileRecord rec(file_path, compat_multi_page);
    m_fileMap.insert(FileMap::value_type(id, rec));
  }
}  // ProjectReader::processFiles

void ProjectReader::processImages(const QDomElement& images_el, const Qt::LayoutDirection layout_direction) {
  const QString image_tag_name("image");

  std::vector<ImageInfo> images;

  QDomNode node(images_el.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != image_tag_name) {
      continue;
    }
    QDomElement el(node.toElement());

    bool ok = true;
    const int id = el.attribute("id").toInt(&ok);
    if (!ok) {
      continue;
    }
    const int sub_pages = el.attribute("subPages").toInt(&ok);
    if (!ok) {
      continue;
    }
    const int file_id = el.attribute("fileId").toInt(&ok);
    if (!ok) {
      continue;
    }
    const int file_image = el.attribute("fileImage").toInt(&ok);
    if (!ok) {
      continue;
    }

    const QString removed(el.attribute("removed"));
    const bool left_half_removed = (removed == "L");
    const bool right_half_removed = (removed == "R");

    const FileRecord file_record(getFileRecord(file_id));
    if (file_record.filePath.isEmpty()) {
      continue;
    }
    const ImageId image_id(file_record.filePath, file_image + int(file_record.compatMultiPage));
    const ImageMetadata metadata(processImageMetadata(el));
    const ImageInfo image_info(image_id, metadata, sub_pages, left_half_removed, right_half_removed);

    images.push_back(image_info);
    m_imageMap.insert(ImageMap::value_type(id, image_info));
  }

  if (!images.empty()) {
    m_ptrPages = make_intrusive<ProjectPages>(images, layout_direction);
  }
}  // ProjectReader::processImages

ImageMetadata ProjectReader::processImageMetadata(const QDomElement& image_el) {
  QSize size;
  Dpi dpi;

  const QDomElement size_el(image_el.namedItem("size").toElement());
  if (!size_el.isNull()) {
    size = XmlUnmarshaller::size(size_el);
  }
  const QDomElement dpi_el(image_el.namedItem("dpi").toElement());
  if (!dpi_el.isNull()) {
    dpi = XmlUnmarshaller::dpi(dpi_el);
  }

  return ImageMetadata(size, dpi);
}

void ProjectReader::processPages(const QDomElement& pages_el) {
  const QString page_tag_name("page");

  QDomNode node(pages_el.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != page_tag_name) {
      continue;
    }
    QDomElement el(node.toElement());

    bool ok = true;

    const int id = el.attribute("id").toInt(&ok);
    if (!ok) {
      continue;
    }

    const int image_id = el.attribute("imageId").toInt(&ok);
    if (!ok) {
      continue;
    }

    const PageId::SubPage sub_page = PageId::subPageFromString(el.attribute("subPage"), &ok);
    if (!ok) {
      continue;
    }

    const ImageInfo image(getImageInfo(image_id));
    if (image.id().filePath().isEmpty()) {
      continue;
    }

    const PageId page_id(image.id(), sub_page);
    m_pageMap.insert(PageMap::value_type(id, page_id));

    if (el.attribute("selected") == "selected") {
      m_selectedPage.set(page_id, PAGE_VIEW);
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

QString ProjectReader::expandFilePath(const QString& path_shorthand) const {
  bool ok = false;
  const int file_id = path_shorthand.toInt(&ok);
  if (!ok) {
    return QString();
  }

  return getFileRecord(file_id).filePath;
}

ImageInfo ProjectReader::getImageInfo(int id) const {
  auto it(m_imageMap.find(id));
  if (it != m_imageMap.end()) {
    return it->second;
  }

  return ImageInfo();
}

ImageId ProjectReader::imageId(const int numeric_id) const {
  auto it(m_imageMap.find(numeric_id));
  if (it != m_imageMap.end()) {
    return it->second.id();
  }

  return ImageId();
}

PageId ProjectReader::pageId(int numeric_id) const {
  auto it(m_pageMap.find(numeric_id));
  if (it != m_pageMap.end()) {
    return it->second;
  }

  return PageId();
}
