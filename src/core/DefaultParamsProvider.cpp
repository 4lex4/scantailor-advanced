
#include "DefaultParamsProvider.h"
#include <cassert>
#include "ApplicationSettings.h"
#include "DefaultParams.h"
#include "DefaultParamsProfileManager.h"

DefaultParamsProvider::DefaultParamsProvider() {
  ApplicationSettings& settings = ApplicationSettings::getInstance();
  DefaultParamsProfileManager defaultParamsProfileManager;

  const QString profile = settings.getCurrentProfile();
  if (profile == "Default") {
    m_params = defaultParamsProfileManager.createDefaultProfile();
    m_profileName = profile;
  } else if (profile == "Source") {
    m_params = defaultParamsProfileManager.createSourceProfile();
    m_profileName = profile;
  } else {
    std::unique_ptr<DefaultParams> params = defaultParamsProfileManager.readProfile(profile);
    if (params != nullptr) {
      m_params = std::move(params);
      m_profileName = profile;
    } else {
      m_params = defaultParamsProfileManager.createDefaultProfile();
      m_profileName = "Default";
      settings.setCurrentProfile("Default");
    }
  }
}

DefaultParamsProvider& DefaultParamsProvider::getInstance() {
  static DefaultParamsProvider instance;
  return instance;
}

const QString& DefaultParamsProvider::getProfileName() const {
  return m_profileName;
}

DefaultParams DefaultParamsProvider::getParams() const {
  assert(m_params != nullptr);

  return *m_params;
}

void DefaultParamsProvider::setParams(std::unique_ptr<DefaultParams> params, const QString& name) {
  if (params == nullptr) {
    return;
  }

  m_params = std::move(params);
  m_profileName = name;
}
