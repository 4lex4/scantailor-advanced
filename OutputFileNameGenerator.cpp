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

#include "OutputFileNameGenerator.h"
#include <QDir>
#include <QFileInfo>
#include <utility>
#include "AbstractRelinker.h"
#include "PageId.h"
#include "RelinkablePath.h"

OutputFileNameGenerator::OutputFileNameGenerator()
    : m_ptrDisambiguator(new FileNameDisambiguator), m_outDir(), m_layoutDirection(Qt::LeftToRight) {}

OutputFileNameGenerator::OutputFileNameGenerator(intrusive_ptr<FileNameDisambiguator> disambiguator,
                                                 const QString& out_dir,
                                                 Qt::LayoutDirection layout_direction)
    : m_ptrDisambiguator(std::move(disambiguator)), m_outDir(out_dir), m_layoutDirection(layout_direction) {
  assert(m_ptrDisambiguator);
}

void OutputFileNameGenerator::performRelinking(const AbstractRelinker& relinker) {
  m_ptrDisambiguator->performRelinking(relinker);
  m_outDir = relinker.substitutionPathFor(RelinkablePath(m_outDir, RelinkablePath::Dir));
}

QString OutputFileNameGenerator::fileNameFor(const PageId& page) const {
  const bool ltr = (m_layoutDirection == Qt::LeftToRight);
  const PageId::SubPage sub_page = page.subPage();
  const int label = m_ptrDisambiguator->getLabel(page.imageId().filePath());

  QString name(QFileInfo(page.imageId().filePath()).completeBaseName());
  if (label != 0) {
    name += QString::fromLatin1("(%1)").arg(label);
  }
  if (page.imageId().isMultiPageFile()) {
    name += QString::fromLatin1("_page%1").arg(page.imageId().page(), 4, 10, QLatin1Char('0'));
  }
  if (sub_page != PageId::SINGLE_PAGE) {
    name += QLatin1Char('_');
    name += QLatin1Char(ltr == (sub_page == PageId::LEFT_PAGE) ? '1' : '2');
    name += QLatin1Char(sub_page == PageId::LEFT_PAGE ? 'L' : 'R');
  }
  name += QString::fromLatin1(".tif");

  return name;
}

QString OutputFileNameGenerator::filePathFor(const PageId& page) const {
  const QString file_name(fileNameFor(page));

  return QDir(m_outDir).absoluteFilePath(file_name);
}
