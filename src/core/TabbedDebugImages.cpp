// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TabbedDebugImages.h"
#include "DebugImageView.h"

TabbedDebugImages::TabbedDebugImages(QWidget* parent) : QTabWidget(parent) {
  setDocumentMode(true);
  connect(this, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int)));
}

void TabbedDebugImages::currentTabChanged(const int idx) {
  if (auto* div = dynamic_cast<DebugImageView*>(widget(idx))) {
    div->unlink();
    m_liveViews.push_back(*div);
    removeExcessLiveViews();
    div->setLive(true);
  }
}

void TabbedDebugImages::removeExcessLiveViews() {
  auto remaining = static_cast<int>(m_liveViews.size());
  for (; remaining > MAX_LIVE_VIEWS; --remaining) {
    m_liveViews.front().setLive(false);
    m_liveViews.erase(m_liveViews.begin());
  }
}
