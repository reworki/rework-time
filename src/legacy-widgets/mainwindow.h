#pragma once

#include <QHash>
#include <QIcon>
#include <QWidget>

#include "store.h"
#include "tracker.h"

class PodiumWidget;

class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;
class QVBoxLayout;
class QSystemTrayIcon;

// Окно: период сверху, суммарные часы, топ-5 с иконками и барами, статус снизу.
class MainWindow final : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    static QIcon appIcon();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    struct RowUi {
        QWidget* host = nullptr;
        QLabel* icon = nullptr;
        QLabel* name = nullptr;
        QLabel* spent = nullptr;
        QProgressBar* bar = nullptr;
    };

    void buildUi();
    void buildTray();
    void startTimers();
    void onTick();
    void refresh();
    void setPeriod(Store::Period period);
    void renderRows(const QVector<Store::Row>& rows, qint64 totalSeconds);
    RowUi createRow();
    QIcon iconFor(const QString& app);
    static QString formatDuration(qint64 seconds);
    static QString periodCaption(Store::Period period);

    Tracker tracker_;
    Store store_;
    QTimer* tickTimer_ = nullptr;
    QTimer* refreshTimer_ = nullptr;
    QVBoxLayout* rowsLayout_ = nullptr;
    PodiumWidget* podium_ = nullptr;
    QLabel* emptyLabel_ = nullptr;
    QLabel* totalLabel_ = nullptr;
    QLabel* captionLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* pauseButton_ = nullptr;
    QSystemTrayIcon* tray_ = nullptr;
    QVector<RowUi> rowWidgets_;
    QHash<QString, QIcon> iconCache_;
    QHash<QString, QString> pathByApp_;
    Store::Period period_ = Store::Period::Day;
    bool paused_ = false;
    bool quitRequested_ = false;
};
