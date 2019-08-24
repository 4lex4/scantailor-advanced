// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_OUTPUTMARGINS_H_
#define SCANTAILOR_OUTPUT_OUTPUTMARGINS_H_

#include "Margins.h"

namespace output {
/**
 * Having margins on the Output stage is useful when creating zones
 * that are meant to cover a corner or an edge of a page.
 * We use the same margins on all tabs to preserve their geometrical
 * one-to-one relationship.
 */
class OutputMargins : public Margins {
 public:
  OutputMargins() : Margins(10.0, 10.0, 10.0, 10.0) {}
};
}  // namespace output
#endif
