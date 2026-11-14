#pragma once

#include <QIcon>
#include <QObject>

class QQuickWindow;
class QSystemTrayIcon;
class QMenu;

// Иконка в трее: закрытие окна прячет его сюда, а не выходит из приложения.
class Tray final : public QObject {
    Q_OBJECT

public:
    explicit Tray(QObject* parent = nullptr);
    ~Tray() override;

    // Окно появляется позже, после загрузки QML.
    void attach(QQuickWindow* window);
    bool available() const;

    Q_INVOKABLE void hideToTray();

signals:
    void showRequested();

private:
    void buildMenu();
    static QIcon makeIcon();

    QSystemTrayIcon* icon_ = nullptr;
    QMenu* menu_ = nullptr;
    QQuickWindow* window_ = nullptr;
};
