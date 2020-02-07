// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Utils.h"

#include <core/DefaultParams.h>
#include <core/DefaultParamsProvider.h>
#include <core/UnitsConverter.h>

#include "Params.h"

using namespace select_content;

Params Utils::buildDefaultParams(const Dpi& dpi) {
  const DefaultParams& defaultParams = DefaultParamsProvider::getInstance().getParams();
  const DefaultParams::SelectContentParams& selectContentParams = defaultParams.getSelectContentParams();

  const UnitsConverter unitsConverter(dpi);

  const QSizeF& pageRectSize = selectContentParams.getPageRectSize();
  double pageRectWidth = pageRectSize.width();
  double pageRectHeight = pageRectSize.height();
  unitsConverter.convert(pageRectWidth, pageRectHeight, defaultParams.getUnits(), PIXELS);

  return Params(QRectF(), QSizeF(), QRectF(QPointF(0, 0), QSizeF(pageRectWidth, pageRectHeight)), Dependencies(),
                selectContentParams.isContentDetectEnabled() ? MODE_AUTO : MODE_DISABLED,
                selectContentParams.getPageDetectMode(), selectContentParams.isFineTuneCorners());
}
