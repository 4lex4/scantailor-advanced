// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RelinkingSortingModel.h"
#include <cassert>
#include "RelinkingModel.h"

RelinkingSortingModel::RelinkingSortingModel(QObject* parent) : QSortFilterProxyModel(parent) {
  setDynamicSortFilter(true);
  sort(0);
}

bool RelinkingSortingModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
  const int left_status = left.data(RelinkingModel::UncommittedStatusRole).toInt();
  const int right_status = right.data(RelinkingModel::UncommittedStatusRole).toInt();
  if (left_status != right_status) {
    if (left_status == RelinkingModel::Missing) {
      return true;  // Missing items go before other ones.
    } else if (right_status == RelinkingModel::Missing) {
      return false;
    } else if (left_status == RelinkingModel::StatusUpdatePending) {
      return true;  // These go after missing ones.
    } else if (right_status == RelinkingModel::StatusUpdatePending) {
      return false;
    }
    assert(!"Unreachable");
  }

  const int left_type = left.data(RelinkingModel::TypeRole).toInt();
  const int right_type = right.data(RelinkingModel::TypeRole).toInt();
  const QString left_path(left.data(RelinkingModel::UncommittedPathRole).toString());
  const QString right_path(right.data(RelinkingModel::UncommittedPathRole).toString());

  if (left_type != right_type) {
    // Directories go above their siblings.
    const QString left_dir(left_path.left(left_path.lastIndexOf(QChar('/'))));
    const QString right_dir(right_path.left(right_path.lastIndexOf(QChar('/'))));
    if (left_dir == right_dir) {
      return left_type == RelinkablePath::Dir;
    }
  }

  // The last resort is lexicographical ordering.
  return left_path < right_path;
}  // RelinkingSortingModel::lessThan
