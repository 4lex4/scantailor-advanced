// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Utils.h"

#include <core/DefaultParams.h>
#include <core/DefaultParamsProvider.h>

using namespace page_split;

Settings::UpdateAction Utils::buildDefaultUpdateAction() {
  const DefaultParams& defaultParams = DefaultParamsProvider::getInstance().getParams();
  const DefaultParams::PageSplitParams& pageSplitParams = defaultParams.getPageSplitParams();

  Settings::UpdateAction update;
  update.setLayoutType(pageSplitParams.getLayoutType());

  return update;
}
