// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PAGEORDERPROVIDER_H_
#define SCANTAILOR_CORE_PAGEORDERPROVIDER_H_

#include <memory>

class PageId;

/**
 * A base interface for different page ordering strategies.
 */
class PageOrderProvider : public std::enable_shared_from_this<PageOrderProvider> {
 public:
  virtual ~PageOrderProvider() = default;

  /**
   * Returns true if \p lhsPage precedes \p rhsPage.
   * \p lhsIncomplete and \p rhsIncomplete indicate whether
   * a page is represented by IncompleteThumbnail.
   */
  virtual bool precedes(const PageId& lhsPage, bool lhsIncomplete, const PageId& rhsPage, bool rhsIncomplete) const = 0;

  virtual std::shared_ptr<const PageOrderProvider> reversed() const;
};


#endif
