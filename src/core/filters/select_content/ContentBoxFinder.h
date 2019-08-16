// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SELECT_CONTENT_CONTENTBOXFINDER_H_
#define SELECT_CONTENT_CONTENTBOXFINDER_H_

#include <BinaryThreshold.h>

class TaskStatus;
class DebugImages;
class FilterData;
class QImage;
class QRect;
class QRectF;

namespace imageproc {
class BinaryImage;
class ConnComp;

class SEDM;
}  // namespace imageproc

namespace select_content {
class ContentBoxFinder {
 public:
  static QRectF findContentBox(const TaskStatus& status,
                               const FilterData& data,
                               const QRectF& page_rect,
                               DebugImages* dbg = nullptr);

 private:
  class Garbage;

  static void segmentGarbage(const imageproc::BinaryImage& garbage,
                             imageproc::BinaryImage& hor_garbage,
                             imageproc::BinaryImage& vert_garbage,
                             DebugImages* dbg);

  static void trimContentBlocksInPlace(const imageproc::BinaryImage& content, imageproc::BinaryImage& content_blocks);

  static void inPlaceRemoveAreasTouchingBorders(imageproc::BinaryImage& content_blocks, DebugImages* dbg);

  static imageproc::BinaryImage estimateTextMask(const imageproc::BinaryImage& content,
                                                 const imageproc::BinaryImage& content_blocks,
                                                 DebugImages* dbg);

  static void filterShadows(const TaskStatus& status, imageproc::BinaryImage& shadows, DebugImages* dbg);

  static QRect trimLeft(const imageproc::BinaryImage& content,
                        const imageproc::BinaryImage& content_blocks,
                        const imageproc::BinaryImage& text_mask,
                        const QRect& area,
                        Garbage& garbage,
                        DebugImages* dbg);

  static QRect trimRight(const imageproc::BinaryImage& content,
                         const imageproc::BinaryImage& content_blocks,
                         const imageproc::BinaryImage& text_mask,
                         const QRect& area,
                         Garbage& garbage,
                         DebugImages* dbg);

  static QRect trimTop(const imageproc::BinaryImage& content,
                       const imageproc::BinaryImage& content_blocks,
                       const imageproc::BinaryImage& text_mask,
                       const QRect& area,
                       Garbage& garbage,
                       DebugImages* dbg);

  static QRect trimBottom(const imageproc::BinaryImage& content,
                          const imageproc::BinaryImage& content_blocks,
                          const imageproc::BinaryImage& text_mask,
                          const QRect& area,
                          Garbage& garbage,
                          DebugImages* dbg);

  static QRect trim(const imageproc::BinaryImage& content,
                    const imageproc::BinaryImage& content_blocks,
                    const imageproc::BinaryImage& text_mask,
                    const QRect& area,
                    const QRect& new_area,
                    const QRect& removed_area,
                    Garbage& garbage,
                    bool& can_retry_grouped,
                    DebugImages* dbg);
};
}  // namespace select_content
#endif  // ifndef SELECT_CONTENT_CONTENTBOXFINDER_H_
