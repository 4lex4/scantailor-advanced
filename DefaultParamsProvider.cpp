
#include "DefaultParamsProvider.h"
#include <QtCore/QSettings>
#include <cassert>
#include "DefaultParams.h"
#include "DefaultParamsProfileManager.h"

std::unique_ptr<DefaultParamsProvider> DefaultParamsProvider::m_instance = nullptr;

DefaultParamsProvider::DefaultParamsProvider() {
  QSettings settings;
  DefaultParamsProfileManager defaultParamsProfileManager;

  const QString profile = settings.value("settings/current_profile", "Default").toString();
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
      settings.setValue("settings/current_profile", "Default");
    }
  }
}

DefaultParamsProvider* DefaultParamsProvider::getInstance() {
  if (m_instance == nullptr) {
    m_instance.reset(new DefaultParamsProvider());
  }

  return m_instance.get();
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
