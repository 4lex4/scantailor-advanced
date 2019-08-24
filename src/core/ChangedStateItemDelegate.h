// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_CHANGEDSTATEITEMDELEGATE_H_
#define SCANTAILOR_CORE_CHANGEDSTATEITEMDELEGATE_H_

#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

/**
 * \brief A decoration of an existing item delegate
 *        that forces certain item states.
 */
template <typename T = QStyledItemDelegate>
class ChangedStateItemDelegate : public T {
 public:
  explicit ChangedStateItemDelegate(QObject* parent = nullptr) : T(parent), m_changedFlags(), m_changedMask() {}

  void flagsForceEnabled(QStyle::State flags) {
    m_changedFlags |= flags;
    m_changedMask |= flags;
  }

  void flagsForceDisabled(QStyle::State flags) {
    m_changedFlags &= ~flags;
    m_changedMask |= flags;
  }

  void removeChanges(QStyle::State remove) { m_changedMask &= ~remove; }

  void removeAllChanges() { m_changedMask = QStyle::State(); }

  virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    const QStyle::State origState = option.state;

    const QStyle::State newState = (origState & ~m_changedMask) | (m_changedFlags & m_changedMask);

    // Evil but necessary: the alternative solution of modifying
    // a copy doesn't work, as option doesn't really point to
    // QStyleOptionViewItem, but to one of its subclasses.
    QStyleOptionViewItem& nonConstOpt = const_cast<QStyleOptionViewItem&>(option);

    nonConstOpt.state = newState;
    T::paint(painter, nonConstOpt, index);
    nonConstOpt.state = origState;
  }

 private:
  QStyle::State m_changedFlags;
  QStyle::State m_changedMask;
};


#endif  // ifndef SCANTAILOR_CORE_CHANGEDSTATEITEMDELEGATE_H_
