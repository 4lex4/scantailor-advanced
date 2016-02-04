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
#include <boost/foreach.hpp>

ProcessingTaskQueue::Entry::Entry(
        PageInfo const& page_info, BackgroundTaskPtr const& tsk)
        : pageInfo(page_info),
          task(tsk),
          takenForProcessing(false)
{
}

ProcessingTaskQueue::ProcessingTaskQueue()
{
}

void
ProcessingTaskQueue::addProcessingTask(
        PageInfo const& page_info, BackgroundTaskPtr const& task)
{
    m_queue.push_back(Entry(page_info, task));
    m_pageToSelectWhenDone = PageInfo();
}

BackgroundTaskPtr
ProcessingTaskQueue::takeForProcessing()
{
    for (Entry& ent :  m_queue) {
        if (!ent.takenForProcessing) {
            ent.takenForProcessing = true;

            if (m_selectedPage.isNull()) {
                m_selectedPage = ent.pageInfo;
            }

            return ent.task;
        }
    }

    return BackgroundTaskPtr();
}

void
ProcessingTaskQueue::processingFinished(BackgroundTaskPtr const& task)
{
    std::list<Entry>::iterator it(m_queue.begin());
    std::list<Entry>::iterator const end(m_queue.end());

    for (; ; ++it) {
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

    // If we reached this point, it means we've found our entry and
    // have <it> pointing to it.

    bool const removing_selected_page = (m_selectedPage.id() == it->pageInfo.id());

    std::list<Entry>::iterator next_it(it);
    ++next_it;

    if (next_it == end && m_pageToSelectWhenDone.isNull()) {
        m_pageToSelectWhenDone = it->pageInfo;
    }

    m_queue.erase(it);

    if (removing_selected_page) {
        if (!m_queue.empty()) {
            m_selectedPage = m_queue.front().pageInfo;
        }
        else if (!m_pageToSelectWhenDone.isNull()) {
            m_selectedPage = m_pageToSelectWhenDone;
        }
    }
}

PageInfo
ProcessingTaskQueue::selectedPage() const
{
    return m_selectedPage;
}

bool
ProcessingTaskQueue::allProcessed() const
{
    return m_queue.empty();
}

void
ProcessingTaskQueue::cancelAndRemove(std::set<PageId> const& pages)
{
    std::list<Entry>::iterator it(m_queue.begin());
    std::list<Entry>::iterator const end(m_queue.end());
    while (it != end) {
        if (pages.find(it->pageInfo.id()) == pages.end()) {
            ++it;
        }
        else {
            if (it->takenForProcessing) {
                it->task->cancel();
            }

            if (m_selectedPage.id() == it->pageInfo.id()) {
                m_selectedPage = PageInfo();
            }

            // We don't bother matching against m_pageToSelectWhenDone,
            // because in the current implementation that's always a page
            // that's no longer in the queue.

            m_queue.erase(it++);
        }
    }
}

void
ProcessingTaskQueue::cancelAndClear()
{
    while (!m_queue.empty()) {
        Entry& ent = m_queue.front();
        if (ent.takenForProcessing) {
            ent.task->cancel();
        }
        m_queue.pop_front();
    }
    m_selectedPage = m_pageToSelectWhenDone;
}
