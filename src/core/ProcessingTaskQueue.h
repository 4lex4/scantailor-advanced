// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
