// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <Binarize.h>
#include <BinaryImage.h>
#include <QImage>
#include <QSize>
#include <boost/test/unit_test.hpp>
#include "Utils.h"

namespace imageproc {
namespace tests {
using namespace utils;

BOOST_AUTO_TEST_SUITE(BinarizeTestSuite)
#if 0
            BOOST_AUTO_TEST_CASE(test) {
                QImage img("test.png");
                binarizeWolf(img).toQImage().save("out.png");
            }
#endif
BOOST_AUTO_TEST_SUITE_END()
}  // namespace tests
}  // namespace imageproc
