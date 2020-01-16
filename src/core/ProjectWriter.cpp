// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ProjectWriter.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QtXml>

#include "AbstractFilter.h"
#include "FileNameDisambiguator.h"
#include "ImageId.h"
#include "ImageMetadata.h"
#include "PageId.h"
#include "PageInfo.h"
#include "PageView.h"
#include "ProjectPages.h"
#include "version.h"

#ifndef Q_MOC_RUN

#include <boost/bind.hpp>

#endif

#include <cassert>
#include <cstddef>

ProjectWriter::ProjectWriter(const intrusive_ptr<ProjectPages>& pageSequence,
                             const SelectedPage& selectedPage,
                             const OutputFileNameGenerator& outFileNameGen)
    : m_pageSequence(pageSequence->toPageSequence(PAGE_VIEW)),
      m_outFileNameGen(outFileNameGen),
      m_selectedPage(selectedPage),
      m_layoutDirection(pageSequence->layoutDirection()) {
  int nextId = 1;
  for (const PageInfo& page : m_pageSequence) {
    const PageId& pageId = page.id();
    const ImageId& imageId = pageId.imageId();
    const QString& filePath = imageId.filePath();
    const QFileInfo fileInfo(filePath);
    const QString dirPath(fileInfo.absolutePath());

    m_metadataByImage[imageId] = page.metadata();

    if (m_dirs.insert(Directory(dirPath, nextId)).second) {
      ++nextId;
    }

    if (m_files.insert(File(filePath, nextId)).second) {
      ++nextId;
    }

    if (m_images.insert(Image(page, nextId)).second) {
      ++nextId;
    }

    if (m_pages.insert(Page(pageId, nextId)).second) {
      ++nextId;
    }
  }
}

ProjectWriter::~ProjectWriter() = default;

bool ProjectWriter::write(const QString& filePath, const std::vector<FilterPtr>& filters) const {
  QDomDocument doc;
  QDomElement rootEl(doc.createElement("project"));
  doc.appendChild(rootEl);
  rootEl.setAttribute("version", PROJECT_VERSION);
  rootEl.setAttribute("outputDirectory", m_outFileNameGen.outDir());
  rootEl.setAttribute("layoutDirection", m_layoutDirection == Qt::LeftToRight ? "LTR" : "RTL");

  rootEl.appendChild(processDirectories(doc));
  rootEl.appendChild(processFiles(doc));
  rootEl.appendChild(processImages(doc));
  rootEl.appendChild(processPages(doc));
  rootEl.appendChild(m_outFileNameGen.disambiguator()->toXml(doc, "file-name-disambiguation",
                                                             boost::bind(&ProjectWriter::packFilePath, this, _1)));

  QDomElement filtersEl(doc.createElement("filters"));
  rootEl.appendChild(filtersEl);
  auto it(filters.begin());
  const auto end(filters.end());
  for (; it != end; ++it) {
    filtersEl.appendChild((*it)->saveSettings(*this, doc));
  }

  QFile file(filePath);
  if (file.open(QIODevice::WriteOnly)) {
    QTextStream strm(&file);
    doc.save(strm, 2);
    return true;
  }
  return false;
}  // ProjectWriter::write

QDomElement ProjectWriter::processDirectories(QDomDocument& doc) const {
  QDomElement dirsEl(doc.createElement("directories"));

  for (const Directory& dir : m_dirs.get<Sequenced>()) {
    QDomElement dirEl(doc.createElement("directory"));
    dirEl.setAttribute("id", dir.numericId);
    dirEl.setAttribute("path", dir.path);
    dirsEl.appendChild(dirEl);
  }
  return dirsEl;
}

QDomElement ProjectWriter::processFiles(QDomDocument& doc) const {
  QDomElement filesEl(doc.createElement("files"));

  for (const File& file : m_files.get<Sequenced>()) {
    const QFileInfo fileInfo(file.path);
    const QString& dirPath = fileInfo.absolutePath();
    QDomElement fileEl(doc.createElement("file"));
    fileEl.setAttribute("id", file.numericId);
    fileEl.setAttribute("dirId", dirId(dirPath));
    fileEl.setAttribute("name", fileInfo.fileName());
    filesEl.appendChild(fileEl);
  }
  return filesEl;
}

