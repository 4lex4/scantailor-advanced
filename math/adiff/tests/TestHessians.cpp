
/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Function.h"
#include <boost/test/auto_unit_test.hpp>

namespace adiff
{
    namespace tests
    {
        BOOST_AUTO_TEST_SUITE(AutomaticDifferentiationTestSuite);

        BOOST_AUTO_TEST_CASE(test1)
        {
            SparseMap<2> sparse_map(1);
            sparse_map.markAllNonZero();

            Function<2> const x(0, 3, sparse_map);
            Function<2> const res(x * x);

            VecT<double> const gradient(res.gradient(sparse_map));
            MatT<double> const hessian(res.hessian(sparse_map));


            BOOST_REQUIRE_CLOSE(res.value, 9, 1e-06);
            BOOST_REQUIRE_CLOSE(gradient[0], 6, 1e-06);
            BOOST_REQUIRE_CLOSE(hessian(0, 0), 2, 1e-06);
        }

        BOOST_AUTO_TEST_CASE(test2)
        {
            SparseMap<2> sparse_map(2);
            sparse_map.markAllNonZero();

            Function<2> const x(0, 3, sparse_map);
            Function<2> const res(x * x);

            VecT<double> const gradient(res.gradient(sparse_map));
            MatT<double> const hessian(res.hessian(sparse_map));


            BOOST_REQUIRE_CLOSE(res.value, 9, 1e-06);
            BOOST_REQUIRE_CLOSE(gradient[0], 6, 1e-06);
            BOOST_REQUIRE_CLOSE(gradient[1], 0, 1e-06);
            BOOST_REQUIRE_CLOSE(hessian(0, 0), 2, 1e-06);
            BOOST_REQUIRE_CLOSE(hessian(1, 0), 0, 1e-06);
            BOOST_REQUIRE_CLOSE(hessian(0, 1), 0, 1e-06);
            BOOST_REQUIRE_CLOSE(hessian(1, 1), 0, 1e-06);
        }

        BOOST_AUTO_TEST_CASE(test3)
        {
            SparseMap<2> sparse_map(2);
            sparse_map.markAllNonZero();

            Function<2> const x(0, 2, sparse_map);
            Function<2> const y(1, 3, sparse_map);
            Function<2> const xxx(x * x * x);
            Function<2> const yy(y * y);
            Function<2> const res(xxx * yy);

            VecT<double> const gradient(res.gradient(sparse_map));
            MatT<double> const hessian(res.hessian(sparse_map));


            BOOST_REQUIRE_CLOSE(res.value, 72, 1e-06);
            BOOST_REQUIRE_CLOSE(gradient[0], 108, 1e-06);
            BOOST_REQUIRE_CLOSE(gradient[1], 48, 1e-06);
            BOOST_REQUIRE_CLOSE(hessian(0, 0), 108, 1e-06);
            BOOST_REQUIRE_CLOSE(hessian(0, 1), 72, 1e-06);
            BOOST_REQUIRE_CLOSE(hessian(1, 0), 72, 1e-06);
            BOOST_REQUIRE_CLOSE(hessian(1, 1), 16, 1e-06);
        }

        BOOST_AUTO_TEST_SUITE_END();
    }  // namespace tests
}  // namespace adiff