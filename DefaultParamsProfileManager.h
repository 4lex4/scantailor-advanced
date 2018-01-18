
#ifndef SCANTAILOR_DEFAULTPARAMSPROFILEMANAGER_H
#define SCANTAILOR_DEFAULTPARAMSPROFILEMANAGER_H


#include <QtCore/QString>
#include <memory>

class DefaultParams;

class DefaultParamsProfileManager {
private:
    QString path;

public:
    DefaultParamsProfileManager();

    explicit DefaultParamsProfileManager(const QString& path);

    std::unique_ptr<std::list<QString>> getProfileList();

    std::unique_ptr<DefaultParams> readProfile(const QString& name);

    bool writeProfile(const DefaultParams& params, const QString& name);

    bool deleteProfile(const QString& name);

    std::unique_ptr<DefaultParams> createDefaultProfile();

    std::unique_ptr<DefaultParams> createSourceProfile();
};


#endif //SCANTAILOR_DEFAULTPARAMSPROFILEMANAGER_H
