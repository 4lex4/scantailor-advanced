// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_ABSTRACTRELINKER_H_
#define SCANTAILOR_CORE_ABSTRACTRELINKER_H_

#include "ref_countable.h"

class RelinkablePath;
class QString;

class AbstractRelinker : public ref_countable {
 public:
  ~AbstractRelinker() override = default;

  /**
   * Returns the path to be used instead of the given path.
   * The same path will be returned if no substitution is to be made.
   */
  virtual QString substitutionPathFor(const RelinkablePath& origPath) const = 0;
};


#endif
