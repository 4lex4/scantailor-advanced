// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageSelectionAccessor.h"

#include <utility>

#include "PageSequence.h"

PageSelectionAccessor::PageSelectionAccessor(intrusive_ptr<const PageSelectionProvider> provider)
    : m_provider(std::move(provider)) {}

PageSequence PageSelectionAccessor::allPages() const {
  return m_provider->allPages();
}

std::set<PageId> PageSelectionAccessor::selectedPages() const {
  return m_provider->selectedPages();
}

std::vector<PageRange> PageSelectionAccessor::selectedRanges() const {
  return m_provider->selectedRanges();
}
