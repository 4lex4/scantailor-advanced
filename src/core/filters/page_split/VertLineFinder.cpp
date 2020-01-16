// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "VertLineFinder.h"

#include <Constants.h>
#include <GrayImage.h>
#include <GrayRasterOp.h>
#include <Grayscale.h>
#include <HoughLineDetector.h>
#include <MorphGradientDetect.h>
#include <Morphology.h>
#include <Transform.h>

#include <QDebug>
#include <QPainter>
#include <cmath>

#include "DebugImages.h"
#include "ImageTransformation.h"

namespace page_split {
using namespace imageproc;

std::vector<QLineF> VertLineFinder::findLines(const QImage& image,
                                              const ImageTransformation& xform,
                                              const int maxLines,
                                              DebugImages* dbg,
                                              GrayImage* grayDownscaled,
                                              QTransform* outToDownscaled) {
  const int dpi = 100;

  ImageTransformation xform100dpi(xform);
  xform100dpi.preScaleToDpi(Dpi(dpi, dpi));

  QRect targetRect(xform100dpi.resultingRect().toRect());
  if (targetRect.isEmpty()) {
    targetRect.setWidth(1);
    targetRect.setHeight(1);
  }

  const GrayImage gray100(transformToGray(image, xform100dpi.transform(), targetRect,
                                          OutsidePixels::assumeWeakColor(Qt::black), QSizeF(5.0, 5.0)));
  if (dbg) {
    dbg->add(gray100, "gray100");
  }

  if (grayDownscaled) {
    *grayDownscaled = gray100;
  }
  if (outToDownscaled) {
    *outToDownscaled = xform.transformBack() * xform100dpi.transform();
  }

#if 0
        GrayImage preprocessed(removeDarkVertBorders(gray100));
        if (dbg) {
            dbg->add(preprocessed, "preprocessed");
        }
#else
  // It looks like preprocessing causes more problems than it solves.
  // It can reduce the visibility of a folding line to a level where
  // it can't be detected, while it can't always fulfill its purpose of
  // removing vertical edges of a book.  Because of that, other methods
  // of dealing with them were developed, which makes preprocessing
  // obsolete.
  GrayImage preprocessed(gray100);
#endif

#if 0
        GrayImage hGradient(morphGradientDetectDarkSide(preprocessed, QSize(11, 1)));
        GrayImage vGradient(morphGradientDetectDarkSide(preprocessed, QSize(1, 11)));
        if (dbg) {
            dbg->add(hGradient, "hGradient");
            dbg->add(vGradient, "vGradient");
        }
#else
  // These are not gradients, but their difference is the same as for
  // the two gradients above.  This branch is an optimization.
  GrayImage hGradient(erodeGray(preprocessed, QSize(11, 1), 0x00));
  GrayImage vGradient(erodeGray(preprocessed, QSize(1, 11), 0x00));
#endif

  if (!dbg) {
    // We'll need it later if debugging is on.
    preprocessed = GrayImage();
  }

  grayRasterOp<GRopClippedSubtract<GRopDst, GRopSrc>>(hGradient, vGradient);
  vGradient = GrayImage();
  if (dbg) {
    dbg->add(hGradient, "vert_raster_lines");
  }

  const GrayImage rasterLines(closeGray(hGradient, QSize(1, 19), 0x00));
  hGradient = GrayImage();
  if (dbg) {
    dbg->add(rasterLines, "short_segments_removed");
  }

  const double lineThickness = 5.0;
  const double maxAngle = 7.0;  // degrees
  const double angleStep = 0.25;
  const auto angleStepsToMax = (int) (maxAngle / angleStep);
  const int totalAngleSteps = angleStepsToMax * 2 + 1;
  const double minAngle = -angleStepsToMax * angleStep;
  HoughLineDetector lineDetector(rasterLines.size(), lineThickness, minAngle, angleStep, totalAngleSteps);

  unsigned weight_table[256];
  buildWeightTable(weight_table);

  // We don't want to process areas too close to the vertical edges.
  const double marginMm = 3.5;
  const auto margin = (int) std::floor(0.5 + marginMm * constants::MM2INCH * dpi);

  const int xLimit = rasterLines.width() - margin;
  const int height = rasterLines.height();
  const uint8_t* line = rasterLines.data();
  const int stride = rasterLines.stride();
  for (int y = 0; y < height; ++y, line += stride) {
    for (int x = margin; x < xLimit; ++x) {
      const unsigned val = line[x];
      if (val > 1) {
        lineDetector.process(x, y, weight_table[val]);
      }
    }
  }

  const unsigned minQuality = (unsigned) (height * lineThickness * 1.8) + 1;

  if (dbg) {
    dbg->add(lineDetector.visualizeHoughSpace(minQuality), "hough_space");
  }

  const std::vector<HoughLine> houghLines(lineDetector.findLines(minQuality));

  using LineGroups = std::list<LineGroup>;
  LineGroups lineGroups;
  for (const HoughLine& houghLine : houghLines) {
    const QualityLine newLine(houghLine.pointAtY(0.0), houghLine.pointAtY(height), houghLine.quality());
    LineGroup* homeGroup = nullptr;

    auto it(lineGroups.begin());
    const auto end(lineGroups.end());
    while (it != end) {
      LineGroup& group = *it;
      if (group.belongsHere(newLine)) {
        if (homeGroup) {
          homeGroup->merge(group);
          lineGroups.erase(it++);
          continue;
        } else {
          group.add(newLine);
          homeGroup = &group;
        }
      }
      ++it;
    }

    if (!homeGroup) {
      lineGroups.emplace_back(newLine);
    }
  }

  std::vector<QLineF> lines;
  for (const LineGroup& group : lineGroups) {
    lines.push_back(group.leader().toQLine());
    if ((int) lines.size() == maxLines) {
      break;
    }
  }

  if (dbg) {
    QImage visual(preprocessed.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied));

    {
      QPainter painter(&visual);
      painter.setRenderHint(QPainter::Antialiasing);
      QPen pen(QColor(0xff, 0x00, 0x00, 0x80));
      pen.setWidthF(3.0);
      painter.setPen(pen);

      for (const QLineF& line : lines) {
        painter.drawLine(line);
      }
    }
    dbg->add(visual, "vector_lines");
  }

