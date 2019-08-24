// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_NONCOPYABLE_H_
#define SCANTAILOR_FOUNDATION_NONCOPYABLE_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DECLARE_NON_COPYABLE(Class)            \
 public:                                       \
  /** \brief Copying is forbidden. */          \
  Class(const Class&) = delete;                \
  /** \brief Copying is forbidden. */          \
  Class& operator=(const Class&) = delete;     \
  /** \brief Moving is forbidden. */           \
  Class(Class&&) noexcept = delete;            \
  /** \brief Moving is forbidden. */           \
  Class& operator=(Class&&) noexcept = delete; \
                                               \
 private:

#endif
