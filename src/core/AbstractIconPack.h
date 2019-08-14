#ifndef SCANTAILOR_ABSTRACTICONPACK_H
#define SCANTAILOR_ABSTRACTICONPACK_H

#include "IconPack.h"

class AbstractIconPack : public IconPack {
 public:
  QIcon getIcon(const QString& iconKey) const override;

  void mergeWith(std::unique_ptr<IconPack> pack) override;

 protected:
  static QIcon::Mode iconModeFromString(const QString& mode);

  static QIcon::State iconStateFromString(const QString& state);

 private:
  std::unique_ptr<IconPack> m_parentIconPack;
};


#endif  // SCANTAILOR_ABSTRACTICONPACK_H
