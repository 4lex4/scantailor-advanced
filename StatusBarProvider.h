
#ifndef SCANTAILOR_STATUSBARPROVIDER_H
#define SCANTAILOR_STATUSBARPROVIDER_H

#include <memory>
#include <QPointF>
#include <QSizeF>

class StatusBarPanel;

class StatusBarProvider {
private:
    static std::unique_ptr<StatusBarProvider> instance;

    std::weak_ptr<StatusBarPanel> statusBarPanel;
    QSizeF physSize;
    QPointF mousePos;

    StatusBarProvider() = default;

public:
    static StatusBarProvider* getInstance();

    void registerStatusBarPanel(const std::shared_ptr<StatusBarPanel>& statusBarPanel);

    const QSizeF& getPhysSize() const;

    void setPhysSize(const QSizeF& physSize);

    const QPointF& getMousePos() const;

    void setMousePos(const QPointF& mousePos);

protected:
    void physSizeChanged();

    void mousePosChanged();
};


#endif //SCANTAILOR_STATUSBARPROVIDER_H
