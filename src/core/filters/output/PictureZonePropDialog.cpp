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
    case PictureLayerProperty::NO_OP:
      break;
    case PictureLayerProperty::ERASER1:
      ui.eraser1->setChecked(true);
      break;
    case PictureLayerProperty::PAINTER2:
      ui.painter2->setChecked(true);
      break;
    case PictureLayerProperty::ERASER3:
      ui.eraser3->setChecked(true);
      break;
  }

  connect(ui.eraser1, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
  connect(ui.painter2, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
  connect(ui.eraser3, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
}

void PictureZonePropDialog::itemToggled(bool selected) {
  PictureLayerProperty::Layer layer = PictureLayerProperty::NO_OP;

  QObject* const obj = sender();
  if (obj == ui.eraser1) {
    layer = PictureLayerProperty::ERASER1;
  } else if (obj == ui.painter2) {
    layer = PictureLayerProperty::PAINTER2;
  } else if (obj == ui.eraser3) {
    layer = PictureLayerProperty::ERASER3;
  }

  m_props->locateOrCreate<PictureLayerProperty>()->setLayer(layer);

  emit updated();
}
}  // namespace output