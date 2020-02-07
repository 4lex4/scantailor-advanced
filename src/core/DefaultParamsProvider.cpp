// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

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

const DefaultParams& DefaultParamsProvider::getParams() const {
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
