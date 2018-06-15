/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    ConsoleBatch - Batch processing scanned pages from command line.
    Copyright (C) 2011 Petr Kovar <pejuko@gmail.com>

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

#ifndef CONSOLEBATCH_H_
#define CONSOLEBATCH_H_

#include <QString>
#include <vector>

#include "BackgroundTask.h"
#include "FilterResult.h"
#include "ImageFileInfo.h"
#include "OutputFileNameGenerator.h"
#include "PageId.h"
#include "PageInfo.h"
#include "PageSelectionAccessor.h"
#include "PageView.h"
#include "ProjectPages.h"
#include "ProjectReader.h"
#include "StageSequence.h"
#include "ThumbnailPixmapCache.h"
#include "intrusive_ptr.h"


class ConsoleBatch {
  // Member-wise copying is OK.
 public:
  ConsoleBatch(const std::vector<ImageFileInfo>& images,
               const QString& output_directory,
               const Qt::LayoutDirection layout);

  ConsoleBatch(const QString project_file);

  void process();

  void saveProject(const QString project_file);

 private:
  bool batch;
  bool debug;
  intrusive_ptr<FileNameDisambiguator> m_ptrDisambiguator;
  intrusive_ptr<ProjectPages> m_ptrPages;
  intrusive_ptr<StageSequence> m_ptrStages;
  OutputFileNameGenerator m_outFileNameGen;
  intrusive_ptr<ThumbnailPixmapCache> m_ptrThumbnailCache;
  std::unique_ptr<ProjectReader> m_ptrReader;

  void setupFilter(int idx, std::set<PageId> allPages);

  void setupFixOrientation(std::set<PageId> allPages);

  void setupPageSplit(std::set<PageId> allPages);

  void setupDeskew(std::set<PageId> allPages);

  void setupSelectContent(std::set<PageId> allPages);

  void setupPageLayout(std::set<PageId> allPages);

  void setupOutput(std::set<PageId> allPages);

  BackgroundTaskPtr createCompositeTask(const PageInfo& page, const int last_filter_idx);
};


#endif  // ifndef CONSOLEBATCH_H_
