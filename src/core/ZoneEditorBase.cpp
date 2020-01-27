// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ZoneEditorBase.h"

#include <QKeyEvent>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStatusBar>

#include "Utils.h"
#include "ZoneModeListener.h"

class ZoneEditorBase::ZoneModeProvider {
  DECLARE_NON_COPYABLE(ZoneModeProvider)
 public:
  explicit ZoneModeProvider(const ZoneEditorBase& parent);

  ~ZoneModeProvider();

  void addListener(ZoneModeListener* listener);

  void removeListener(ZoneModeListener* listener);

  void updateZoneMode();

 private:
  std::list<ZoneModeListener*> m_infoListeners;
  const ZoneEditorBase& m_parent;
};

ZoneEditorBase::ZoneEditorBase(const QImage& image,
                               const ImagePixmapUnion& downscaledVersion,
                               const ImagePresentation& presentation,
                               const Margins& margins)
    : ImageViewBase(image, downscaledVersion, presentation, margins),
      m_context(*this, m_zones),
      m_zoneModeProvider(std::make_unique<ZoneModeProvider>(*this)) {}

ZoneEditorBase::~ZoneEditorBase() = default;

void ZoneEditorBase::onKeyPressEvent(QKeyEvent* event, InteractionState&) {
  switch (event->key()) {
    case Qt::Key_Z:
      m_context.setZoneCreationMode(ZoneCreationMode::POLYGONAL);
      event->accept();
      m_zoneModeProvider->updateZoneMode();
      break;
    case Qt::Key_X:
      m_context.setZoneCreationMode(ZoneCreationMode::LASSO);
      event->accept();
      m_zoneModeProvider->updateZoneMode();
      break;
    case Qt::Key_C:
      m_context.setZoneCreationMode(ZoneCreationMode::RECTANGULAR);
      event->accept();
      m_zoneModeProvider->updateZoneMode();
      break;
    default:
      break;
  }
}

void ZoneEditorBase::showEvent(QShowEvent* event) {
  ImageViewBase::showEvent(event);

  if (auto* mainWindow = dynamic_cast<QMainWindow*>(window())) {
    if (auto* zoneModeListener = core::Utils::castOrFindChild<ZoneModeListener*>(mainWindow->statusBar())) {
      m_zoneModeProvider->addListener(zoneModeListener);
    }
  }
}

void ZoneEditorBase::hideEvent(QHideEvent* event) {
  ImageViewBase::hideEvent(event);

  if (auto* mainWindow = dynamic_cast<QMainWindow*>(window())) {
    if (auto* zoneModeListener = core::Utils::castOrFindChild<ZoneModeListener*>(mainWindow->statusBar())) {
      m_zoneModeProvider->removeListener(zoneModeListener);
    }
  }
}

ZoneEditorBase::ZoneModeProvider::ZoneModeProvider(const ZoneEditorBase& parent) : m_parent(parent) {}

ZoneEditorBase::ZoneModeProvider::~ZoneModeProvider() {
  for (ZoneModeListener* listener : m_infoListeners) {
    listener->onZoneModeProviderStopped();
  }
}

void ZoneEditorBase::ZoneModeProvider::addListener(ZoneModeListener* listener) {
  listener->onZoneModeChanged(m_parent.context().getZoneCreationMode());
  m_infoListeners.push_back(listener);
}

void ZoneEditorBase::ZoneModeProvider::removeListener(ZoneModeListener* listener) {
  listener->onZoneModeProviderStopped();
  m_infoListeners.remove(listener);
}

void ZoneEditorBase::ZoneModeProvider::updateZoneMode() {
  ZoneCreationMode mode = m_parent.context().getZoneCreationMode();
  for (ZoneModeListener* listener : m_infoListeners) {
    listener->onZoneModeChanged(mode);
  }
}
