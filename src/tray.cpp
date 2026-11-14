#include "tray.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QQuickWindow>
#include <QSystemTrayIcon>

Tray::Tray(QObject* parent) : QObject(parent) {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) return;

    icon_ = new QSystemTrayIcon(makeIcon(), this);
    icon_->setToolTip(QStringLiteral("Rework Time — считаю время в программах"));
    buildMenu();
    icon_->show();

    connect(icon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            emit showRequested();
        }
    });
}

Tray::~Tray() = default;

void Tray::attach(QQuickWindow* window) {
    window_ = window;
    if (!icon_ || !window_) return;

    // крестик на окне прячет в трей, а не убивает приложение
    connect(window_, &QQuickWindow::closing, this, [this](QQuickCloseEvent*) {
        window_->hide();
    });
}

bool Tray::available() const { return icon_ != nullptr; }

void Tray::hideToTray() {
    if (window_) window_->hide();
}

void Tray::buildMenu() {
    menu_ = new QMenu();
    menu_->addAction(QStringLiteral("Показать"), this, [this]() { emit showRequested(); });
    menu_->addSeparator();
    menu_->addAction(QStringLiteral("Выход"), this, []() { QCoreApplication::quit(); });
    icon_->setContextMenu(menu_);
}

QIcon Tray::makeIcon() {
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#101520")));
    painter.drawRoundedRect(QRectF(2, 2, 60, 60), 14, 14);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(QStringLiteral("#4d6bfe")), 4));
    painter.drawEllipse(QRectF(14, 12, 36, 36));

    painter.setPen(QPen(QColor(QStringLiteral("#e5c07b")), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(32, 22), QPointF(32, 30));
    painter.setPen(QPen(QColor(QStringLiteral("#98c379")), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(32, 30), QPointF(40, 34));

    painter.end();
    return QIcon(pixmap);
}
