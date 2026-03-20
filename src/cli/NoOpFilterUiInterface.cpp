// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "NoOpFilterUiInterface.h"

#include <core/AbstractCommand.h>
#include <core/FilterOptionsWidget.h>

void NoOpFilterUiInterface::setOptionsWidget(FilterOptionsWidget* widget, Ownership ownership) {
  (void) widget;
  (void) ownership;
}

void NoOpFilterUiInterface::setImageWidget(QWidget* widget,
                                            Ownership ownership,
                                            DebugImages* debugImages,
                                            bool overlay) {
  (void) widget;
  (void) ownership;
  (void) debugImages;
  (void) overlay;
}

void NoOpFilterUiInterface::invalidateThumbnail(const PageId& pageId) {
  (void) pageId;
}

void NoOpFilterUiInterface::invalidateAllThumbnails() {}

std::shared_ptr<AbstractCommand<void>> NoOpFilterUiInterface::relinkingDialogRequester() {
  return nullptr;
}
