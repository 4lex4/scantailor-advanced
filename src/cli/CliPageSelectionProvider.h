// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CLI_CLIPAGESELECTIONPROVIDER_H_
#define SCANTAILOR_CLI_CLIPAGESELECTIONPROVIDER_H_

#include <core/PageSelectionProvider.h>
#include <core/PageSequence.h>

class CliPageSelectionProvider : public PageSelectionProvider {
 public:
  explicit CliPageSelectionProvider(PageSequence pages);

  PageSequence allPages() const override;
  std::set<PageId> selectedPages() const override;
  std::vector<PageRange> selectedRanges() const override;

 private:
  PageSequence m_pages;
};

#endif  // SCANTAILOR_CLI_CLIPAGESELECTIONPROVIDER_H_
