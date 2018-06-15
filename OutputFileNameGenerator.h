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

#ifndef OUTPUT_FILE_NAME_GENERATOR_H_
#define OUTPUT_FILE_NAME_GENERATOR_H_

#include <QString>
#include <Qt>
#include "FileNameDisambiguator.h"
#include "intrusive_ptr.h"

class PageId;
class AbstractRelinker;

class OutputFileNameGenerator {
  // Member-wise copying is OK.
 public:
  OutputFileNameGenerator();

  OutputFileNameGenerator(intrusive_ptr<FileNameDisambiguator> disambiguator,
                          const QString& out_dir,
                          Qt::LayoutDirection layout_direction);

  void performRelinking(const AbstractRelinker& relinker);

  Qt::LayoutDirection layoutDirection() const { return m_layoutDirection; }

  const QString& outDir() const { return m_outDir; }

  FileNameDisambiguator* disambiguator() { return m_ptrDisambiguator.get(); }

  const FileNameDisambiguator* disambiguator() const { return m_ptrDisambiguator.get(); }

  QString fileNameFor(const PageId& page) const;

  QString filePathFor(const PageId& page) const;

 private:
  intrusive_ptr<FileNameDisambiguator> m_ptrDisambiguator;
  QString m_outDir;
  Qt::LayoutDirection m_layoutDirection;
};


#endif  // ifndef OUTPUT_FILE_NAME_GENERATOR_H_
