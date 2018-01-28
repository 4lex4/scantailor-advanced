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

namespace deskew {
    Settings::Settings()
            : m_avg(0.0),
              m_sigma(0.0),
              m_maxDeviation(5.0) {
    }

    Settings::~Settings() = default;

    void Settings::clear() {
        QMutexLocker locker(&m_mutex);
        m_perPageParams.clear();
    }

    void Settings::performRelinking(const AbstractRelinker& relinker) {
        QMutexLocker locker(&m_mutex);
        PerPageParams new_params;

        for (const PerPageParams::value_type& kv : m_perPageParams) {
            const RelinkablePath old_path(kv.first.imageId().filePath(), RelinkablePath::File);
            PageId new_page_id(kv.first);
            new_page_id.imageId().setFilePath(relinker.substitutionPathFor(old_path));
            new_params.insert(PerPageParams::value_type(new_page_id, kv.second));
        }

        m_perPageParams.swap(new_params);
    }

    void Settings::updateDeviation() {
        m_avg = 0.0;
        for (const PerPageParams::value_type& kv : m_perPageParams) {
            m_avg += kv.second.deskewAngle();
        }
        m_avg = m_avg / m_perPageParams.size();
#ifdef DEBUG
        std::cout << "avg skew = " << m_avg << std::endl;
#endif

        double sigma2 = 0.0;
        for (PerPageParams::value_type& kv : m_perPageParams) {
            kv.second.computeDeviation(m_avg);
            sigma2 += kv.second.deviation() * kv.second.deviation();
        }
        sigma2 = sigma2 / m_perPageParams.size();
        m_sigma = sqrt(sigma2);
#ifdef DEBUG
        std::cout << "sigma2 = " << sigma2 << std::endl;
        std::cout << "sigma = " << m_sigma << std::endl;
#endif
    }

    void Settings::setPageParams(const PageId& page_id, const Params& params) {
        QMutexLocker locker(&m_mutex);
        Utils::mapSetValue(m_perPageParams, page_id, params);
    }

    void Settings::clearPageParams(const PageId& page_id) {
        QMutexLocker locker(&m_mutex);
        m_perPageParams.erase(page_id);
    }

    std::unique_ptr<Params>
    Settings::getPageParams(const PageId& page_id) const {
        QMutexLocker locker(&m_mutex);

        auto it(m_perPageParams.find(page_id));
        if (it != m_perPageParams.end()) {
            return std::make_unique<Params>(it->second);
        } else {
            return nullptr;
        }
    }

    void Settings::setDegress(const std::set<PageId>& pages, const Params& params) {
        const QMutexLocker locker(&m_mutex);
        for (const PageId& page : pages) {
            Utils::mapSetValue(m_perPageParams, page, params);
        }
    }

    bool Settings::isParamsNull(const PageId& page_id) const {
        QMutexLocker locker(&m_mutex);

        return m_perPageParams.count(page_id) == 0;
    }
}  // namespace deskew