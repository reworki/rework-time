#include "mainwindow.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QProgressBar>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVBoxLayout>

#include <windows.h>

#include "podium.h"

namespace {

const char* kStyle = R"(
QWidget { background: #0b0e14; color: #d7dee8; font-family: Consolas, 'Courier New', monospace; }
QLabel#title { color: #4d6bfe; font-size: 21px; letter-spacing: 3px; }
QLabel#subtitle { color: #5d6875; font-size: 11px; letter-spacing: 1px; }
QLabel#caption { color: #6a7684; font-size: 11px; letter-spacing: 2px; }
QLabel#total { color: #f0f4fa; font-size: 34px; }
QLabel#app { color: #d7dee8; font-size: 13px; }
QLabel#spent { color: #8fa3ff; font-size: 13px; }
QLabel#status { color: #55606d; font-size: 11px; }
QLabel#empty { color: #55606d; font-size: 12px; }

QFrame#card { background: #101520; border: 1px solid #1b2433; border-radius: 12px; }

QPushButton { background: transparent; border: 1px solid #1b2433; border-radius: 8px;
              padding: 6px 14px; color: #8b949e; font-size: 12px; }
QPushButton:hover { border-color: #2f4bd8; color: #c9d1d9; background: #121a2b; }
QPushButton:checked { background: #26379e; border-color: #4d6bfe; color: #eaefff; }

QPushButton#pause { padding: 6px 12px; font-size: 13px; }
QPushButton#pause:checked { background: #2a1d12; border-color: #d19a66; color: #e5c07b; }

QProgressBar { background: #121a26; border: none; border-radius: 4px; }
QProgressBar::chunk { border-radius: 4px; }
)";

QString appDataDir() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) dir = QDir::homePath() + QStringLiteral("/.reworktime");
    return dir;
}

// Цвет полоски по месту в топе: золото / серебро / бронза / акцент.
QString barColor(int rank) {
    switch (rank) {
    case 0: return QStringLiteral("#e5c07b");
    case 1: return QStringLiteral("#b8c4d4");
    case 2: return QStringLiteral("#d19a66");
    default: return QStringLiteral("#4d6bfe");
    }
}

} // namespace

QIcon MainWindow::appIcon() {
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#101520")));
    painter.drawRoundedRect(QRectF(2, 2, 60, 60), 14, 14);

    // циферблат
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

MainWindow::MainWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle(QStringLiteral("REWORK TIME"));
    setWindowIcon(appIcon());
    resize(1040, 820);
    setMinimumSize(880, 700);
    setStyleSheet(QLatin1String(kStyle));

    buildUi();
    startTimers();

    const QString dbPath = appDataDir() + QStringLiteral("/usage.db");
    QString error;
    if (!store_.open(dbPath, &error)) {
        statusLabel_->setText(QStringLiteral("база не открылась: ") + error);
    } else {
        statusLabel_->setText(QStringLiteral("пишу в ") + store_.filePath());
    }

    buildTray();
    refresh();
}

MainWindow::~MainWindow() {
    store_.closeOpenSession(QDateTime::currentSecsSinceEpoch());
}

void MainWindow::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 18, 22, 14);
    root->setSpacing(14);

    // --- шапка -------------------------------------------------------------
    auto* header = new QHBoxLayout();
    header->setSpacing(12);

    auto* logo = new QLabel(this);
    logo->setPixmap(appIcon().pixmap(34, 34));
    logo->setFixedSize(34, 34);
    header->addWidget(logo);

    auto* titles = new QVBoxLayout();
    titles->setSpacing(2);
    auto* title = new QLabel(QStringLiteral("REWORK TIME"), this);
    title->setObjectName(QStringLiteral("title"));
    auto* subtitle = new QLabel(QStringLiteral("экранное время"), this);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    titles->addWidget(title);
    titles->addWidget(subtitle);
    header->addLayout(titles);
    header->addStretch(1);

    pauseButton_ = new QPushButton(QStringLiteral("❚❚  пауза"), this);
    pauseButton_->setObjectName(QStringLiteral("pause"));
    pauseButton_->setCheckable(true);
    pauseButton_->setCursor(Qt::PointingHandCursor);
    connect(pauseButton_, &QPushButton::toggled, this, [this](bool checked) {
        paused_ = checked;
        pauseButton_->setText(checked ? QStringLiteral("▶  дальше")
                                      : QStringLiteral("❚❚  пауза"));
        statusLabel_->setText(checked ? QStringLiteral("сбор приостановлен")
                                      : QStringLiteral("пишу в ") + store_.filePath());
    });
    header->addWidget(pauseButton_);
    root->addLayout(header);

    // --- периоды -----------------------------------------------------------
    auto* periods = new QHBoxLayout();
    periods->setSpacing(8);
    auto* group = new QButtonGroup(this);
    group->setExclusive(true);

