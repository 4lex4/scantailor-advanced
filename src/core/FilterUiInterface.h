// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_FILTERUIINTERFACE_H_
#define SCANTAILOR_CORE_FILTERUIINTERFACE_H_

#include "AbstractCommand.h"
#include "PageId.h"
#include "intrusive_ptr.h"

class DebugImages;
class FilterOptionsWidget;
class QWidget;

/**
 * \brief A reduced interface to MainWindow to allow filters to manupulate the UI.
 */
class FilterUiInterface {
 public:
  enum Ownership { KEEP_OWNERSHIP, TRANSFER_OWNERSHIP };

  virtual ~FilterUiInterface() = default;

  virtual void setOptionsWidget(FilterOptionsWidget* widget, Ownership ownership) = 0;

  virtual void setImageWidget(QWidget* widget,
                              Ownership ownership,
                              DebugImages* debugImages = nullptr,
                              bool overlay = false)
      = 0;

  virtual void invalidateThumbnail(const PageId& pageId) = 0;

  virtual void invalidateAllThumbnails() = 0;

  /**
   * Returns a callable object that when called will open a relinking dialog.
   */
  virtual intrusive_ptr<AbstractCommand<void>> relinkingDialogRequester() = 0;
};


#endif  // ifndef SCANTAILOR_CORE_FILTERUIINTERFACE_H_
