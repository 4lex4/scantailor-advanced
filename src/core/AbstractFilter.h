// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_ABSTRACTFILTER_H_
#define SCANTAILOR_CORE_ABSTRACTFILTER_H_

#include <vector>

#include "PageOrderOption.h"
#include "PageView.h"
#include "ref_countable.h"

class FilterUiInterface;
class PageInfo;
class ProjectReader;
class ProjectWriter;
class AbstractRelinker;
class QString;
class QDomDocument;
class QDomElement;

/**
 * Filters represent processing stages, like "Deskew", "Margins" and "Output".
 */
class AbstractFilter : public ref_countable {
 public:
  ~AbstractFilter() override = default;

  virtual QString getName() const = 0;

  virtual PageView getView() const = 0;

  virtual void selected() {}

  virtual int selectedPageOrder() const { return -1; }

  virtual void selectPageOrder(int option) {}

  virtual std::vector<PageOrderOption> pageOrderOptions() const { return std::vector<PageOrderOption>(); }

  virtual void performRelinking(const AbstractRelinker& relinker) = 0;

  virtual void preUpdateUI(FilterUiInterface* ui, const PageInfo& pageInfo) = 0;

  virtual QDomElement saveSettings(const ProjectWriter& writer, QDomDocument& doc) const = 0;

  virtual void loadSettings(const ProjectReader& reader, const QDomElement& filtersEl) = 0;

  virtual void loadDefaultSettings(const PageInfo& pageInfo) = 0;
};


#endif  // ifndef SCANTAILOR_CORE_ABSTRACTFILTER_H_
