// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ImageFileInfo.h"
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

bool ImageFileInfo::isDpiOK() const {
  using namespace boost::lambda;

  return std::find_if(m_imageInfo.begin(), m_imageInfo.end(), !bind(&ImageMetadata::isDpiOK, _1)) == m_imageInfo.end();
}
