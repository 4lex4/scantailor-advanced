// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutputFileNameGenerator.h"
#include <QDir>
#include <QFileInfo>
#include <utility>
#include "AbstractRelinker.h"
#include "PageId.h"
#include "RelinkablePath.h"

OutputFileNameGenerator::OutputFileNameGenerator()
    : m_disambiguator(new FileNameDisambiguator), m_outDir(), m_layoutDirection(Qt::LeftToRight) {}

OutputFileNameGenerator::OutputFileNameGenerator(intrusive_ptr<FileNameDisambiguator> disambiguator,
                                                 const QString& outDir,
                                                 Qt::LayoutDirection layoutDirection)
    : m_disambiguator(std::move(disambiguator)), m_outDir(outDir), m_layoutDirection(layoutDirection) {
  assert(m_disambiguator);
}

void OutputFileNameGenerator::performRelinking(const AbstractRelinker& relinker) {
  m_disambiguator->performRelinking(relinker);
  m_outDir = relinker.substitutionPathFor(RelinkablePath(m_outDir, RelinkablePath::Dir));
}

QString OutputFileNameGenerator::fileNameFor(const PageId& page) const {
  const bool ltr = (m_layoutDirection == Qt::LeftToRight);
  const PageId::SubPage subPage = page.subPage();
  const int label = m_disambiguator->getLabel(page.imageId().filePath());

  QString name(QFileInfo(page.imageId().filePath()).completeBaseName());
  if (label != 0) {
    name += QString::fromLatin1("(%1)").arg(label);
  }
  if (page.imageId().isMultiPageFile()) {
    name += QString::fromLatin1("_page%1").arg(page.imageId().page(), 4, 10, QLatin1Char('0'));
  }
  if (subPage != PageId::SINGLE_PAGE) {
    name += QLatin1Char('_');
    name += QLatin1Char(ltr == (subPage == PageId::LEFT_PAGE) ? '1' : '2');
    name += QLatin1Char(subPage == PageId::LEFT_PAGE ? 'L' : 'R');
  }
  name += QString::fromLatin1(".tif");

  return name;
}

QString OutputFileNameGenerator::filePathFor(const PageId& page) const {
  const QString fileName(fileNameFor(page));

  return QDir(m_outDir).absoluteFilePath(fileName);
}