    struct Item { const char* text; Store::Period period; };
    const Item items[] = {
        {"день", Store::Period::Day},
        {"неделя", Store::Period::Week},
        {"месяц", Store::Period::Month},
        {"год", Store::Period::Year},
    };
    for (const Item& item : items) {
        auto* button = new QPushButton(QString::fromUtf8(item.text), this);
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setChecked(item.period == period_);
        group->addButton(button);
        periods->addWidget(button);
        const Store::Period period = item.period;
        connect(button, &QPushButton::clicked, this, [this, period]() { setPeriod(period); });
    }
    periods->addStretch(1);
    root->addLayout(periods);

    // --- карточка с суммой -------------------------------------------------
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("card"));
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 14, 18, 16);
    cardLayout->setSpacing(2);

    captionLabel_ = new QLabel(QStringLiteral("ВСЕГО"), card);
    captionLabel_->setObjectName(QStringLiteral("caption"));
    totalLabel_ = new QLabel(QStringLiteral("—"), card);
    totalLabel_->setObjectName(QStringLiteral("total"));
    cardLayout->addWidget(captionLabel_);
    cardLayout->addWidget(totalLabel_);
    root->addWidget(card);

    // --- подиум победителей ------------------------------------------------
    podium_ = new PodiumWidget(this);
    root->addWidget(podium_, 3);

    // --- строки ------------------------------------------------------------
    auto* rowsHost = new QWidget(this);
    rowsLayout_ = new QVBoxLayout(rowsHost);
    rowsLayout_->setContentsMargins(0, 0, 0, 0);
    rowsLayout_->setSpacing(12);

    emptyLabel_ = new QLabel(QStringLiteral("пока пусто — работай, я считаю"), rowsHost);
    emptyLabel_->setObjectName(QStringLiteral("empty"));
    rowsLayout_->addWidget(emptyLabel_);
    rowsLayout_->addStretch(1);
    root->addWidget(rowsHost, 2);

    statusLabel_ = new QLabel(this);
    statusLabel_->setObjectName(QStringLiteral("status"));
    root->addWidget(statusLabel_);
}

void MainWindow::startTimers() {
    tickTimer_ = new QTimer(this);
    tickTimer_->setInterval(1000);
    connect(tickTimer_, &QTimer::timeout, this, &MainWindow::onTick);
    tickTimer_->start();

    refreshTimer_ = new QTimer(this);
    refreshTimer_->setInterval(1000);
    connect(refreshTimer_, &QTimer::timeout, this, &MainWindow::refresh);
    refreshTimer_->start();
}

void MainWindow::onTick() {
    if (paused_) return;

    const Tracker::Sample sample = tracker_.take();
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    // Себя не считаем: пока открыто это окно, время не идёт никуда.
    if (sample.idle || sample.app.isEmpty() || sample.pid == GetCurrentProcessId()) {
        store_.closeOpenSession(now);
        return;
    }
    // запоминаем путь к exe: из него берём настоящую иконку программы
    if (!sample.path.isEmpty()) pathByApp_.insert(sample.app, sample.path);
    store_.tick(sample.app, sample.title, now);
}

void MainWindow::refresh() {
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    qint64 total = 0;
    const QVector<Store::Row> rows = store_.top(period_, 5, now, &total);
    renderRows(rows, total);
    statusLabel_->setText((paused_ ? QStringLiteral("пауза · ")
                                   : QStringLiteral("пишу в ") + store_.filePath()) +
                          QStringLiteral(" · ") +
                          QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")));
}

QIcon MainWindow::iconFor(const QString& app) {
    const auto cached = iconCache_.constFind(app);
    if (cached != iconCache_.constEnd()) return cached.value();

    QIcon icon;
    const QString path = pathByApp_.value(app);
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        QFileIconProvider provider;
        icon = provider.icon(QFileInfo(path));
    }
    if (icon.isNull()) icon = style()->standardIcon(QStyle::SP_FileIcon);
    iconCache_.insert(app, icon);
    return icon;
}

MainWindow::RowUi MainWindow::createRow() {
    RowUi row;

    auto* host = new QWidget(this);
    auto* box = new QVBoxLayout(host);
    box->setContentsMargins(0, 0, 0, 0);
    box->setSpacing(6);

    auto* line = new QHBoxLayout();
    line->setSpacing(10);

    row.icon = new QLabel(host);
    row.icon->setFixedSize(22, 22);
    row.icon->setScaledContents(true);

    row.name = new QLabel(host);
    row.name->setObjectName(QStringLiteral("app"));

    row.spent = new QLabel(host);
    row.spent->setObjectName(QStringLiteral("spent"));
    row.spent->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    line->addWidget(row.icon, 0);
    line->addWidget(row.name, 1);
    line->addWidget(row.spent, 0);

    row.bar = new QProgressBar(host);
    row.bar->setTextVisible(false);
    row.bar->setRange(0, 1);
    row.bar->setValue(0);
    row.bar->setFixedHeight(7);

    box->addLayout(line);
    box->addWidget(row.bar);
    host->hide();

    const int insertAt = rowsLayout_->count() > 0 ? rowsLayout_->count() - 1 : 0;
    rowsLayout_->insertWidget(insertAt, host);

    row.host = host;
    return row;
}

