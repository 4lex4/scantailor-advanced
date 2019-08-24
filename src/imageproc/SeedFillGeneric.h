// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_SEEDFILLGENERIC_H_
#define SCANTAILOR_IMAGEPROC_SEEDFILLGENERIC_H_

#include <QSize>
#include <cassert>
#include <vector>
#include "BinaryImage.h"
#include "Connectivity.h"
#include "FastQueue.h"

namespace imageproc {
namespace detail {
namespace seed_fill_generic {
struct HTransition {
  int westDelta;  // -1 or 0
  int eastDelta;  // 1 or 0

  HTransition(int westDelta_, int eastDelta_) : westDelta(westDelta_), eastDelta(eastDelta_) {}
};

struct VTransition {
  int northMask;  // 0 or ~0
  int southMask;  // 0 or ~0

  VTransition(int northMask_, int southMask_) : northMask(northMask_), southMask(southMask_) {}
};

template <typename T>
struct Position {
  T* seed;
  const T* mask;
  int x;
  int y;

  Position(T* seed_, const T* mask_, int x_, int y_) : seed(seed_), mask(mask_), x(x_), y(y_) {}
};

void initHorTransitions(std::vector<HTransition>& transitions, int width);

void initVertTransitions(std::vector<VTransition>& transitions, int height);

template <typename T, typename SpreadOp, typename MaskOp>
void seedFillSingleLine(SpreadOp spreadOp,
                        MaskOp maskOp,
                        const int lineLen,
                        T* seed,
                        const int seedStride,
                        const T* mask,
                        const int maskStride) {
  if (lineLen == 0) {
    return;
  }

  *seed = maskOp(*seed, *mask);

  for (int i = 1; i < lineLen; ++i) {
    seed += seedStride;
    mask += maskStride;
    *seed = maskOp(*mask, spreadOp(*seed, seed[-seedStride]));
  }

  for (int i = 1; i < lineLen; ++i) {
    seed -= seedStride;
    mask -= maskStride;
    *seed = maskOp(*mask, spreadOp(*seed, seed[seedStride]));
  }
}

template <typename T, typename SpreadOp, typename MaskOp>
inline void processNeighbor(SpreadOp spreadOp,
                            MaskOp maskOp,
                            FastQueue<Position<T>>& queue,
                            uint32_t* inQueueLine,
                            const T thisVal,
                            T* const neighbor,
                            const T* const neighborMask,
                            const Position<T>& basePos,
                            const int xDelta,
                            const int yDelta) {
  const T newVal(maskOp(*neighborMask, spreadOp(thisVal, *neighbor)));
  if (newVal != *neighbor) {
    *neighbor = newVal;
    const int x = basePos.x + xDelta;
    const int y = basePos.y + yDelta;
    uint32_t& inQueueWord = inQueueLine[x >> 5];
    const uint32_t inQueueMask = (uint32_t(1) << 31) >> (x & 31);
    if (!(inQueueWord & inQueueMask)) {  // If not already in the queue.
      queue.push(Position<T>(neighbor, neighborMask, x, y));
      inQueueWord |= inQueueMask;  // Mark as already in the queue.
    }
  }
}

template <typename T, typename SpreadOp, typename MaskOp>
void spread4(SpreadOp spreadOp,
             MaskOp maskOp,
             FastQueue<Position<T>>& queue,
             uint32_t* const inQueueData,
             const int inQueueStride,
             const HTransition* hTransitions,
             const VTransition* vTransitions,
             const int seedStride,
             const int maskStride) {
  while (!queue.empty()) {
    const Position<T> pos(queue.front());
    queue.pop();

    const T thisVal(*pos.seed);
    const HTransition ht(hTransitions[pos.x]);
    const VTransition vt(vTransitions[pos.y]);
    uint32_t* const inQueueLine = inQueueData + inQueueStride * pos.y;
    T* seed;
    const T* mask;

    // Western neighbor.
    seed = pos.seed + ht.westDelta;
    mask = pos.mask + ht.westDelta;
    processNeighbor(spreadOp, maskOp, queue, inQueueLine, thisVal, seed, mask, pos, ht.westDelta, 0);

    // Eastern neighbor.
    seed = pos.seed + ht.eastDelta;
    mask = pos.mask + ht.eastDelta;
    processNeighbor(spreadOp, maskOp, queue, inQueueLine, thisVal, seed, mask, pos, ht.eastDelta, 0);

    // Northern neighbor.
    seed = pos.seed - (seedStride & vt.northMask);
    mask = pos.mask - (maskStride & vt.northMask);
    processNeighbor(spreadOp, maskOp, queue, inQueueLine - (inQueueStride & vt.northMask), thisVal, seed, mask, pos, 0,
                    -1 & vt.northMask);

    // Southern neighbor.
    seed = pos.seed + (seedStride & vt.southMask);
    mask = pos.mask + (maskStride & vt.southMask);
    processNeighbor(spreadOp, maskOp, queue, inQueueLine + (inQueueStride & vt.southMask), thisVal, seed, mask, pos, 0,
                    1 & vt.southMask);
  }
}  // spread4

template <typename T, typename SpreadOp, typename MaskOp>
void spread8(SpreadOp spreadOp,
             MaskOp maskOp,
             FastQueue<Position<T>>& queue,
             uint32_t* const inQueueData,
             const int inQueueStride,
             const HTransition* hTransitions,
             const VTransition* vTransitions,
             const int seedStride,
             const int maskStride) {
  while (!queue.empty()) {
    const Position<T> pos(queue.front());
    queue.pop();

    const T thisVal(*pos.seed);
    const HTransition ht(hTransitions[pos.x]);
    const VTransition vt(vTransitions[pos.y]);
    uint32_t* const inQueueLine = inQueueData + inQueueStride * pos.y;
    T* seed;
    const T* mask;

    // Northern neighbor.
    seed = pos.seed - (seedStride & vt.northMask);
    mask = pos.mask - (maskStride & vt.northMask);
    processNeighbor(spreadOp, maskOp, queue, inQueueLine - (inQueueStride & vt.northMask), thisVal, seed, mask, pos, 0,
                    -1 & vt.northMask);

    // North-Western neighbor.
    seed = pos.seed - (seedStride & vt.northMask) + ht.westDelta;
    mask = pos.mask - (maskStride & vt.northMask) + ht.westDelta;
    processNeighbor(spreadOp, maskOp, queue, inQueueLine - (inQueueStride & vt.northMask), thisVal, seed, mask, pos,
                    ht.westDelta, -1 & vt.northMask);

    // North-Eastern neighbor.
    seed = pos.seed - (seedStride & vt.northMask) + ht.eastDelta;
    mask = pos.mask - (maskStride & vt.northMask) + ht.eastDelta;
    processNeighbor(spreadOp, maskOp, queue, inQueueLine - (inQueueStride & vt.northMask), thisVal, seed, mask, pos,
                    ht.eastDelta, -1 & vt.northMask);

    // Eastern neighbor.
    seed = pos.seed + ht.eastDelta;
    mask = pos.mask + ht.eastDelta;
    processNeighbor(spreadOp, maskOp, queue, inQueueLine, thisVal, seed, mask, pos, ht.eastDelta, 0);

    // Western neighbor.
    seed = pos.seed + ht.westDelta;
    mask = pos.mask + ht.westDelta;
    processNeighbor(spreadOp, maskOp, queue, inQueueLine, thisVal, seed, mask, pos, ht.westDelta, 0);

    // Southern neighbor.
    seed = pos.seed + (seedStride & vt.southMask);
    mask = pos.mask + (maskStride & vt.southMask);
    processNeighbor(spreadOp, maskOp, queue, inQueueLine + (inQueueStride & vt.southMask), thisVal, seed, mask, pos, 0,
                    1 & vt.southMask);

    // South-Eastern neighbor.
    seed = pos.seed + (seedStride & vt.southMask) + ht.eastDelta;
    mask = pos.mask + (maskStride & vt.southMask) + ht.eastDelta;
    processNeighbor(spreadOp, maskOp, queue, inQueueLine + (inQueueStride & vt.southMask), thisVal, seed, mask, pos,
                    ht.eastDelta, 1 & vt.southMask);

    // South-Western neighbor.
    seed = pos.seed + (seedStride & vt.southMask) + ht.westDelta;
    mask = pos.mask + (seedStride & vt.southMask) + ht.westDelta;
    processNeighbor(spreadOp, maskOp, queue, inQueueLine + (inQueueStride & vt.southMask), thisVal, seed, mask, pos,
                    ht.westDelta, 1 & vt.southMask);
  }
}  // spread8

template <typename T, typename SpreadOp, typename MaskOp>
void seedFill4(SpreadOp spreadOp,
               MaskOp maskOp,
               T* const seed,
               const int seedStride,
               const QSize size,
               const T* const mask,
               const int maskStride) {
  const int w = size.width();
  const int h = size.height();

  T* seedLine = seed;
  const T* maskLine = mask;
  T* prevLine = seedLine;

  // Top to bottom.
  for (int y = 0; y < h; ++y) {
    int x = 0;

    // First item in line.
    T prev(maskOp(maskLine[x], spreadOp(seedLine[x], prevLine[x])));
    seedLine[x] = prev;

    // Other items, left to right.
    while (++x < w) {
      prev = maskOp(maskLine[x], spreadOp(prev, spreadOp(seedLine[x], prevLine[x])));
      seedLine[x] = prev;
    }

    prevLine = seedLine;
    seedLine += seedStride;
    maskLine += maskStride;
  }

  seedLine -= seedStride;
  maskLine -= maskStride;

  FastQueue<Position<T>> queue;
  BinaryImage inQueue(size, WHITE);
  uint32_t* const inQueueData = inQueue.data();
  const int inQueueStride = inQueue.wordsPerLine();
  std::vector<HTransition> hTransitions;
  std::vector<VTransition> vTransitions;
  initHorTransitions(hTransitions, w);
  initVertTransitions(vTransitions, h);

  // Bottom to top.
  uint32_t* inQueueLine = inQueueData + inQueueStride * (h - 1);
  for (int y = h - 1; y >= 0; --y) {
    const VTransition vt(vTransitions[y]);

    // Right to left.
    for (int x = w - 1; x >= 0; --x) {
      const HTransition ht(hTransitions[x]);

      T* const pBaseSeed = seedLine + x;
      const T* const pBaseMask = maskLine + x;

      T* const pEastSeed = pBaseSeed + ht.eastDelta;
      T* const pSouthSeed = pBaseSeed + (seedStride & vt.southMask);

      const T newVal(maskOp(*pBaseMask, spreadOp(*pBaseSeed, spreadOp(*pEastSeed, *pSouthSeed))));
      if (newVal == *pBaseSeed) {
        continue;
      }

      *pBaseSeed = newVal;

      const Position<T> pos(pBaseSeed, pBaseMask, x, y);
      const T* pEastMask = pBaseMask + ht.eastDelta;
      const T* pSouthMask = pBaseMask + (maskStride & vt.southMask);

      // Eastern neighbor.
      processNeighbor(spreadOp, maskOp, queue, inQueueLine, newVal, pEastSeed, pEastMask, pos, ht.eastDelta, 0);

      // Southern neighbor.
      processNeighbor(spreadOp, maskOp, queue, inQueueLine + (inQueueStride & vt.southMask), newVal, pSouthSeed,
                      pSouthMask, pos, 0, 1 & vt.southMask);
    }

    seedLine -= seedStride;
    maskLine -= maskStride;
    inQueueLine -= inQueueStride;
  }

  spread4(spreadOp, maskOp, queue, inQueueData, inQueueStride, &hTransitions[0], &vTransitions[0], seedStride,
          maskStride);
}  // seedFill4

template <typename T, typename SpreadOp, typename MaskOp>
void seedFill8(SpreadOp spreadOp,
               MaskOp maskOp,
               T* const seed,
               const int seedStride,
               const QSize size,
               const T* const mask,
               const int maskStride) {
  const int w = size.width();
  const int h = size.height();

  // Some code below doesn't handle such cases.
  if (w == 1) {
    seedFillSingleLine(spreadOp, maskOp, h, seed, seedStride, mask, maskStride);
    return;
  } else if (h == 1) {
    seedFillSingleLine(spreadOp, maskOp, w, seed, 1, mask, 1);
    return;
  }

  T* seedLine = seed;
  const T* maskLine = mask;

  // Note: we usually process the first line by assigning
  // prevLine = seedLine, but in this case prevLine[x + 1]
  // won't be clipped by its mask when we use it to update seedLine[x].
  // The wrong value may propagate further from there, so clipping
  // we do on the anti-raster pass won't help.
  // That's why we process the first line separately.
  seedLine[0] = maskOp(seedLine[0], maskLine[0]);
  for (int x = 1; x < w; ++x) {
    seedLine[x] = maskOp(maskLine[x], spreadOp(seedLine[x], seedLine[x - 1]));
  }

  T* prevLine = seedLine;

  // Top to bottom.
  for (int y = 1; y < h; ++y) {
    seedLine += seedStride;
    maskLine += maskStride;

    int x = 0;

    // Leftmost pixel.
    seedLine[x] = maskOp(maskLine[x], spreadOp(seedLine[x], spreadOp(prevLine[x], prevLine[x + 1])));

    // Left to right.
    while (++x < w - 1) {
      seedLine[x]
          = maskOp(maskLine[x],
                   spreadOp(spreadOp(spreadOp(seedLine[x], seedLine[x - 1]), spreadOp(prevLine[x], prevLine[x - 1])),
                            prevLine[x + 1]));
    }

    // Rightmost pixel.
    seedLine[x]
        = maskOp(maskLine[x], spreadOp(spreadOp(seedLine[x], seedLine[x - 1]), spreadOp(prevLine[x], prevLine[x - 1])));

    prevLine = seedLine;
  }

  FastQueue<Position<T>> queue;
  BinaryImage inQueue(size, WHITE);
  uint32_t* const inQueueData = inQueue.data();
  const int inQueueStride = inQueue.wordsPerLine();
  std::vector<HTransition> hTransitions;
  std::vector<VTransition> vTransitions;
  initHorTransitions(hTransitions, w);
  initVertTransitions(vTransitions, h);

  // Bottom to top.
  uint32_t* inQueueLine = inQueueData + inQueueStride * (h - 1);
  for (int y = h - 1; y >= 0; --y) {
    const VTransition vt(vTransitions[y]);

    for (int x = w - 1; x >= 0; --x) {
      const HTransition ht(hTransitions[x]);

      T* const pBaseSeed = seedLine + x;
      const T* const pBaseMask = maskLine + x;

      T* const pEastSeed = pBaseSeed + ht.eastDelta;
      T* const pSouthSeed = pBaseSeed + (seedStride & vt.southMask);
      T* const pSouthWestSeed = pSouthSeed + ht.westDelta;
      T* const pSouthEastSeed = pSouthSeed + ht.eastDelta;

      const T newVal = maskOp(*pBaseMask, spreadOp(*pBaseSeed, spreadOp(spreadOp(*pEastSeed, *pSouthEastSeed),
                                                                        spreadOp(*pSouthSeed, *pSouthWestSeed))));
      if (newVal == *pBaseSeed) {
        continue;
      }

      *pBaseSeed = newVal;

      const Position<T> pos(pBaseSeed, pBaseMask, x, y);
      const T* pEastMask = pBaseMask + ht.eastDelta;
      const T* pSouthMask = pBaseMask + (maskStride & vt.southMask);
      const T* pSouthWestMask = pSouthMask + ht.westDelta;
      const T* pSouthEastMask = pSouthMask + ht.eastDelta;

      // Eastern neighbor.
      processNeighbor(spreadOp, maskOp, queue, inQueueLine, newVal, pEastSeed, pEastMask, pos, ht.eastDelta, 0);

      // South-eastern neighbor.
      processNeighbor(spreadOp, maskOp, queue, inQueueLine + (inQueueStride & vt.southMask), newVal, pSouthEastSeed,
                      pSouthEastMask, pos, ht.eastDelta, 1 & vt.southMask);

      // Southern neighbor.
      processNeighbor(spreadOp, maskOp, queue, inQueueLine + (inQueueStride & vt.southMask), newVal, pSouthSeed,
                      pSouthMask, pos, 0, 1 & vt.southMask);

      // South-western neighbor.
      processNeighbor(spreadOp, maskOp, queue, inQueueLine + (inQueueStride & vt.southMask), newVal, pSouthWestSeed,
                      pSouthWestMask, pos, ht.westDelta, 1 & vt.southMask);
    }

    seedLine -= seedStride;
    maskLine -= maskStride;
    inQueueLine -= inQueueStride;
  }

  spread8(spreadOp, maskOp, queue, inQueueData, inQueueStride, &hTransitions[0], &vTransitions[0], seedStride,
          maskStride);
}  // seedFill8
}  // namespace seed_fill_generic
}  // namespace detail


/**
 * The following pseudocode illustrates the principle of a seed-fill algorithm:
 * [code]
 * do {
 *   foreach (<point at x, y>) {
 *     val = maskOp(mask[x, y], seed[x, y]);
 *     foreach (<neighbor at nx, ny>) {
 *       seed[nx, ny] = maskOp(mask[nx, ny], spreadOp(seed[nx, ny], val));
 *     }
 *   }
 * } while (<changes to seed were made on this iteration>);
 * [/code]
 *
 * \param spreadOp A functor or a pointer to a free function that can be called with
 *        two arguments of type T and return the bigger or the smaller of the two.
 * \param maskOp Same as spreadOp, but the opposite operation.
 * \param conn Determines whether to spread values to 4 or 8 eight immediate neighbors.
 * \param[in,out] seed Pointer to the seed buffer.
 * \param seedStride The size of a row in the seed buffer, in terms of the number of T objects.
 * \param size Dimensions of the seed and the mask buffers.
 * \param mask Pointer to the mask data.
 * \param maskStride The size of a row in the mask buffer, in terms of the number of T objects.
 *
 * This code is an implementation of the hybrid grayscale restoration algorithm described in:
 * Morphological Grayscale Reconstruction in Image Analysis:
 * Applications and Efficient Algorithms, technical report 91-16, Harvard Robotics Laboratory,
 * November 1991, IEEE Transactions on Image Processing, Vol. 2, No. 2, pp. 176-201, April 1993.\n
 */
template <typename T, typename SpreadOp, typename MaskOp>
void seedFillGenericInPlace(SpreadOp spreadOp,
                            MaskOp maskOp,
                            Connectivity conn,
                            T* seed,
                            int seedStride,
                            QSize size,
                            const T* mask,
                            int maskStride) {
  if (size.isEmpty()) {
    return;
  }

  if (conn == CONN4) {
    detail::seed_fill_generic::seedFill4(spreadOp, maskOp, seed, seedStride, size, mask, maskStride);
  } else {
    assert(conn == CONN8);
    detail::seed_fill_generic::seedFill8(spreadOp, maskOp, seed, seedStride, size, mask, maskStride);
  }
}
}  // namespace imageproc

#endif  // ifndef SCANTAILOR_IMAGEPROC_SEEDFILLGENERIC_H_
