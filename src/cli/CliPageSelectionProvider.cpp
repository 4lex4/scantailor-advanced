// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CliPageSelectionProvider.h"

#include <core/PageRange.h>
#include <core/PageSequence.h>

CliPageSelectionProvider::CliPageSelectionProvider(PageSequence pages) : m_pages(std::move(pages)) {}

PageSequence CliPageSelectionProvider::allPages() const {
  return m_pages;
}

std::set<PageId> CliPageSelectionProvider::selectedPages() const {
  return m_pages.selectAll();
}

std::vector<PageRange> CliPageSelectionProvider::selectedRanges() const {
  std::vector<PageRange> ranges;
  PageRange range;
  for (size_t i = 0; i < m_pages.numPages(); ++i) {
    range.pages.push_back(m_pages.pageAt(i).id());
  }
  if (!range.pages.empty()) {
    ranges.push_back(range);
  }
  return ranges;
}
