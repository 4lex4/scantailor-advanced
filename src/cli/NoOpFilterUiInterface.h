// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CLI_NOOPFILTERUIINTERFACE_H_
#define SCANTAILOR_CLI_NOOPFILTERUIINTERFACE_H_

#include <core/FilterUiInterface.h>

class NoOpFilterUiInterface : public FilterUiInterface {
 public:
  void setOptionsWidget(FilterOptionsWidget* widget, Ownership ownership) override;
  void setImageWidget(QWidget* widget,
                      Ownership ownership,
                      DebugImages* debugImages = nullptr,
                      bool overlay = false) override;
  void invalidateThumbnail(const PageId& pageId) override;
  void invalidateAllThumbnails() override;
  std::shared_ptr<AbstractCommand<void>> relinkingDialogRequester() override;
};

#endif  // SCANTAILOR_CLI_NOOPFILTERUIINTERFACE_H_
