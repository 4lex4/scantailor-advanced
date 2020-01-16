// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "UnitsListener.h"

#include "UnitsProvider.h"

UnitsListener::UnitsListener() {
  UnitsProvider::getInstance().addListener(this);
}

UnitsListener::~UnitsListener() {
  UnitsProvider::getInstance().removeListener(this);
}