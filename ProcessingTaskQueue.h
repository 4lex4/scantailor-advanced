/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef PROCESSING_TASK_QUEUE_H_
#define PROCESSING_TASK_QUEUE_H_

#include <list>
#include <set>
#include "BackgroundTask.h"
#include "NonCopyable.h"
#include "PageId.h"
#include "PageInfo.h"

class ProcessingTaskQueue {
  DECLARE_NON_COPYABLE(ProcessingTaskQueue)

 public:
  ProcessingTaskQueue();

  void addProcessingTask(const PageInfo& page_info, const BackgroundTaskPtr& task);

  /**
   * The first task among those that haven't been already taken for processing
   * is marked as taken and returned.  A null task will be returned if there
   * are no such tasks.
   */
  BackgroundTaskPtr takeForProcessing();

  void processingFinished(const BackgroundTaskPtr& task);

  /**
   * \brief Returns the page to be visually selected.
   *
   * To be called after takeForProcessing() / processingFinished().
   * It may return a null PageInfo, meaning not to change whatever
   * selection we currently have.
   */
  PageInfo selectedPage() const;

  bool allProcessed() const;

  void cancelAndRemove(const std::set<PageId>& pages);

  void cancelAndClear();

 private:
  struct Entry {
    PageInfo pageInfo;
    BackgroundTaskPtr task;
    bool takenForProcessing;

    Entry(const PageInfo& page_info, const BackgroundTaskPtr& task);
  };

  std::list<Entry> m_queue;
  PageInfo m_selectedPage;
  PageInfo m_pageToSelectWhenDone;
};


#endif  // ifndef PROCESSING_TASK_QUEUE_H_
