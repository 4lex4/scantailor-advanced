// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ProcessingTaskQueue.h"

ProcessingTaskQueue::Entry::Entry(const PageInfo& pageInfo, const BackgroundTaskPtr& tsk)
    : pageInfo(pageInfo), task(tsk), takenForProcessing(false) {}

ProcessingTaskQueue::ProcessingTaskQueue() = default;

void ProcessingTaskQueue::addProcessingTask(const PageInfo& pageInfo, const BackgroundTaskPtr& task) {
  m_queue.emplace_back(pageInfo, task);
  m_pageToSelectWhenDone = PageInfo();
}

BackgroundTaskPtr ProcessingTaskQueue::takeForProcessing() {
  for (Entry& ent : m_queue) {
    if (!ent.takenForProcessing) {
      ent.takenForProcessing = true;

      if (m_selectedPage.isNull()) {
        // In this mode we select the most recently submitted for processing page.
        // This means question marks on selected pages, but at least this avoids
        // jumps caused by dynamic ordering.
        m_selectedPage = ent.pageInfo;
      }
      return ent.task;
    }
  }
  return nullptr;
}

void ProcessingTaskQueue::processingFinished(const BackgroundTaskPtr& task) {
  auto it(m_queue.begin());
  const auto end(m_queue.end());

  for (;; ++it) {
    if (it == end) {
      // Task not found.
      return;
    }

    if (!it->takenForProcessing) {
      // There is no point in looking further.
      return;
    }

    if (it->task == task) {
      break;
    }
  }


  const bool removingSelectedPage = (m_selectedPage.id() == it->pageInfo.id());

  auto nextIt(it);
  ++nextIt;

  if ((nextIt == end) && m_pageToSelectWhenDone.isNull()) {
    m_pageToSelectWhenDone = it->pageInfo;
  }

  m_queue.erase(it);

  if (removingSelectedPage) {
    if (!m_queue.empty()) {
      m_selectedPage = m_queue.front().pageInfo;
    } else if (!m_pageToSelectWhenDone.isNull()) {
      m_selectedPage = m_pageToSelectWhenDone;
    }
  }
}  // ProcessingTaskQueue::processingFinished

PageInfo ProcessingTaskQueue::selectedPage() const {
  return m_selectedPage;
}

bool ProcessingTaskQueue::allProcessed() const {
  return m_queue.empty();
}

void ProcessingTaskQueue::cancelAndRemove(const std::set<PageId>& pages) {
  auto it(m_queue.begin());
  const auto end(m_queue.end());
  while (it != end) {
    if (pages.find(it->pageInfo.id()) == pages.end()) {
      ++it;
    } else {
      if (it->takenForProcessing) {
        it->task->cancel();
      }

      if (m_selectedPage.id() == it->pageInfo.id()) {
        m_selectedPage = PageInfo();
      }


      m_queue.erase(it++);
    }
  }
}

void ProcessingTaskQueue::cancelAndClear() {
  while (!m_queue.empty()) {
    Entry& ent = m_queue.front();
    if (ent.takenForProcessing) {
      ent.task->cancel();
    }
    m_queue.pop_front();
  }
  m_selectedPage = m_pageToSelectWhenDone;
}
