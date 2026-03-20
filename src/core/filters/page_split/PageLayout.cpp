// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PageLayout.h"

#include <PolygonUtils.h>

#include <QTransform>
#include <cassert>

#include "NumericTraits.h"
#include "ToLineProjector.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

using namespace imageproc;

namespace page_split {
PageLayout::PageLayout() : m_type(SINGLE_PAGE_UNCUT) {}

PageLayout::PageLayout(const QRectF& fullRect)
    : m_uncutOutline(fullRect),
      m_cutter1(fullRect.topLeft(), fullRect.bottomLeft()),
      m_cutter2(fullRect.topRight(), fullRect.bottomRight()),
      m_type(SINGLE_PAGE_UNCUT) {}

PageLayout::PageLayout(const QRectF& fullRect, const QLineF& cutter1, const QLineF& cutter2)
    : m_uncutOutline(fullRect), m_cutter1(cutter1), m_cutter2(cutter2), m_type(SINGLE_PAGE_CUT) {}

PageLayout::PageLayout(const QRectF fullRect, const QLineF& splitLine)
    : m_uncutOutline(fullRect), m_cutter1(splitLine), m_type(TWO_PAGES) {}

PageLayout::PageLayout(const QPolygonF& outline, const QLineF& cutter1, const QLineF& cutter2, Type type)
    : m_uncutOutline(outline), m_cutter1(cutter1), m_cutter2(cutter2), m_type(type) {}

PageLayout::PageLayout(const QDomElement& layoutEl)
    : m_uncutOutline(XmlUnmarshaller::polygonF(layoutEl.namedItem("outline").toElement())),
      m_cutter1(XmlUnmarshaller::lineF(layoutEl.namedItem("cutter1").toElement())),
      m_cutter2(XmlUnmarshaller::lineF(layoutEl.namedItem("cutter2").toElement())),
      m_type(typeFromString(layoutEl.attribute("type"))) {}

void PageLayout::setType(Type type) {
  m_type = type;
  if (type == TWO_PAGES) {
    m_cutter2 = m_cutter1;
  }
}

void PageLayout::setUncutOutline(const QRectF& outline) {
  m_uncutOutline = outline;
  if (m_uncutOutline.size() < 4) {
    // Empty rect?
    return;
  }
}

const QLineF& PageLayout::cutterLine(int idx) const {
  assert(idx >= 0 && idx < numCutters());
  return idx == 0 ? m_cutter1 : m_cutter2;
}

void PageLayout::setCutterLine(int idx, const QLineF& cutter) {
  assert(idx >= 0 && idx < numCutters());
  (idx == 0 ? m_cutter1 : m_cutter2) = cutter;
}

LayoutType PageLayout::toLayoutType() const {
  switch (m_type) {
    case SINGLE_PAGE_UNCUT:
      return page_split::SINGLE_PAGE_UNCUT;
    case SINGLE_PAGE_CUT:
      return page_split::PAGE_PLUS_OFFCUT;
    case TWO_PAGES:
      return page_split::TWO_PAGES;
  }

  assert(!"Unreachable");
  return page_split::SINGLE_PAGE_UNCUT;
}

int PageLayout::numCutters() const {
  switch (m_type) {
    case SINGLE_PAGE_UNCUT:
      return 0;
    case SINGLE_PAGE_CUT:
      return 2;
    case TWO_PAGES:
      return 1;
  }

  assert(!"Unreachable");
  return 0;
}

QLineF PageLayout::inscribedCutterLine(int idx) const {
  assert(idx >= 0 && idx < numCutters());

  if (m_uncutOutline.size() < 4) {
    return QLineF();
  }

  const QLineF rawLine(cutterLine(idx));
  const QPointF origin(rawLine.p1());
  const QPointF lineVec(rawLine.p2() - origin);

  double minProj = NumericTraits<double>::max();
  double maxProj = NumericTraits<double>::min();
  QPointF minPt;
  QPointF maxPt;

  QPointF p1, p2;
  double projection;

  for (int i = 0; i < 4; ++i) {
    const QLineF polySegment(m_uncutOutline[i], m_uncutOutline[(i + 1) & 3]);
    QPointF intersection;
#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR < 14
    auto is = polySegment.intersect(rawLine, &intersection);
#else
    auto is = polySegment.intersects(rawLine, &intersection);
#endif
    if (is == QLineF::NoIntersection) {
      continue;
    }

    // Project the intersection point on polySegment to check
    // if it's between its endpoints.
    p1 = polySegment.p2() - polySegment.p1();
    p2 = intersection - polySegment.p1();
    projection = p1.x() * p2.x() + p1.y() * p2.y();
    if ((projection < 0) || (projection > p1.x() * p1.x() + p1.y() * p1.y())) {
      // Intersection point not on the polygon segment.
      continue;
    }
    // Now project it on rawLine and update min/max projections.
    p1 = intersection - origin;
    p2 = lineVec;
    projection = p1.x() * p2.x() + p1.y() * p2.y();
    if (projection <= minProj) {
      minProj = projection;
      minPt = intersection;
    }
    if (projection >= maxProj) {
      maxProj = projection;
      maxPt = intersection;
    }
  }

  QLineF res(minPt, maxPt);
  ensureSameDirection(rawLine, res);
  return res;
}  // PageLayout::inscribedCutterLine

QPolygonF PageLayout::singlePageOutline() const {
  if (m_uncutOutline.size() < 4) {
    return QPolygonF();
  }

  switch (m_type) {
    case SINGLE_PAGE_UNCUT:
      return m_uncutOutline;
    case SINGLE_PAGE_CUT:
      break;
    case TWO_PAGES:
      return QPolygonF();
  }

  const QLineF line1(extendToCover(m_cutter1, m_uncutOutline));
  QLineF line2(extendToCover(m_cutter2, m_uncutOutline));
  ensureSameDirection(line1, line2);
  const QLineF reverseLine1(line1.p2(), line1.p1());
  const QLineF reverseLine2(line2.p2(), line2.p1());

  QPolygonF poly;
  poly << line1.p1();
  maybeAddIntersectionPoint(poly, line1.normalVector(), line2.normalVector());
  poly << line2.p1() << line2.p2();
  maybeAddIntersectionPoint(poly, reverseLine1.normalVector(), reverseLine2.normalVector());
  poly << line1.p2();
  return PolygonUtils::round(m_uncutOutline).intersected(PolygonUtils::round(poly));
}

QPolygonF PageLayout::leftPageOutline() const {
  if (m_uncutOutline.size() < 4) {
    return QPolygonF();
  }

  switch (m_type) {
    case SINGLE_PAGE_UNCUT:
    case SINGLE_PAGE_CUT:
      return QPolygonF();
    case TWO_PAGES:
      break;
  }

  const QLineF line1(m_uncutOutline[0], m_uncutOutline[3]);
  QLineF line2(extendToCover(m_cutter1, m_uncutOutline));
  ensureSameDirection(line1, line2);
  const QLineF reverseLine1(line1.p2(), line1.p1());
  const QLineF reverseLine2(line2.p2(), line2.p1());

  QPolygonF poly;
  poly << line1.p1();
  maybeAddIntersectionPoint(poly, line1.normalVector(), line2.normalVector());
  poly << line2.p1() << line2.p2();
  maybeAddIntersectionPoint(poly, reverseLine1.normalVector(), reverseLine2.normalVector());
  poly << line1.p2();
  return PolygonUtils::round(m_uncutOutline).intersected(PolygonUtils::round(poly));
}

QPolygonF PageLayout::rightPageOutline() const {
  if (m_uncutOutline.size() < 4) {
    return QPolygonF();
  }

  switch (m_type) {
    case SINGLE_PAGE_UNCUT:
    case SINGLE_PAGE_CUT:
      return QPolygonF();
    case TWO_PAGES:
      break;
  }

  const QLineF line1(m_uncutOutline[1], m_uncutOutline[2]);
  QLineF line2(extendToCover(m_cutter1, m_uncutOutline));
  ensureSameDirection(line1, line2);
  const QLineF reverseLine1(line1.p2(), line1.p1());
  const QLineF reverseLine2(line2.p2(), line2.p1());

  QPolygonF poly;
  poly << line1.p1();
  maybeAddIntersectionPoint(poly, line1.normalVector(), line2.normalVector());
  poly << line2.p1() << line2.p2();
  maybeAddIntersectionPoint(poly, reverseLine1.normalVector(), reverseLine2.normalVector());
  poly << line1.p2();
  return PolygonUtils::round(m_uncutOutline).intersected(PolygonUtils::round(poly));
}

QPolygonF PageLayout::pageOutline(const PageId::SubPage page) const {
  switch (page) {
    case PageId::SINGLE_PAGE:
      return singlePageOutline();
    case PageId::LEFT_PAGE:
      return leftPageOutline();
    case PageId::RIGHT_PAGE:
      return rightPageOutline();
  }

  assert(!"Unreachable");
  return QPolygonF();
}

PageLayout PageLayout::transformed(const QTransform& xform) const {
  return PageLayout(xform.map(m_uncutOutline), xform.map(m_cutter1), xform.map(m_cutter2), m_type);
}

QDomElement PageLayout::toXml(QDomDocument& doc, const QString& name) const {
  XmlMarshaller marshaller(doc);

  QDomElement el(doc.createElement(name));
  el.setAttribute("type", typeToString(m_type));
  el.appendChild(marshaller.polygonF(m_uncutOutline, "outline"));

  const int numCutters = this->numCutters();
  if (numCutters > 0) {
    el.appendChild(marshaller.lineF(m_cutter1, "cutter1"));
    if (numCutters > 1) {
      el.appendChild(marshaller.lineF(m_cutter2, "cutter2"));
    }
  }
  return el;
}

PageLayout::Type PageLayout::typeFromString(const QString& str) {
  if (str == "two-pages") {
    return TWO_PAGES;
  } else if (str == "single-cut") {
    return SINGLE_PAGE_CUT;
  } else {  // "single-uncut"
    return SINGLE_PAGE_UNCUT;
  }
}

QString PageLayout::typeToString(const Type type) {
  const char* str = nullptr;
  switch (type) {
    case SINGLE_PAGE_UNCUT:
      str = "single-uncut";
      break;
    case SINGLE_PAGE_CUT:
      str = "single-cut";
      break;
    case TWO_PAGES:
      str = "two-pages";
      break;
  }
  return QString::fromLatin1(str);
}

/**
 * Extends or shrinks a line segment in such a way that if you draw perpendicular
 * lines through its endpoints, the given polygon would be squeezed between these
 * two perpendiculars.  This ensures that the resulting line segment intersects
 * all the polygon edges it can possibly intersect.
 */
QLineF PageLayout::extendToCover(const QLineF& line, const QPolygonF& poly) {
  if (poly.isEmpty()) {
    return line;
  }

  // Project every vertex of the polygon onto the line and take extremas.

  double min = NumericTraits<double>::max();
  double max = NumericTraits<double>::min();
  const ToLineProjector projector(line);

  for (const QPointF& pt : poly) {
    const double scalar = projector.projectionScalar(pt);
    if (scalar < min) {
      min = scalar;
    }
    if (scalar > max) {
      max = scalar;
    }
  }
  return QLineF(line.pointAt(min), line.pointAt(max));
}

/**
 * Flips \p line2 if that would make the angle between the two lines more acute.
 * The angle between lines is interpreted as an angle between vectors
 * (line1.p2() - line1.p1()) and (line2.p2() - line2.p1()).
 */
void PageLayout::ensureSameDirection(const QLineF& line1, QLineF& line2) {
  const QPointF v1(line1.p2() - line1.p1());
  const QPointF v2(line2.p2() - line2.p1());
  const double dot = v1.x() * v2.x() + v1.y() * v2.y();
  if (dot < 0.0) {
    line2 = QLineF(line2.p2(), line2.p1());
  }
}

/**
 * Add the intersection point between \p line1 and \p line2
 * to \p poly, provided they intersect at all and the intersection
 * point is "between" line1.p1() and line2.p1().  We consider a point
 * to be between two other points by projecting it to the line between
 * those two points and checking if the projected point is between them.
 * When finding the intersection point, we treat \p line1 and \p line2
 * as lines, not line segments.
 */
void PageLayout::maybeAddIntersectionPoint(QPolygonF& poly, const QLineF& line1, const QLineF& line2) {
  QPointF intersection;
#if QT_VERSION_MAJOR == 5 && QT_VERSION_MINOR < 14
  auto is = line1.intersect(line2, &intersection);
#else
  auto is = line1.intersects(line2, &intersection);
#endif
  if (is == QLineF::NoIntersection) {
    return;
  }

  const ToLineProjector projector(QLineF(line1.p1(), line2.p1()));
  const double p = projector.projectionScalar(intersection);
  if ((p > 0.0) && (p < 1.0)) {
    poly << intersection;
  }
}

bool PageLayout::isNull() const {
  return m_uncutOutline.isEmpty();
}
}  // namespace page_split
