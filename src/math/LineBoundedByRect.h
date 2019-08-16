// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef LINE_BOUNDED_BY_RECT_H_
#define LINE_BOUNDED_BY_RECT_H_

#include <QLineF>
#include <QRectF>

/**
 * If \p line (not line segment!) intersects with \p rect,
 * writes intersection points as the new \p line endpoints
 * and returns true.  Otherwise returns false and leaves
 * \p line unmodified.
 */
bool lineBoundedByRect(QLineF& line, const QRectF& rect);

#endif
