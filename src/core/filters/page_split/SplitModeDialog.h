// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PAGE_SPLIT_SPLITMODEDIALOG_H_
#define PAGE_SPLIT_SPLITMODEDIALOG_H_

#include <QButtonGroup>
#include <QDialog>
#include <set>
#include "LayoutType.h"
#include "PageId.h"
#include "PageLayout.h"
#include "PageSequence.h"
#include "intrusive_ptr.h"
#include "ui_SplitModeDialog.h"

class ProjectPages;
class PageSelectionAccessor;

namespace page_split {
class SplitModeDialog : public QDialog, private Ui::SplitModeDialog {
  Q_OBJECT
 public:
  SplitModeDialog(QWidget* parent,
                  const PageId& cur_page,
                  const PageSelectionAccessor& page_selection_accessor,
                  LayoutType layout_type,
                  PageLayout::Type auto_detected_layout_type);

  ~SplitModeDialog() override;

 signals:

  void accepted(const std::set<PageId>& pages, LayoutType layout_type, bool apply_cut);

 private slots:

  void autoDetectionSelected();

  void manualModeSelected();

  void onSubmit();

 private:
  LayoutType combinedLayoutType() const;

  static QIcon iconFor(LayoutType layout_type);

  PageSequence m_pages;
  std::set<PageId> m_selectedPages;
  PageId m_curPage;
  QButtonGroup* m_scopeGroup;
  LayoutType m_layoutType;
  PageLayout::Type m_autoDetectedLayoutType;
};
}  // namespace page_split
#endif  // ifndef PAGE_SPLIT_SPLITMODEDIALOG_H_