void MainWindow::renderRows(const QVector<Store::Row>& rows, qint64 totalSeconds) {
    captionLabel_->setText(QStringLiteral("ВСЕГО · ") + periodCaption(period_).toUpper());
    totalLabel_->setText(rows.isEmpty() ? QStringLiteral("—") : formatDuration(totalSeconds));
    emptyLabel_->setVisible(rows.isEmpty());

    while (rowWidgets_.size() < rows.size()) {
        rowWidgets_.push_back(createRow());
    }

    const qint64 maxSeconds = rows.isEmpty() || rows.first().seconds <= 0 ? 1 : rows.first().seconds;

    // подиум: три верхних места, иконки и растущие колонки
    QVector<PodiumWidget::Entry> podiumEntries;
    podiumEntries.reserve(rows.size());
    for (const Store::Row& row : rows) {
        PodiumWidget::Entry entry;
        entry.name = row.app;
        entry.icon = iconFor(row.app);
        entry.seconds = row.seconds;
        podiumEntries.push_back(entry);
    }
    if (podium_) podium_->setEntries(podiumEntries);

    for (int i = 0; i < rowWidgets_.size(); ++i) {
        RowUi& ui = rowWidgets_[i];
        if (i >= rows.size()) {
            ui.host->hide();
            continue;
        }
        const Store::Row& row = rows.at(i);
        ui.icon->setPixmap(iconFor(row.app).pixmap(22, 22));
        ui.name->setText(row.app);
        ui.spent->setText(formatDuration(row.seconds));
        ui.bar->setRange(0, int(maxSeconds));
        ui.bar->setValue(int(row.seconds));
        ui.bar->setStyleSheet(
            QStringLiteral("QProgressBar { background: #121a26; border: none; border-radius: 4px; }"
                           "QProgressBar::chunk { background: %1; border-radius: 4px; }")
                .arg(barColor(i)));
        ui.host->show();
    }
}

QString MainWindow::periodCaption(Store::Period period) {
    switch (period) {
    case Store::Period::Day: return QStringLiteral("за сегодня");
    case Store::Period::Week: return QStringLiteral("за неделю");
    case Store::Period::Month: return QStringLiteral("за месяц");
    case Store::Period::Year: return QStringLiteral("за год");
    }
    return QString();
}

QString MainWindow::formatDuration(qint64 seconds) {
    if (seconds < 60) return QStringLiteral("%1 с").arg(seconds);

    const qint64 minutes = seconds / 60;
    if (minutes < 60) {
        const qint64 rest = seconds % 60;
        return rest ? QStringLiteral("%1 мин %2 с").arg(minutes).arg(rest)
                    : QStringLiteral("%1 мин").arg(minutes);
    }

    const qint64 hours = minutes / 60;
    const qint64 restMinutes = minutes % 60;
    return restMinutes ? QStringLiteral("%1 ч %2 мин").arg(hours).arg(restMinutes)
                       : QStringLiteral("%1 ч").arg(hours);
}

void MainWindow::setPeriod(Store::Period period) {
    period_ = period;
    refresh();
}

void MainWindow::buildTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) return;

    tray_ = new QSystemTrayIcon(appIcon(), this);
    tray_->setToolTip(QStringLiteral("REWORK TIME — считаю время в программах"));

    auto* menu = new QMenu(this);
    menu->addAction(QStringLiteral("Показать"), this, [this]() {
        showNormal();
        raise();
        activateWindow();
    });
    menu->addAction(QStringLiteral("Открыть папку с базой"), this, [this]() {
        QProcess::startDetached(QStringLiteral("explorer.exe"),
                                {QDir(store_.filePath()).absolutePath().replace(QLatin1Char('/'),
                                                                               QLatin1Char('\\'))});
    });
    menu->addSeparator();
    menu->addAction(QStringLiteral("Выход"), this, [this]() {
        quitRequested_ = true;
        close();
    });

    tray_->setContextMenu(menu);
    connect(tray_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            showNormal();
            raise();
            activateWindow();
        }
    });
    tray_->show();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (quitRequested_ || !tray_) {
        store_.closeOpenSession(QDateTime::currentSecsSinceEpoch());
        event->accept();
        return;
    }

    hide();
    tray_->showMessage(QStringLiteral("REWORK TIME"),
                       QStringLiteral("Продолжаю считать в трее"),
                       QSystemTrayIcon::Information, 2000);
    event->ignore();
}
