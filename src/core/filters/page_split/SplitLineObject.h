// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PAGE_SPLIT_SPLIT_LINE_OBJECT_H_
#define PAGE_SPLIT_SPLIT_LINE_OBJECT_H_

#include "DraggableObject.h"

namespace page_split {
class SplitLineObject : public DraggableObject {
 protected:
  virtual Proximity lineProximity(const QPointF& widget_mouse_pos, const InteractionState& interaction) const = 0;

  virtual QPointF linePosition(const InteractionState& interaction) const = 0;

  virtual void lineMoveRequest(const QPointF& widget_pos) = 0;

  virtual Proximity proximity(const QPointF& widget_mouse_pos, const InteractionState& interaction) {
    return lineProximity(widget_mouse_pos, interaction);
  }

  virtual QPointF position(const InteractionState& interaction) const { return linePosition(interaction); }

  virtual void moveRequest(const QPointF& widget_pos) { return lineMoveRequest(widget_pos); }
};
}  // namespace page_split
#endif  // ifndef PAGE_SPLIT_SPLIT_LINE_OBJECT_H_
