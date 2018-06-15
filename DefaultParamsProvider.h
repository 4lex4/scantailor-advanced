
#ifndef SCANTAILOR_DEFAULTPARAMSPROVIDER_H
#define SCANTAILOR_DEFAULTPARAMSPROVIDER_H


#include <QtCore/QString>
#include <memory>

class DefaultParams;

class DefaultParamsProvider {
 private:
  static std::unique_ptr<DefaultParamsProvider> instance;

  QString profileName;
  std::unique_ptr<DefaultParams> params;

  DefaultParamsProvider();

 public:
  static DefaultParamsProvider* getInstance();

  const QString& getProfileName() const;

  DefaultParams getParams() const;

  void setParams(std::unique_ptr<DefaultParams> params, const QString& name);
};


#endif  // SCANTAILOR_DEFAULTPARAMSPROVIDER_H
