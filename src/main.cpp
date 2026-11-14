#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

#include <windows.h>
#include <dwmapi.h>

#include "backend.h"
#include "tray.h"

namespace {

constexpr auto kServerName = "ReworkTime-single-instance";

// Тёмный заголовок окна: Windows по умолчанию рисует светлый.
void applyDarkTitleBar(QWindow* window) {
    if (!window) return;
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    BOOL enabled = TRUE;
    constexpr DWORD kDark = 20;   // DWMWA_USE_IMMERSIVE_DARK_MODE
    if (FAILED(DwmSetWindowAttribute(hwnd, kDark, &enabled, sizeof(enabled)))) {
        constexpr DWORD kDarkOld = 19;
        DwmSetWindowAttribute(hwnd, kDarkOld, &enabled, sizeof(enabled));
    }
}

// Второй запуск не должен плодить счётчики: просим уже работающую копию
// показать окно и выходим.
bool notifyRunningInstance() {
    QLocalSocket socket;
    socket.connectToServer(QLatin1String(kServerName));
    if (!socket.waitForConnected(300)) return false;
    socket.write("show");
    socket.flush();
    socket.waitForBytesWritten(300);
    socket.disconnectFromServer();
    return true;
}

} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("ReworkTime"));
    app.setOrganizationName(QStringLiteral("REWORK"));
    app.setQuitOnLastWindowClosed(false);   // прячемся в трей, а не выходим

    if (notifyRunningInstance()) return 0;

    Backend backend;
    Tray tray;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
    engine.rootContext()->setContextProperty(QStringLiteral("tray"), &tray);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("ReworkTime"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty()) return 1;

    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if (window) {
        applyDarkTitleBar(window);
        tray.attach(window);

        QObject::connect(&tray, &Tray::showRequested, window, [window]() {
            window->show();
            window->raise();
            window->requestActivate();
        });
    }

    QLocalServer server;
    QLocalServer::removeServer(QLatin1String(kServerName));
    if (server.listen(QLatin1String(kServerName))) {
        QObject::connect(&server, &QLocalServer::newConnection, &server, [&server, window]() {
            while (QLocalSocket* client = server.nextPendingConnection()) {
                client->deleteLater();
                if (window) {
                    window->show();
                    window->raise();
                    window->requestActivate();
                }
            }
        });
    }

    return app.exec();
}
