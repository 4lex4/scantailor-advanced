#ifndef SCANTAILOR_ICONPROVIDER_H
#define SCANTAILOR_ICONPROVIDER_H


#include <foundation/NonCopyable.h>
#include <QtGui/QIcon>
#include <memory>

class IconPack;

class IconProvider {
  DECLARE_NON_COPYABLE(IconProvider)
 private:
  IconProvider();

  ~IconProvider();

 public:
  static IconProvider& getInstance();

  void setIconPack(std::unique_ptr<IconPack> pack);

  void addIconPack(std::unique_ptr<IconPack> pack);

  QIcon getIcon(const QString& iconKey) const;

 private:
  std::unique_ptr<IconPack> m_iconPack;
};


#endif  // SCANTAILOR_ICONPROVIDER_H
