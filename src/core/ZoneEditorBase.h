// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_ZONEEDITORBASE_H_
#define SCANTAILOR_CORE_ZONEEDITORBASE_H_

#include <core/zones/EditableZoneSet.h>
#include <core/zones/ZoneInteractionContext.h>

#include "ImageViewBase.h"

class ZoneEditorBase : public ImageViewBase, protected InteractionHandler {
 public:
  ZoneEditorBase(const QImage& image,
                 const ImagePixmapUnion& downscaledVersion,
                 const ImagePresentation& presentation,
                 const Margins& margins = Margins());

  ~ZoneEditorBase() override;

  EditableZoneSet& zones() { return m_zones; }

  const EditableZoneSet& zones() const { return m_zones; }

  ZoneInteractionContext& context() { return m_context; }

  const ZoneInteractionContext& context() const { return m_context; }

 protected:
  void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) override;

  void showEvent(QShowEvent* event) override;

  void hideEvent(QHideEvent* event) override;

 private:
  class ZoneModeProvider;

  EditableZoneSet m_zones;
  ZoneInteractionContext m_context;
  std::unique_ptr<ZoneModeProvider> m_zoneModeProvider;
};


#endif  // SCANTAILOR_CORE_ZONEEDITORBASE_H_
