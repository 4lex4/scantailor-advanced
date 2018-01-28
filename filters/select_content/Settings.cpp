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

#include "Settings.h"
#include "Utils.h"
#include "RelinkablePath.h"
#include "AbstractRelinker.h"
#include <iostream>

namespace select_content {
    Settings::Settings()
            : m_avg(0.0),
              m_sigma(0.0),
              m_pageDetectionBox(0.0, 0.0),
              m_pageDetectionTolerance(0.1),
              m_maxDeviation(1.0) {
    }

    Settings::~Settings() = default;

    void Settings::clear() {
        QMutexLocker locker(&m_mutex);
        m_pageParams.clear();
    }

    void Settings::performRelinking(const AbstractRelinker& relinker) {
        QMutexLocker locker(&m_mutex);
        PageParams new_params;

        for (const PageParams::value_type& kv : m_pageParams) {
            const RelinkablePath old_path(kv.first.imageId().filePath(), RelinkablePath::File);
            PageId new_page_id(kv.first);
            new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
            new_params.insert(PageParams::value_type(new_page_id, kv.second));
        }

        m_pageParams.swap(new_params);
    }

    void Settings::updateDeviation() {
        m_avg = 0.0;
        for (PageParams::value_type& kv : m_pageParams) {
            kv.second.computeDeviation(0.0);
            m_avg += -1 * kv.second.deviation();
        }
        m_avg = m_avg / double(m_pageParams.size());
#ifdef DEBUG
        std::cout << "avg_content = " << m_avg << std::endl;
#endif

        double sigma2 = 0.0;
        for (PageParams::value_type& kv : m_pageParams) {
            kv.second.computeDeviation(m_avg);
            sigma2 += kv.second.deviation() * kv.second.deviation();
        }
        sigma2 = sigma2 / double(m_pageParams.size());
        m_sigma = sqrt(sigma2);
#if DEBUG
        std::cout << "sigma2 = " << sigma2 << std::endl;
        std::cout << "sigma = " << m_sigma << std::endl;
#endif
    }

    void Settings::setPageParams(const PageId& page_id, const Params& params) {
        QMutexLocker locker(&m_mutex);
        Utils::mapSetValue(m_pageParams, page_id, params);
    }

    void Settings::clearPageParams(const PageId& page_id) {
        QMutexLocker locker(&m_mutex);
        m_pageParams.erase(page_id);
    }

    std::unique_ptr<Params>
    Settings::getPageParams(const PageId& page_id) const {
        QMutexLocker locker(&m_mutex);

        const auto it(m_pageParams.find(page_id));
        if (it != m_pageParams.end()) {
            return std::make_unique<Params>(it->second);
        } else {
            return nullptr;
        }
    }

    bool Settings::isParamsNull(const PageId& page_id) const {
        QMutexLocker locker(&m_mutex);

        return m_pageParams.count(page_id) == 0;
    }

    double Settings::maxDeviation() const {
        return m_maxDeviation;
    }

    void Settings::setMaxDeviation(double md) {
        m_maxDeviation = md;
    }

    QSizeF Settings::pageDetectionBox() const {
        return m_pageDetectionBox;
    }

    void Settings::setPageDetectionBox(QSizeF size) {
        m_pageDetectionBox = size;
    }

    double Settings::pageDetectionTolerance() const {
        return m_pageDetectionTolerance;
    }

    void Settings::setPageDetectionTolerance(double tolerance) {
        m_pageDetectionTolerance = tolerance;
    }

    double Settings::avg() const {
        return m_avg;
    }

    void Settings::setAvg(double a) {
        m_avg = a;
    }

    double Settings::std() const {
        return m_sigma;
    }

    void Settings::setStd(double s) {
        m_sigma = s;
    }
}  // namespace select_content