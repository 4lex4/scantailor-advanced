
#include "DefaultParamsProvider.h"
#include <QtCore/QSettings>
#include <cassert>
#include "DefaultParams.h"
#include "DefaultParamsProfileManager.h"

std::unique_ptr<DefaultParamsProvider> DefaultParamsProvider::instance = nullptr;

DefaultParamsProvider::DefaultParamsProvider() {
  QSettings settings;
  DefaultParamsProfileManager defaultParamsProfileManager;

  const QString profile = settings.value("settings/current_profile", "Default").toString();
  if (profile == "Default") {
    this->params = defaultParamsProfileManager.createDefaultProfile();
    this->profileName = profile;
  } else if (profile == "Source") {
    this->params = defaultParamsProfileManager.createSourceProfile();
    this->profileName = profile;
  } else {
    std::unique_ptr<DefaultParams> params = defaultParamsProfileManager.readProfile(profile);
    if (params != nullptr) {
      this->params = std::move(params);
      this->profileName = profile;
    } else {
      this->params = defaultParamsProfileManager.createDefaultProfile();
      this->profileName = "Default";
      settings.setValue("settings/current_profile", "Default");
    }
  }
}

DefaultParamsProvider* DefaultParamsProvider::getInstance() {
  if (instance == nullptr) {
    instance.reset(new DefaultParamsProvider());
  }

  return instance.get();
}

const QString& DefaultParamsProvider::getProfileName() const {
  return profileName;
}

DefaultParams DefaultParamsProvider::getParams() const {
  assert(params != nullptr);

  return *params;
}

void DefaultParamsProvider::setParams(std::unique_ptr<DefaultParams> params, const QString& name) {
  if (params == nullptr) {
    return;
  }

  this->params = std::move(params);
  this->profileName = name;
}
