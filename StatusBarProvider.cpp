
#include "StatusBarProvider.h"
#include "StatusBarPanel.h"

std::unique_ptr<StatusBarProvider> StatusBarProvider::instance = nullptr;

StatusBarProvider* StatusBarProvider::getInstance() {
    if (instance == nullptr) {
        instance.reset(new StatusBarProvider());
    }

    return instance.get();
}

const QSizeF& StatusBarProvider::getPhysSize() const {
    return physSize;
}

void StatusBarProvider::setPhysSize(const QSizeF& physSize) {
    StatusBarProvider::physSize = physSize;
    physSizeChanged();
}

const QPointF& StatusBarProvider::getMousePos() const {
    return mousePos;
}

void StatusBarProvider::setMousePos(const QPointF& mousePos) {
    StatusBarProvider::mousePos = mousePos;
    mousePosChanged();
}

void StatusBarProvider::registerStatusBarPanel(const std::shared_ptr<StatusBarPanel>& statusBarPanel) {
    StatusBarProvider::statusBarPanel = statusBarPanel;
}

void StatusBarProvider::physSizeChanged() {
    if (auto statusBarPanel = this->statusBarPanel.lock()) {
        statusBarPanel->updatePhysSize(physSize);
    }
}

void StatusBarProvider::mousePosChanged() {
    if (auto statusBarPanel = this->statusBarPanel.lock()) {
        statusBarPanel->updateMousePos(mousePos);
    }
}
