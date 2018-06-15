
#ifndef SCANTAILOR_DEFAULTPARAMSPROFILEMANAGER_H
#define SCANTAILOR_DEFAULTPARAMSPROFILEMANAGER_H


#include <QtCore/QString>
#include <list>
#include <memory>

class DefaultParams;

class DefaultParamsProfileManager {
 private:
  QString path;

 public:
  DefaultParamsProfileManager();

  explicit DefaultParamsProfileManager(const QString& path);

  std::unique_ptr<std::list<QString>> getProfileList() const;

  std::unique_ptr<DefaultParams> readProfile(const QString& name) const;

  bool writeProfile(const DefaultParams& params, const QString& name) const;

  bool deleteProfile(const QString& name) const;

  std::unique_ptr<DefaultParams> createDefaultProfile() const;

  std::unique_ptr<DefaultParams> createSourceProfile() const;
};


#endif  // SCANTAILOR_DEFAULTPARAMSPROFILEMANAGER_H
