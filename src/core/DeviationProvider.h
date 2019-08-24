// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_DEVIATIONPROVIDER_H_
#define SCANTAILOR_CORE_DEVIATIONPROVIDER_H_

#include <foundation/NonCopyable.h>
#include <cmath>
#include <functional>
#include <unordered_map>

template <typename K, typename Hash = std::hash<K>>
class DeviationProvider {
  DECLARE_NON_COPYABLE(DeviationProvider)
 public:
  DeviationProvider() = default;

  explicit DeviationProvider(const std::function<double(const K&)>& computeValueByKey);

  bool isDeviant(const K& key, double coefficient = 1.0, double threshold = 0.0) const;

  double getDeviationValue(const K& key) const;

  void addOrUpdate(const K& key);

  void addOrUpdate(const K& key, double value);

  void remove(const K& key);

  void clear();

  void setComputeValueByKey(const std::function<double(const K&)>& computeValueByKey);

 protected:
  void update() const;

 private:
  std::function<double(const K&)> m_computeValueByKey;
  std::unordered_map<K, double, Hash> m_keyValueMap;

  // Cached values.
  mutable bool m_needUpdate = false;
  mutable double m_meanValue = 0.0;
  mutable double m_standardDeviation = 0.0;
};


template <typename K, typename Hash>
DeviationProvider<K, Hash>::DeviationProvider(const std::function<double(const K&)>& computeValueByKey)
    : m_computeValueByKey(computeValueByKey) {}

template <typename K, typename Hash>
bool DeviationProvider<K, Hash>::isDeviant(const K& key, const double coefficient, const double threshold) const {
  if (m_keyValueMap.find(key) == m_keyValueMap.end()) {
    return false;
  }
  if (m_keyValueMap.size() < 3) {
    return false;
  }

  update();

  return (std::abs(m_keyValueMap.at(key) - m_meanValue) > std::max((coefficient * m_standardDeviation), threshold));
}

template <typename K, typename Hash>
double DeviationProvider<K, Hash>::getDeviationValue(const K& key) const {
  if (m_keyValueMap.find(key) == m_keyValueMap.end()) {
    return .0;
  }
  if (m_keyValueMap.size() < 2) {
    return .0;
  }

  update();

  return std::abs(m_keyValueMap.at(key) - m_meanValue);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::addOrUpdate(const K& key) {
  m_needUpdate = true;

  m_keyValueMap[key] = m_computeValueByKey(key);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::addOrUpdate(const K& key, const double value) {
  m_needUpdate = true;

  m_keyValueMap[key] = value;
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::remove(const K& key) {
  m_needUpdate = true;

  if (m_keyValueMap.find(key) == m_keyValueMap.end()) {
    return;
  }

  m_keyValueMap.erase(key);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::update() const {
  if (!m_needUpdate) {
    return;
  }
  if (m_keyValueMap.size() < 2) {
    return;
  }

  {
    double sum = .0;
    for (const std::pair<K, double>& keyAndValue : m_keyValueMap) {
      sum += keyAndValue.second;
    }
    m_meanValue = sum / m_keyValueMap.size();
  }

  {
    double differencesSum = .0;
    for (const std::pair<K, double>& keyAndValue : m_keyValueMap) {
      differencesSum += std::pow(keyAndValue.second - m_meanValue, 2);
    }
    m_standardDeviation = std::sqrt(differencesSum / (m_keyValueMap.size() - 1));
  }

  m_needUpdate = false;
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::setComputeValueByKey(const std::function<double(const K&)>& computeValueByKey) {
  this->m_computeValueByKey = std::move(computeValueByKey);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::clear() {
  m_keyValueMap.clear();

  m_needUpdate = false;
  m_meanValue = 0.0;
  m_standardDeviation = 0.0;
}


#endif  // SCANTAILOR_CORE_DEVIATIONPROVIDER_H_
