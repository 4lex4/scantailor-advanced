// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef DYNAMIC_POOL_H_
#define DYNAMIC_POOL_H_

#include <boost/intrusive/list.hpp>
#include <boost/scoped_array.hpp>
#include <cstddef>
#include "NonCopyable.h"

/**
 * \brief Allocates objects from the heap.
 *
 * There is no way of freeing the allocated objects
 * besides destroying the whole pool.
 */
template <typename T>
class DynamicPool {
  DECLARE_NON_COPYABLE(DynamicPool)

 public:
  DynamicPool() {}

  ~DynamicPool();

  /**
   * \brief Allocates a sequence of objects.
   *
   * If T is a POD type, the returned objects are uninitialized,
   * otherwise they are default-constructed.
   */
  T* alloc(size_t num_elements);

 private:
  enum { OVERALLOCATION_FACTOR = 3 };

  /**< Allocate 3 times the requested size. */
  enum { OVERALLOCATION_LIMIT = 256 };

  /**< Don't overallocate too much. */

  struct Chunk : public boost::intrusive::list_base_hook<> {
    boost::scoped_array<T> storage;
    T* pData;
    size_t remainingElements;

    Chunk() : pData(0), remainingElements(0) {}

    void init(boost::scoped_array<T>& data, size_t size) {
      data.swap(storage);
      pData = storage.get();
      remainingElements = size;
    }
  };

  struct DeleteDisposer {
    void operator()(Chunk* chunk) { delete chunk; }
  };

  typedef boost::intrusive::list<Chunk, boost::intrusive::constant_time_size<false>> ChunkList;

  static size_t adviseChunkSize(size_t num_elements);

  ChunkList m_chunkList;
};


template <typename T>
DynamicPool<T>::~DynamicPool() {
  m_chunkList.clear_and_dispose(DeleteDisposer());
}

template <typename T>
T* DynamicPool<T>::alloc(size_t num_elements) {
  Chunk* chunk = 0;

  if (!m_chunkList.empty()) {
    chunk = &m_chunkList.back();
    if (chunk->remainingElements < num_elements) {
      chunk = 0;
    }
  }

  if (!chunk) {
    // Create a new chunk.
    const size_t chunk_size = adviseChunkSize(num_elements);
    boost::scoped_array<T> data(new T[chunk_size]);
    chunk = &*m_chunkList.insert(m_chunkList.end(), *new Chunk);
    chunk->init(data, chunk_size);
  }

  // Allocate from chunk.
  T* data = chunk->pData;
  chunk->pData += num_elements;
  chunk->remainingElements -= num_elements;

  return data;
}

template <typename T>
size_t DynamicPool<T>::adviseChunkSize(size_t num_elements) {
  size_t factor = OVERALLOCATION_LIMIT / num_elements;
  if (factor > (size_t) OVERALLOCATION_FACTOR) {
    factor = OVERALLOCATION_FACTOR;
  }

  return num_elements * (factor + 1);
}

#endif  // ifndef DYNAMIC_POOL_H_
