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

#include "ArcLengthMapper.h"
#include <math.h>
#include <assert.h>

ArcLengthMapper::Hint::Hint()
        : m_lastSegment(0),
          m_direction(1) {
}

void ArcLengthMapper::Hint::update(int new_segment) {
    m_direction = new_segment < m_lastSegment ? -1 : 1;
    m_lastSegment = new_segment;
}

ArcLengthMapper::ArcLengthMapper()
        : m_prevFX() {
}

void ArcLengthMapper::addSample(double x, double fx) {
    double arc_len = 0;

    if (!m_samples.empty()) {
        double const dx = x - m_samples.back().x;
        double const dy = fx - m_prevFX;
        assert(dx > 0);
        arc_len = m_samples.back().arcLen + sqrt(dx * dx + dy * dy);
    }

    m_samples.push_back(Sample(x, arc_len));
    m_prevFX = fx;
}

double ArcLengthMapper::totalArcLength() const {
    return m_samples.size() < 2 ? 0.0 : m_samples.back().arcLen;
}

void ArcLengthMapper::normalizeRange(double total_arc_len) {
    if (m_samples.size() <= 1) {
        return;
    }

    assert(total_arc_len != 0);

    double const scale = total_arc_len / m_samples.back().arcLen;
    for (Sample& sample : m_samples) {
        sample.arcLen *= scale;
    }
}

double ArcLengthMapper::arcLenToX(double arc_len, Hint& hint) const {
    switch (m_samples.size()) {
        case 0:
            return 0;
        case 1:
            return m_samples.front().x;
    }

    if (arc_len < 0) {
        hint.update(0);

        return interpolateArcLenInSegment(arc_len, 0);
    } else if (arc_len > m_samples.back().arcLen) {
        hint.update(m_samples.size() - 2);

        return interpolateArcLenInSegment(arc_len, hint.m_lastSegment);
    }

    if (checkSegmentForArcLen(arc_len, hint.m_lastSegment)) {
        return interpolateArcLenInSegment(arc_len, hint.m_lastSegment);
    } else if (checkSegmentForArcLen(arc_len, hint.m_lastSegment + hint.m_direction)) {
        hint.update(hint.m_lastSegment + hint.m_direction);

        return interpolateArcLenInSegment(arc_len, hint.m_lastSegment);
    } else if (checkSegmentForArcLen(arc_len, hint.m_lastSegment - hint.m_direction)) {
        hint.update(hint.m_lastSegment - hint.m_direction);

        return interpolateArcLenInSegment(arc_len, hint.m_lastSegment);
    }

    int left_idx = 0;
    int right_idx = m_samples.size() - 1;
    double left_arc_len = m_samples[left_idx].arcLen;
    while (left_idx + 1 < right_idx) {
        int const mid_idx = (left_idx + right_idx) >> 1;
        double const mid_arc_len = m_samples[mid_idx].arcLen;
        if ((arc_len - mid_arc_len) * (arc_len - left_arc_len) <= 0) {
            right_idx = mid_idx;
        } else {
            left_idx = mid_idx;
            left_arc_len = mid_arc_len;
        }
    }

    hint.update(left_idx);

    return interpolateArcLenInSegment(arc_len, left_idx);
}  // ArcLengthMapper::arcLenToX

double ArcLengthMapper::xToArcLen(double x, Hint& hint) const {
    switch (m_samples.size()) {
        case 0:
            return 0;
        case 1:
            return m_samples.front().arcLen;
    }

    if (x < m_samples.front().x) {
        hint.update(0);

        return interpolateXInSegment(x, 0);
    } else if (x > m_samples.back().x) {
        hint.update(m_samples.size() - 2);

        return interpolateXInSegment(x, hint.m_lastSegment);
    }

    if (checkSegmentForX(x, hint.m_lastSegment)) {
        return interpolateXInSegment(x, hint.m_lastSegment);
    } else if (checkSegmentForX(x, hint.m_lastSegment + hint.m_direction)) {
        hint.update(hint.m_lastSegment + hint.m_direction);

        return interpolateXInSegment(x, hint.m_lastSegment);
    } else if (checkSegmentForX(x, hint.m_lastSegment - hint.m_direction)) {
        hint.update(hint.m_lastSegment - hint.m_direction);

        return interpolateXInSegment(x, hint.m_lastSegment);
    }

    int left_idx = 0;
    int right_idx = m_samples.size() - 1;
    double left_x = m_samples[left_idx].x;
    while (left_idx + 1 < right_idx) {
        int const mid_idx = (left_idx + right_idx) >> 1;
        double const mid_x = m_samples[mid_idx].x;
        if ((x - mid_x) * (x - left_x) <= 0) {
            right_idx = mid_idx;
        } else {
            left_idx = mid_idx;
            left_x = mid_x;
        }
    }

    hint.update(left_idx);

    return interpolateXInSegment(x, left_idx);
}  // ArcLengthMapper::xToArcLen

bool ArcLengthMapper::checkSegmentForArcLen(double arc_len, int segment) const {
    assert(m_samples.size() > 1);
    if ((segment < 0) || (segment >= int(m_samples.size()) - 1)) {
        return false;
    }

    double const left_arc_len = m_samples[segment].arcLen;
    double const right_arc_len = m_samples[segment + 1].arcLen;

    return (arc_len - left_arc_len) * (arc_len - right_arc_len) <= 0;
}

bool ArcLengthMapper::checkSegmentForX(double x, int segment) const {
    assert(m_samples.size() > 1);
    if ((segment < 0) || (segment >= int(m_samples.size()) - 1)) {
        return false;
    }

    double const left_x = m_samples[segment].x;
    double const right_x = m_samples[segment + 1].x;

    return (x - left_x) * (x - right_x) <= 0;
}

double ArcLengthMapper::interpolateArcLenInSegment(double arc_len, int segment) const {
    double const x0 = m_samples[segment].x;
    double const a0 = m_samples[segment].arcLen;
    double const x1 = m_samples[segment + 1].x;
    double const a1 = m_samples[segment + 1].arcLen;
    double const x = x0 + (arc_len - a0) * (x1 - x0) / (a1 - a0);

    return x;
}

double ArcLengthMapper::interpolateXInSegment(double x, int segment) const {
    double const x0 = m_samples[segment].x;
    double const a0 = m_samples[segment].arcLen;
    double const x1 = m_samples[segment + 1].x;
    double const a1 = m_samples[segment + 1].arcLen;
    double const a = a0 + (a1 - a0) * (x - x0) / (x1 - x0);

    return a;
}

