#ifndef SCANTAILOR_ICONPACK_H
#define SCANTAILOR_ICONPACK_H

#include <QIcon>
#include <memory>

class QString;

class IconPack {
 public:
  virtual ~IconPack() = default;

  virtual QIcon getIcon(const QString& iconKey) const = 0;

  virtual void mergeWith(std::unique_ptr<IconPack> pack) = 0;
};

#endif  // SCANTAILOR_ICONPACK_H
