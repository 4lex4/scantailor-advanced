
#ifndef SCANTAILOR_DEFAULTPARAMSPROFILEMANAGER_H
#define SCANTAILOR_DEFAULTPARAMSPROFILEMANAGER_H


#include <foundation/NonCopyable.h>
#include <QtCore/QString>
#include <list>
#include <memory>

class DefaultParams;

class DefaultParamsProfileManager {
  DECLARE_NON_COPYABLE(DefaultParamsProfileManager)
 public:
  DefaultParamsProfileManager();

  explicit DefaultParamsProfileManager(const QString& path);

  enum LoadStatus {
    SUCCESS,
    IO_ERROR,
    INCOMPATIBLE_VERSION_ERROR
  };

  std::list<QString> getProfileList() const;

  std::unique_ptr<DefaultParams> readProfile(const QString& name, LoadStatus* status = nullptr) const;

  bool writeProfile(const DefaultParams& params, const QString& name) const;

  bool deleteProfile(const QString& name) const;

  std::unique_ptr<DefaultParams> createDefaultProfile() const;

  std::unique_ptr<DefaultParams> createSourceProfile() const;

 private:
  QString m_path;
};


#endif  // SCANTAILOR_DEFAULTPARAMSPROFILEMANAGER_H
