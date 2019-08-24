// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_FASTQUEUE_H_
#define SCANTAILOR_FOUNDATION_FASTQUEUE_H_

#include <boost/foreach.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include "NonCopyable.h"

template <typename T>
class FastQueue {
 public:
  FastQueue() : m_chunkCapacity(defaultChunkCapacity()) {}

  FastQueue(const FastQueue& other);

  ~FastQueue() { m_chunkList.clear_and_dispose(ChunkDisposer()); }

  FastQueue& operator=(const FastQueue& other);

  bool empty() const { return m_chunkList.empty(); }

  T& front() { return *m_chunkList.front().pBegin; }

  const T& front() const { return *m_chunkList.front().pBegin; }

  void push(const T& t);

  void pop();

  void swap(FastQueue& other);

 private:
  struct Chunk : public boost::intrusive::list_base_hook<> {
    DECLARE_NON_COPYABLE(Chunk)

   public:
    explicit Chunk(size_t capacity) {
      const uintptr_t p = (uintptr_t)(this + 1);
      const size_t alignment = boost::alignment_of<T>::value;
      pBegin = (T*) (((p + alignment - 1) / alignment) * alignment);
      pEnd = pBegin;
      pBufferEnd = pBegin + capacity;
      assert(size_t((char*) pBufferEnd - (char*) this) <= storageRequirement(capacity));
    }

    ~Chunk() {
      for (; pBegin != pEnd; ++pBegin) {
        pBegin->~T();
      }
    }

    static size_t storageRequirement(size_t capacity) {
      return sizeof(Chunk) + boost::alignment_of<T>::value - 1 + capacity * sizeof(T);
    }

    T* pBegin;
    T* pEnd;
    T* pBufferEnd;
    // An implicit array of T follows.
  };

  struct ChunkDisposer {
    void operator()(Chunk* chunk) {
      chunk->~Chunk();
      delete[](char*) chunk;
    }
  };

  using ChunkList = boost::intrusive::list<Chunk, boost::intrusive::constant_time_size<false>>;

  static size_t defaultChunkCapacity() { return (sizeof(T) >= 4096) ? 1 : 4096 / sizeof(T); }

  ChunkList m_chunkList;
  size_t m_chunkCapacity;
};


template <typename T>
FastQueue<T>::FastQueue(const FastQueue& other) : m_chunkCapacity(other.m_chunkCapacity) {
  for (Chunk& chunk : other.m_chunkList) {
    for (const T* obj = chunk->pBegin; obj != chunk->pEnd; ++obj) {
      push(*obj);
    }
  }
}

template <typename T>
FastQueue<T>& FastQueue<T>::operator=(const FastQueue& other) {
  FastQueue(other).swap(*this);

  return *this;
}

template <typename T>
void FastQueue<T>::push(const T& t) {
  Chunk* chunk = 0;

  if (!m_chunkList.empty()) {
    chunk = &m_chunkList.back();
    if (chunk->pEnd == chunk->pBufferEnd) {
      chunk = 0;
    }
  }

  if (!chunk) {
    // Create a new chunk.
    char* buf = new char[Chunk::storageRequirement(m_chunkCapacity)];
    chunk = new (buf) Chunk(m_chunkCapacity);
    m_chunkList.push_back(*chunk);
  }
  // Push to chunk.
  new (chunk->pEnd) T(t);
  ++chunk->pEnd;
}

template <typename T>
void FastQueue<T>::pop() {
  assert(!empty());

  Chunk* chunk = &m_chunkList.front();
  chunk->pBegin->~T();
  ++chunk->pBegin;
  if (chunk->pBegin == chunk->pEnd) {
    m_chunkList.pop_front();
    ChunkDisposer()(chunk);
  }
}

template <typename T>
void FastQueue<T>::swap(FastQueue& other) {
  m_chunkList.swap(other.m_chunkList);
  const size_t tmp = m_chunkCapacity;
  m_chunkCapacity = other.m_chunkCapacity;
  other.m_chunkCapacity = tmp;
}

template <typename T>
inline void swap(FastQueue<T>& o1, FastQueue<T>& o2) {
  o1.swap(o2);
}

#endif  // ifndef SCANTAILOR_FOUNDATION_FASTQUEUE_H_