QDomElement ProjectWriter::processImages(QDomDocument& doc) const {
  QDomElement imagesEl(doc.createElement("images"));

  for (const Image& image : m_images.get<Sequenced>()) {
    QDomElement imageEl(doc.createElement("image"));
    imageEl.setAttribute("id", image.numericId);
    imageEl.setAttribute("subPages", image.numSubPages);
    imageEl.setAttribute("fileId", fileId(image.id.filePath()));
    imageEl.setAttribute("fileImage", image.id.page());
    if (image.leftHalfRemoved != image.rightHalfRemoved) {
      // Both are not supposed to be removed.
      imageEl.setAttribute("removed", image.leftHalfRemoved ? "L" : "R");
    }
    writeImageMetadata(doc, imageEl, image.id);
    imagesEl.appendChild(imageEl);
  }
  return imagesEl;
}

void ProjectWriter::writeImageMetadata(QDomDocument& doc, QDomElement& imageEl, const ImageId& imageId) const {
  auto it(m_metadataByImage.find(imageId));
  assert(it != m_metadataByImage.end());
  const ImageMetadata& metadata = it->second;

  QDomElement sizeEl(doc.createElement("size"));
  sizeEl.setAttribute("width", metadata.size().width());
  sizeEl.setAttribute("height", metadata.size().height());
  imageEl.appendChild(sizeEl);

  QDomElement dpiEl(doc.createElement("dpi"));
  dpiEl.setAttribute("horizontal", metadata.dpi().horizontal());
  dpiEl.setAttribute("vertical", metadata.dpi().vertical());
  imageEl.appendChild(dpiEl);
}

QDomElement ProjectWriter::processPages(QDomDocument& doc) const {
  QDomElement pagesEl(doc.createElement("pages"));

  const PageId selOpt1(m_selectedPage.get(IMAGE_VIEW));
  const PageId selOpt2(m_selectedPage.get(PAGE_VIEW));

  PageId pageLeft;
  PageId pageRight;
  if (selOpt2.subPage() == PageId::SINGLE_PAGE) {
    // In case it was split later select first of its pages found
    pageLeft = PageId(selOpt2.imageId(), PageId::LEFT_PAGE);
    pageRight = PageId(selOpt2.imageId(), PageId::RIGHT_PAGE);
  }


  for (const PageInfo& page : m_pageSequence) {
    const PageId& pageId = page.id();
    QDomElement pageEl(doc.createElement("page"));
    pageEl.setAttribute("id", this->pageId(pageId));
    pageEl.setAttribute("imageId", imageId(pageId.imageId()));
    pageEl.setAttribute("subPage", pageId.subPageAsString());
    if ((pageId == selOpt1) || (pageId == selOpt2) || (pageId == pageLeft) || (pageId == pageRight)) {
      pageEl.setAttribute("selected", "selected");
      pageLeft = pageRight = PageId();  // if one of these match other shouldn't
    }
    pagesEl.appendChild(pageEl);
  }
  return pagesEl;
}  // ProjectWriter::processPages

int ProjectWriter::dirId(const QString& dirPath) const {
  const Directories::const_iterator it(m_dirs.find(dirPath));
  assert(it != m_dirs.end());
  return it->numericId;
}

int ProjectWriter::fileId(const QString& filePath) const {
  const Files::const_iterator it(m_files.find(filePath));
  if (it != m_files.end()) {
    return it->numericId;
  } else {
    return -1;
  }
}

QString ProjectWriter::packFilePath(const QString& filePath) const {
  const Files::const_iterator it(m_files.find(filePath));
  if (it != m_files.end()) {
    return QString::number(it->numericId);
  } else {
    return QString();
  }
}

int ProjectWriter::imageId(const ImageId& imageId) const {
  const Images::const_iterator it(m_images.find(imageId));
  assert(it != m_images.end());
  return it->numericId;
}

int ProjectWriter::pageId(const PageId& pageId) const {
  const Pages::const_iterator it(m_pages.find(pageId));
  assert(it != m_pages.end());
  return it->numericId;
}

void ProjectWriter::enumImagesImpl(const VirtualFunction<void, const ImageId&, int>& out) const {
  for (const Image& image : m_images.get<Sequenced>()) {
    out(image.id, image.numericId);
  }
}

void ProjectWriter::enumPagesImpl(const VirtualFunction<void, const PageId&, int>& out) const {
  for (const Page& page : m_pages.get<Sequenced>()) {
    out(page.id, page.numericId);
  }
}

/*======================== ProjectWriter::Image =========================*/

ProjectWriter::Image::Image(const PageInfo& page, int numericId)
    : id(page.imageId()),
      numericId(numericId),
      numSubPages(page.imageSubPages()),
      leftHalfRemoved(page.leftHalfRemoved()),
      rightHalfRemoved(page.rightHalfRemoved()) {}
