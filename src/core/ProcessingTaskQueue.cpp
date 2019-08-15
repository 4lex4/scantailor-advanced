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

#include "ProcessingTaskQueue.h"

ProcessingTaskQueue::Entry::Entry(const PageInfo& page_info, const BackgroundTaskPtr& tsk)
    : pageInfo(page_info), task(tsk), takenForProcessing(false) {}

ProcessingTaskQueue::ProcessingTaskQueue() = default;

void ProcessingTaskQueue::addProcessingTask(const PageInfo& page_info, const BackgroundTaskPtr& task) {
  m_queue.emplace_back(page_info, task);
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


  const bool removing_selected_page = (m_selectedPage.id() == it->pageInfo.id());

  auto next_it(it);
  ++next_it;

  if ((next_it == end) && m_pageToSelectWhenDone.isNull()) {
    m_pageToSelectWhenDone = it->pageInfo;
  }

  m_queue.erase(it);

  if (removing_selected_page) {
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
