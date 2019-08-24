// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_IMAGEPROC_DPI_H_
#define SCANTAILOR_IMAGEPROC_DPI_H_

#include <QSize>

class Dpm;
class QDomElement;
class QDomDocument;

/**
 * \brief Dots per inch (horizontal and vertical).
 */
class Dpi {
 public:
  Dpi();

  Dpi(int horizontal, int vertical);

  explicit Dpi(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  Dpi(Dpm dpm);

  explicit Dpi(QSize size);

  int horizontal() const;

  int vertical() const;

  QSize toSize() const;

  bool isNull() const;

  bool operator==(const Dpi& other) const;

  bool operator!=(const Dpi& other) const;

 private:
  int m_xDpi;
  int m_yDpi;
};


#endif  // ifndef SCANTAILOR_IMAGEPROC_DPI_H_
