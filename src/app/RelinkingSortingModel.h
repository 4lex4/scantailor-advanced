// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_RELINKINGSORTINGMODEL_H_
#define SCANTAILOR_APP_RELINKINGSORTINGMODEL_H_

#include <QSortFilterProxyModel>

class RelinkingSortingModel : public QSortFilterProxyModel {
 public:
  explicit RelinkingSortingModel(QObject* parent = nullptr);

 protected:
  bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
};


#endif
