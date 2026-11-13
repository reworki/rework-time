#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

#include <windows.h>
#include <dwmapi.h>

#include "backend.h"

namespace {

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

} // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("ReworkTime"));
    app.setOrganizationName(QStringLiteral("REWORK"));

    Backend backend;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("ReworkTime"), QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty()) return 1;

    if (auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().first())) {
        applyDarkTitleBar(window);
    }

    return app.exec();
}
