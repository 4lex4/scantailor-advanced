// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PictureZonePropDialog.h"

#include <utility>

#include "PictureLayerProperty.h"

namespace output {
PictureZonePropDialog::PictureZonePropDialog(std::shared_ptr<PropertySet> props, QWidget* parent)
    : QDialog(parent), m_props(std::move(props)) {
  ui.setupUi(this);

  switch (m_props->locateOrDefault<PictureLayerProperty>()->layer()) {
    case PictureLayerProperty::ZONENOOP:
      break;
    case PictureLayerProperty::ZONEERASER1:
      ui.zoneeraser1->setChecked(true);
      break;
    case PictureLayerProperty::ZONEPAINTER2:
      ui.zonepainter2->setChecked(true);
      break;
    case PictureLayerProperty::ZONEERASER3:
      ui.zoneeraser3->setChecked(true);
      break;
    case PictureLayerProperty::ZONEFG:
      ui.zoneforeground->setChecked(true);
      break;
    case PictureLayerProperty::ZONEBG:
      ui.zonebackground->setChecked(true);
      break;
  }

  connect(ui.zoneeraser1, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
  connect(ui.zonepainter2, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
  connect(ui.zoneeraser3, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
  connect(ui.zoneforeground, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
  connect(ui.zonebackground, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
}

void PictureZonePropDialog::itemToggled(bool selected) {
  PictureLayerProperty::Layer layer = PictureLayerProperty::ZONENOOP;

  QObject* const obj = sender();
  if (obj == ui.zoneeraser1) {
    layer = PictureLayerProperty::ZONEERASER1;
  } else if (obj == ui.zonepainter2) {
    layer = PictureLayerProperty::ZONEPAINTER2;
  } else if (obj == ui.zoneeraser3) {
    layer = PictureLayerProperty::ZONEERASER3;
  } else if (obj == ui.zoneforeground) {
    layer = PictureLayerProperty::ZONEFG;
  } else if (obj == ui.zonebackground) {
    layer = PictureLayerProperty::ZONEBG;
  }

  m_props->locateOrCreate<PictureLayerProperty>()->setLayer(layer);

  emit updated();
}
}  // namespace output
