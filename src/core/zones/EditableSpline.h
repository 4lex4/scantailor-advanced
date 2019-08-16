// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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
    explicit SegmentIterator(EditableSpline& spline) : m_nextVertex(spline.firstVertex()) {}

    bool hasNext() const;

    SplineSegment next();

   private:
    SplineVertex::Ptr m_nextVertex;
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
