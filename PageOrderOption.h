/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef PAGE_ORDER_OPTION_H_
#define PAGE_ORDER_OPTION_H_

#include <QString>
#include "PageOrderProvider.h"
#include "intrusive_ptr.h"

class PageOrderOption {
  // Member-wise copying is OK.
 public:
  typedef intrusive_ptr<const PageOrderProvider> ProviderPtr;

  PageOrderOption(const QString& name, ProviderPtr provider) : m_name(name), m_ptrProvider(std::move(provider)) {}

  const QString& name() const { return m_name; }

  /**
   * Returns the ordering information provider.
   * A null provider is OK and is to be interpreted as default order.
   */
  const ProviderPtr& provider() const { return m_ptrProvider; }

 private:
  QString m_name;
  ProviderPtr m_ptrProvider;
};


#endif  // ifndef PAGE_ORDER_OPTION_H_
