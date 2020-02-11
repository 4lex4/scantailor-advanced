// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_ABSTRACTCOMMAND_H_
#define SCANTAILOR_CORE_ABSTRACTCOMMAND_H_

#include <memory>

template <typename Res, typename... ArgTypes>
class AbstractCommand {
 public:
  using Ptr = std::shared_ptr<AbstractCommand>;

  virtual ~AbstractCommand() = default;

  virtual Res operator()(ArgTypes... args) = 0;
};

#endif  // ifndef SCANTAILOR_CORE_ABSTRACTCOMMAND_H_
