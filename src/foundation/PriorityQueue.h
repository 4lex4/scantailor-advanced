// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_PRIORITYQUEUE_H_
#define SCANTAILOR_FOUNDATION_PRIORITYQUEUE_H_

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

/**
 * \brief A priority queue implemented as a binary heap.
 *
 * \tparam T The type of objects to be stored in the priority queue.
 * \param SubClass A sub class of this class that will be implementing the following:
 *        \li void setIndex(T& obj, size_t heap_idx);
 *        \li bool higherThan(const T& lhs, const T& rhs) const;
 *
 * Also note that this implementation will benefit from a standalone
 * \code
 * void swap(T& o1, T& o2);
 * \endcode
 * function in the same namespace as T.
 */
template <typename T, typename SubClass>
class PriorityQueue {
  // Member-wise copying is OK.
 public:
  PriorityQueue() {}

  void reserve(size_t capacity) { m_index.reserve(capacity); }

  bool empty() const { return m_index.empty(); }

  size_t size() const { return m_index.size(); }

  /**
   * \brief Provides access to the head of priority queue.
   *
   * Modification of an object is allowed, provided your modifications don't
   * affect the logical order of objects, or you will be calling reposition(),
   * pop() or erase() on the modified object before any other operation that
   * involves comparing objects.
   */
  T& front() { return m_index.front(); }

  const T& front() const { return m_index.front(); }

  void push(const T& obj);

  /**
   * Like push(), but implemented through swapping \p obj with a default
   * constructed instance of T. This will make sence if copying a default
   * constructed instance of T is much cheaper than copying \p obj.
   */
  void pushDestructive(T& obj);

  void pop();

  /**
   * Retrieve-and-pop, implemented through swapping \p obj with the instance
   * at the front of the queue. There are no special requirements to
   * the state of the object being passed to this function.
   */
  void retrieveFront(T& obj);

  void swapWith(PriorityQueue& other) { m_index.swap(other.m_index); }

 protected:
  void erase(size_t idx);

  void reposition(size_t idx);

 private:
  static size_t parent(size_t idx) { return (idx - 1) / 2; }

  static size_t left(size_t idx) { return idx * 2 + 1; }

  static size_t right(size_t idx) { return idx * 2 + 2; }

  SubClass* subClass() { return static_cast<SubClass*>(this); }

  const SubClass* subClass() const { return static_cast<SubClass*>(this); }

  size_t bubbleUp(size_t idx);

  size_t bubbleDown(size_t idx);

  std::vector<T> m_index;
};


template <typename T, typename SubClass>
inline void swap(PriorityQueue<T, SubClass>& o1, PriorityQueue<T, SubClass>& o2) {
  o1.swap(o2);
}

template <typename T, typename SubClass>
void PriorityQueue<T, SubClass>::push(const T& obj) {
  const size_t idx = m_index.size();
  m_index.push_back(obj);
  subClass()->setIndex(m_index.back(), idx);
  bubbleUp(idx);
}

template <typename T, typename SubClass>
void PriorityQueue<T, SubClass>::pushDestructive(T& obj) {
  using namespace std;

  const size_t idx = m_index.size();
  m_index.push_back(T());
  swap(m_index.back(), obj);
  subClass()->setIndex(m_index.back(), idx);
  bubbleUp(idx);
}

template <typename T, typename SubClass>
void PriorityQueue<T, SubClass>::pop() {
  using namespace std;

  assert(!empty());

  swap(m_index.front(), m_index.back());
  subClass()->setIndex(m_index.front(), 0);

  m_index.pop_back();
  if (!empty()) {
    bubbleDown(0);
  }
}

template <typename T, typename SubClass>
void PriorityQueue<T, SubClass>::retrieveFront(T& obj) {
  using namespace std;

  assert(!empty());

  swap(m_index.front(), obj);
  swap(m_index.front(), m_index.back());
  subClass()->setIndex(m_index.front(), 0);

  m_index.pop_back();
  if (!empty()) {
    bubbleDown(0);
  }
}

template <typename T, typename SubClass>
void PriorityQueue<T, SubClass>::erase(const size_t idx) {
  using namespace std;

  swap(m_index[idx], m_index.back());
  subClass()->setIndex(m_index[idx], idx);

  m_index.pop_back();
  reposition(m_index[idx]);
}

template <typename T, typename SubClass>
void PriorityQueue<T, SubClass>::reposition(const size_t idx) {
  bubbleUp(bubbleDown(idx));
}

template <typename T, typename SubClass>
size_t PriorityQueue<T, SubClass>::bubbleUp(size_t idx) {
  using namespace std;

  // Iteratively swap the element with its parent,
  // if it's greater than the parent.

  assert(idx < m_index.size());

  while (idx > 0) {
    const size_t parentIdx = parent(idx);
    if (!subClass()->higherThan(m_index[idx], m_index[parentIdx])) {
      break;
    }
    swap(m_index[idx], m_index[parentIdx]);
    subClass()->setIndex(m_index[idx], idx);
    subClass()->setIndex(m_index[parentIdx], parentIdx);
    idx = parentIdx;
  }

  return idx;
}

template <typename T, typename SubClass>
size_t PriorityQueue<T, SubClass>::bubbleDown(size_t idx) {
  using namespace std;

  const size_t len = m_index.size();
  assert(idx < len);

  // While any child is greater than the element itself,
  // swap it with the greatest child.

  while (true) {
    const size_t lft = left(idx);
    const size_t rgt = right(idx);
    size_t bestChild;

    if (rgt < len) {
      bestChild = subClass()->higherThan(m_index[lft], m_index[rgt]) ? lft : rgt;
    } else if (lft < len) {
      bestChild = lft;
    } else {
      break;
    }

    if (subClass()->higherThan(m_index[bestChild], m_index[idx])) {
      swap(m_index[idx], m_index[bestChild]);
      subClass()->setIndex(m_index[idx], idx);
      subClass()->setIndex(m_index[bestChild], bestChild);
      idx = bestChild;
    } else {
      break;
    }
  }

  return idx;
}  // >::bubbleDown

#endif  // ifndef SCANTAILOR_FOUNDATION_PRIORITYQUEUE_H_
