// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SPAN_H_
#define SPAN_H_

/**
 * \brief Represents a [from, to) range in one-dimensional space.
 */
class Span {
 public:
  /**
   * \brief Constructs an empty span.
   */
  Span() : m_begin(0), m_end(0) {}

  /**
   * \brief Constructs a [begin, end) span.
   */
  Span(int begin, int end) : m_begin(begin), m_end(end) {}

  /**
   * \brief Constructs a span between a point and another span.
   */
  Span(int begin, const Span& end) : m_begin(begin), m_end(end.begin()) {}

  /**
   * \brief Constructs a span between another span and a point.
   */
  Span(const Span& begin, int end) : m_begin(begin.end()), m_end(end) {}

  /**
   * \brief Constructs a span between two other spans.
   */
  Span(const Span& begin, const Span& end) : m_begin(begin.end()), m_end(end.begin()) {}

  int begin() const { return m_begin; }

  int end() const { return m_end; }

  int width() const { return m_end - m_begin; }

  double center() const { return 0.5 * (m_begin + m_end); }

  bool operator==(const Span& other) const { return m_begin == other.m_begin && m_end == other.m_end; }

  bool operator!=(const Span& other) const { return m_begin != other.m_begin || m_end != other.m_end; }

  Span& operator+=(int offset) {
    m_begin += offset;
    m_end += offset;

    return *this;
  }

  Span& operator-=(int offset) {
    m_begin -= offset;
    m_end -= offset;

    return *this;
  }

  Span operator+(int offset) const {
    Span span(*this);
    span += offset;

    return span;
  }

  Span operator-(int offset) const {
    Span span(*this);
    span -= offset;

    return span;
  }

 private:
  int m_begin;
  int m_end;
};


#endif  // ifndef SPAN_H_
