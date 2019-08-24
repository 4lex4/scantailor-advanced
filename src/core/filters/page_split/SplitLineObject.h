// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_PAGE_SPLIT_SPLITLINEOBJECT_H_
#define SCANTAILOR_PAGE_SPLIT_SPLITLINEOBJECT_H_

#include "DraggableObject.h"

namespace page_split {
class SplitLineObject : public DraggableObject {
 protected:
  virtual Proximity lineProximity(const QPointF& widgetMousePos, const InteractionState& interaction) const = 0;

  virtual QPointF linePosition(const InteractionState& interaction) const = 0;

  virtual void lineMoveRequest(const QPointF& widgetPos) = 0;

  virtual Proximity proximity(const QPointF& widgetMousePos, const InteractionState& interaction) {
    return lineProximity(widgetMousePos, interaction);
  }

  virtual QPointF position(const InteractionState& interaction) const { return linePosition(interaction); }

  virtual void moveRequest(const QPointF& widgetPos) { return lineMoveRequest(widgetPos); }
};
}  // namespace page_split
#endif  // ifndef SCANTAILOR_PAGE_SPLIT_SPLITLINEOBJECT_H_
