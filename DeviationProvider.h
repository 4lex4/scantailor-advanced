
#ifndef SCANTAILOR_DEVIATION_H
#define SCANTAILOR_DEVIATION_H

#include <cmath>
#include <functional>
#include <unordered_map>

template <typename K, typename Hash = std::hash<K>>
class DeviationProvider {
 private:
  std::function<double(const K&)> computeValueByKey;
  std::unordered_map<K, double, Hash> keyValueMap;

  // Cached values.
  mutable bool needUpdate = false;
  mutable double meanValue = 0.0;
  mutable double standardDeviation = 0.0;

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
};


template <typename K, typename Hash>
DeviationProvider<K, Hash>::DeviationProvider(const std::function<double(const K&)>& computeValueByKey)
    : computeValueByKey(computeValueByKey) {}

template <typename K, typename Hash>
bool DeviationProvider<K, Hash>::isDeviant(const K& key, const double coefficient, const double threshold) const {
  if (keyValueMap.find(key) == keyValueMap.end()) {
    return false;
  }
  if (keyValueMap.size() < 3) {
    return false;
  }

  update();

  return (std::abs(keyValueMap.at(key) - meanValue) > std::max((coefficient * standardDeviation), threshold));
}

template <typename K, typename Hash>
double DeviationProvider<K, Hash>::getDeviationValue(const K& key) const {
  if (keyValueMap.find(key) == keyValueMap.end()) {
    return .0;
  }
  if (keyValueMap.size() < 2) {
    return .0;
  }

  update();

  return std::abs(keyValueMap.at(key) - meanValue);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::addOrUpdate(const K& key) {
  needUpdate = true;

  keyValueMap[key] = computeValueByKey(key);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::addOrUpdate(const K& key, const double value) {
  needUpdate = true;

  keyValueMap[key] = value;
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::remove(const K& key) {
  needUpdate = true;

  if (keyValueMap.find(key) == keyValueMap.end()) {
    return;
  }

  keyValueMap.erase(key);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::update() const {
  if (!needUpdate) {
    return;
  }
  if (keyValueMap.size() < 2) {
    return;
  }

  {
    double sum = .0;
    for (const std::pair<K, double>& keyAndValue : keyValueMap) {
      sum += keyAndValue.second;
    }
    meanValue = sum / keyValueMap.size();
  }

  {
    double differencesSum = .0;
    for (const std::pair<K, double>& keyAndValue : keyValueMap) {
      differencesSum += std::pow(keyAndValue.second - meanValue, 2);
    }
    standardDeviation = std::sqrt(differencesSum / (keyValueMap.size() - 1));
  }

  needUpdate = false;
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::setComputeValueByKey(const std::function<double(const K&)>& computeValueByKey) {
  this->computeValueByKey = std::move(computeValueByKey);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::clear() {
  keyValueMap.clear();

  needUpdate = false;
  meanValue = 0.0;
  standardDeviation = 0.0;
}


#endif  // SCANTAILOR_DEVIATION_H
