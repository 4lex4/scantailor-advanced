
#ifndef SCANTAILOR_DEFAULTPARAMSPROVIDER_H
#define SCANTAILOR_DEFAULTPARAMSPROVIDER_H

#include <foundation/NonCopyable.h>
#include <QtCore/QString>
#include <memory>

class DefaultParams;

class DefaultParamsProvider {
  DECLARE_NON_COPYABLE(DefaultParamsProvider)
 private:
  DefaultParamsProvider();

 public:
  static DefaultParamsProvider& getInstance();

  const QString& getProfileName() const;

  DefaultParams getParams() const;

  void setParams(std::unique_ptr<DefaultParams> params, const QString& name);

 private:
  QString m_profileName;
  std::unique_ptr<DefaultParams> m_params;
};


#endif  // SCANTAILOR_DEFAULTPARAMSPROVIDER_H
