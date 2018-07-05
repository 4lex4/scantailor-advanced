/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PAGE_SPLIT_VERTLINEFINDER_H_
#define PAGE_SPLIT_VERTLINEFINDER_H_

#include <QImage>
#include <QPointF>
#include <QTransform>
#include <vector>

class QLineF;
class QImage;
class ImageTransformation;
class DebugImages;

namespace imageproc {
class GrayImage;
}

namespace page_split {
class VertLineFinder {
 public:
  static std::vector<QLineF> findLines(const QImage& image,
                                       const ImageTransformation& xform,
                                       int max_lines,
                                       DebugImages* dbg = nullptr,
                                       imageproc::GrayImage* gray_downscaled = nullptr,
                                       QTransform* out_to_downscaled = nullptr);

 private:
  class QualityLine {
   public:
    QualityLine(const QPointF& top, const QPointF& bottom, unsigned quality);

    const QPointF& left() const;

    const QPointF& right() const;

    unsigned quality() const;

    QLineF toQLine() const;

   private:
    QPointF m_left;
    QPointF m_right;
    unsigned m_quality;
  };


  class LineGroup {
   public:
    explicit LineGroup(const QualityLine& line);

    bool belongsHere(const QualityLine& line) const;

    void add(const QualityLine& line);

    void merge(const LineGroup& other);

    const QualityLine& leader() const;

   private:
    QualityLine m_leader;
    double m_left;
    double m_right;
  };


  static imageproc::GrayImage removeDarkVertBorders(const imageproc::GrayImage& src);

  static void selectVertBorders(imageproc::GrayImage& image);

  static void buildWeightTable(unsigned weight_table[]);
};
}  // namespace page_split
#endif  // ifndef PAGE_SPLIT_VERTLINEFINDER_H_
