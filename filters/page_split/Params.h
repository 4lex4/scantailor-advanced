/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
