// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef PAGE_SPLIT_PARAMS_H_
#define PAGE_SPLIT_PARAMS_H_

#include <QString>
#include <utility>
#include "AutoManualMode.h"
#include "Dependencies.h"
#include "PageLayout.h"

class QDomDocument;
class QDomElement;

namespace page_split {
class Params {
 public:
  // Member-wise copying is OK.

  Params(const PageLayout& layout, const Dependencies& deps, AutoManualMode split_line_mode);

  explicit Params(const QDomElement& el);

  ~Params();

  const PageLayout& pageLayout() const;

  void setPageLayout(const PageLayout& layout);

  const Dependencies& dependencies() const;

  void setDependencies(const Dependencies& deps);

  AutoManualMode splitLineMode() const;

  void setSplitLineMode(AutoManualMode mode);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

 private:
  PageLayout m_layout;
  Dependencies m_deps;
  AutoManualMode m_splitLineMode;
};
}  // namespace page_split
#endif  // ifndef PAGE_SPLIT_PARAMS_H_
