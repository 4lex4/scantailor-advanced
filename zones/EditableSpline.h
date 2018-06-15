/*

    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef EDITABLE_SPLINE_H_
#define EDITABLE_SPLINE_H_

#include <QPolygonF>
#include "SplineSegment.h"
#include "SplineVertex.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

class SerializableSpline;

class EditableSpline : public ref_countable {
 public:
  typedef intrusive_ptr<EditableSpline> Ptr;

  class SegmentIterator {
   public:
    explicit SegmentIterator(EditableSpline& spline) : m_ptrNextVertex(spline.firstVertex()) {}

    bool hasNext() const;

    SplineSegment next();

   private:
    SplineVertex::Ptr m_ptrNextVertex;
  };


  EditableSpline();

  EditableSpline(const SerializableSpline& spline);

  void appendVertex(const QPointF& pt);

  SplineVertex::Ptr firstVertex() const { return m_sentinel.firstVertex(); }

  SplineVertex::Ptr lastVertex() const { return m_sentinel.lastVertex(); }

  bool hasAtLeastSegments(int num) const;

  bool bridged() const { return m_sentinel.bridged(); }

  void setBridged(bool bridged) { m_sentinel.setBridged(true); }

  QPolygonF toPolygon() const;

 private:
  SentinelSplineVertex m_sentinel;
};


#endif  // ifndef EDITABLE_SPLINE_H_