  // Transform lines back into original coordinates.
  const QTransform undo100dpi(xform100dpi.transformBack() * xform.transform());
  for (QLineF& line : lines) {
    line = undo100dpi.map(line);
  }
  return lines;
}  // VertLineFinder::findLines

GrayImage VertLineFinder::removeDarkVertBorders(const GrayImage& src) {
  GrayImage dst(src);

  selectVertBorders(dst);
  grayRasterOp<GRopInvert<GRopClippedSubtract<GRopDst, GRopSrc>>>(dst, src);
  return dst;
}

void VertLineFinder::selectVertBorders(GrayImage& image) {
  const int w = image.width();
  const int h = image.height();

  unsigned char* imageLine = image.data();
  const int imageStride = image.stride();

  std::vector<unsigned char> tmpLine(w, 0x00);

  for (int y = 0; y < h; ++y, imageLine += imageStride) {
    // Left to right.
    unsigned char prevPixel = 0x00;  // Black vertical border.
    for (int x = 0; x < w; ++x) {
      prevPixel = std::max(imageLine[x], prevPixel);
      tmpLine[x] = prevPixel;
    }
    // Right to left
    prevPixel = 0x00;  // Black vertical border.
    for (int x = w - 1; x >= 0; --x) {
      prevPixel = std::max(imageLine[x], std::min(prevPixel, tmpLine[x]));
      imageLine[x] = prevPixel;
    }
  }
}

void VertLineFinder::buildWeightTable(unsigned weight_table[]) {
  int grayLevel = 0;
  unsigned weight = 2;
  int segment = 2;
  int prevSegment = 1;

  while (grayLevel < 256) {
    const int limit = std::min(256, grayLevel + segment);
    for (; grayLevel < limit; ++grayLevel) {
      weight_table[grayLevel] = weight;
    }
    ++weight;
    segment += prevSegment;
    prevSegment = segment;
  }
}

/*======================= VertLineFinder::QualityLine =======================*/

VertLineFinder::QualityLine::QualityLine(const QPointF& top, const QPointF& bottom, const unsigned quality)
    : m_quality(quality) {
  if (top.x() < bottom.x()) {
    m_left = top;
    m_right = bottom;
  } else {
    m_left = bottom;
    m_right = top;
  }
}

QLineF VertLineFinder::QualityLine::toQLine() const {
  return QLineF(m_left, m_right);
}

const QPointF& VertLineFinder::QualityLine::left() const {
  return m_left;
}

const QPointF& VertLineFinder::QualityLine::right() const {
  return m_right;
}

unsigned VertLineFinder::QualityLine::quality() const {
  return m_quality;
}

/*======================= VertLineFinder::LineGroup ========================*/

VertLineFinder::LineGroup::LineGroup(const QualityLine& line)
    : m_leader(line), m_left(line.left().x()), m_right(line.right().x()) {}

bool VertLineFinder::LineGroup::belongsHere(const QualityLine& line) const {
  if (m_left > line.right().x()) {
    return false;
  } else if (m_right < line.left().x()) {
    return false;
  } else {
    return true;
  }
}

void VertLineFinder::LineGroup::add(const QualityLine& line) {
  m_left = std::min(m_left, line.left().x());
  m_right = std::max(m_right, line.right().x());
  if (line.quality() > m_leader.quality()) {
    m_leader = line;
  }
}

void VertLineFinder::LineGroup::merge(const LineGroup& other) {
  m_left = std::min(m_left, other.m_left);
  m_right = std::max(m_right, other.m_right);
  if (other.leader().quality() > m_leader.quality()) {
    m_leader = other.leader();
  }
}

const VertLineFinder::QualityLine& VertLineFinder::LineGroup::leader() const {
  return m_leader;
}
}  // namespace page_split