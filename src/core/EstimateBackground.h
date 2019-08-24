// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_ESTIMATEBACKGROUND_H_
#define SCANTAILOR_CORE_ESTIMATEBACKGROUND_H_

class ImageTransformation;
class TaskStatus;
class DebugImages;
class QPolygonF;

namespace imageproc {
class PolynomialSurface;
class GrayImage;
}  // namespace imageproc

/**
 * \brief Estimates a grayscale background of a scanned page.
 *
 * \param input The image of a page.
 * \param areaToConsider The area in \p input image coordinates to consider.
 *        The resulting surface will only be valid in that area.
 *        This parameter can be an empty polygon, in which case all of the
 *        \p input image area is considered.
 * \param status The status of a task.  If it's cancelled by another thread,
 *        this function may throw an implementation-defined exception.
 * \param dbg The sink for intermediate images used for debugging purposes.
 *        This argument is optional.
 *
 * This implementation can deal with very complex cases, like a page with
 * a picture covering most of it, but in return, it expects some conditions
 * to be met:
 * -# The orientation must be correct.  To be precise, it can deal with
 *    a more or less vertical folding line, but not a horizontal one.
 * -# The page must have some blank margins.  The margins must be of
 *    a natural origin, or to be precise, there must not be a noticeable
 *    transition between the page background and the margins.
 * -# It's better to provide a single page to this function, not a
 *    two page scan.  When cutting off one of the pages, feel free
 *    to fill areas near the the edges with black.
 * -# This implementation can handle dark surroundings around the page,
 *    provided they touch the edges, but it performs better without them.
 */
imageproc::PolynomialSurface estimateBackground(const imageproc::GrayImage& input,
                                                const QPolygonF& areaToConsider,
                                                const TaskStatus& status,
                                                DebugImages* dbg = nullptr);

#endif
