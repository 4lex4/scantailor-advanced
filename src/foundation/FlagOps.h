// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef FLAGOPS_H_
#define FLAGOPS_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DEFINE_FLAG_OPS(type)                                                               \
  inline type operator&(type lhs, type rhs) { return type(unsigned(lhs) & unsigned(rhs)); } \
                                                                                            \
  inline type operator|(type lhs, type rhs) { return type(unsigned(lhs) | unsigned(rhs)); } \
                                                                                            \
  inline type operator^(type lhs, type rhs) { return type(unsigned(lhs) ^ unsigned(rhs)); } \
                                                                                            \
  inline type operator~(type val) { return type(~unsigned(val)); }                          \
                                                                                            \
  inline type& operator&=(type& lhs, type rhs) {                                            \
    lhs = lhs & rhs;                                                                        \
    return lhs;                                                                             \
  }                                                                                         \
                                                                                            \
  inline type& operator|=(type& lhs, type rhs) {                                            \
    lhs = lhs | rhs;                                                                        \
    return lhs;                                                                             \
  }                                                                                         \
                                                                                            \
  inline type& operator^=(type& lhs, type rhs) {                                            \
    lhs = lhs ^ rhs;                                                                        \
    return lhs;                                                                             \
  }

#endif  // ifndef FLAGOPS_H_
