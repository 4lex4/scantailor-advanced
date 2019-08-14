#include "AbstractIconPack.h"

void AbstractIconPack::mergeWith(std::unique_ptr<IconPack> pack) {
  m_parentIconPack = std::move(pack);
}

QIcon AbstractIconPack::getIcon(const QString& iconKey) const {
  if (m_parentIconPack) {
    return m_parentIconPack->getIcon(iconKey);
  }
  return QIcon();
}

QIcon::Mode AbstractIconPack::iconModeFromString(const QString& mode) {
  if (mode == "normal") {
    return QIcon::Mode::Normal;
  } else if (mode == "disabled") {
    return QIcon::Mode::Disabled;
  } else if (mode == "selected") {
    return QIcon::Mode::Selected;
  } else if (mode == "active") {
    return QIcon::Mode::Active;
  }
  return QIcon::Mode::Normal;
}

QIcon::State AbstractIconPack::iconStateFromString(const QString& state) {
  if (state == "off") {
    return QIcon::State::Off;
  } else if (state == "on") {
    return QIcon::State::On;
  }
  return QIcon::State::Off;
}