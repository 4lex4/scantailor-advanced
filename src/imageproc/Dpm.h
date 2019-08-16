// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef DPM_H_
#define DPM_H_

#include <QSize>

class Dpi;
class QImage;

/**
 * \brief Dots per meter (horizontal and vertical).
 */
class Dpm {
  // Member-wise copying is OK.
 public:
  Dpm() : m_xDpm(0), m_yDpm(0) {}

  Dpm(int horizontal, int vertical) : m_xDpm(horizontal), m_yDpm(vertical) {}

  Dpm(Dpi dpi);

  explicit Dpm(QSize size);

  explicit Dpm(const QImage& image);

  int horizontal() const { return m_xDpm; }

  int vertical() const { return m_yDpm; }

  QSize toSize() const;

  bool isNull() const;

  bool operator==(const Dpm& other) const;

  bool operator!=(const Dpm& other) const { return !(*this == other); }

 private:
  int m_xDpm;
  int m_yDpm;
};


#endif  // ifndef DPM_H_
