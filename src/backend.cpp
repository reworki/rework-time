#include "backend.h"

#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QStandardPaths>
#include <QUrl>
#include <QTimer>
#include <QVariantMap>

#include <windows.h>
#include <psapi.h>

namespace {

QString appDataDir() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) dir = QDir::homePath() + QStringLiteral("/.reworktime");
    return dir;
}

QString exePathOf(DWORD pid) {
    if (!pid) return QString();
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return QString();
    wchar_t buffer[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    QString path;
    if (QueryFullProcessImageNameW(process, 0, buffer, &size)) {
        path = QString::fromWCharArray(buffer, int(size));
    }
    CloseHandle(process);
    return path;
}

// Имя «как в интерфейсе» -> путь к exe для запущенных процессов.
QHash<QString, QString> runningExePaths() {
    QHash<QString, QString> map;
    DWORD pids[4096] = {};
    DWORD needed = 0;
    if (!EnumProcesses(pids, sizeof(pids), &needed)) return map;

    const DWORD count = needed / sizeof(DWORD);
    for (DWORD i = 0; i < count; ++i) {
        const QString path = exePathOf(pids[i]);
        if (path.isEmpty()) continue;
        const QString base = QFileInfo(path).completeBaseName();
        map.insert(base, path);
        map.insert(Tracker::prettyName(base), path);
    }
    return map;
}

} // namespace

Backend::Backend(QObject* parent) : QObject(parent) {
    const QString dbPath = appDataDir() + QStringLiteral("/usage.db");
    QString error;
    if (!store_.open(dbPath, &error)) {
        status_ = QStringLiteral("база не открылась: ") + error;
    } else {
        status_ = QStringLiteral("пишу в ") + store_.filePath();
    }

    tickTimer_ = new QTimer(this);
    tickTimer_->setInterval(1000);
    connect(tickTimer_, &QTimer::timeout, this, &Backend::onTick);
    tickTimer_->start();

    refreshTimer_ = new QTimer(this);
    refreshTimer_->setInterval(1000);
    connect(refreshTimer_, &QTimer::timeout, this, &Backend::refresh);
    refreshTimer_->start();

    refresh();
}

Backend::~Backend() {
    store_.closeOpenSession(QDateTime::currentSecsSinceEpoch());
}

void Backend::onTick() {
    if (paused_) return;

    const Tracker::Sample sample = tracker_.take();
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    // Себя не считаем: пока открыто окно статистики, время не идёт никуда.
    if (sample.idle || sample.app.isEmpty() || sample.pid == GetCurrentProcessId()) {
        store_.closeOpenSession(now);
        return;
    }

    rememberPath(sample.app, sample.path);
    store_.tick(sample.app, sample.title, now);
}

void Backend::refresh() {
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    qint64 totalSeconds = 0;
    const QVector<Store::Row> rows = store_.top(period_, 5, now, &totalSeconds);

    QVariantList list;
    list.reserve(rows.size());
    QStringList missingIcons;
    for (const Store::Row& row : rows) {
        QVariantMap item;
        item.insert(QStringLiteral("app"), row.app);
        item.insert(QStringLiteral("seconds"), double(row.seconds));
        item.insert(QStringLiteral("time"), formatDuration(row.seconds));
        const QString icon = icons_.value(row.app);
        if (icon.isEmpty()) missingIcons.append(row.app);
        item.insert(QStringLiteral("icon"), icon);
        list.append(item);
    }

    top_ = list;
    total_ = rows.isEmpty() ? QStringLiteral("—") : formatDuration(totalSeconds);
    caption_ = captionFor(period_);
    status_ = (paused_ ? QStringLiteral("пауза · ") : QStringLiteral("пишу в ") + store_.filePath()) +
              QStringLiteral(" · ") + QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));

    emit changed();

    // иконки достаём отложенно: первый кадр не ждёт файловых операций
    if (!missingIcons.isEmpty()) {
        QMetaObject::invokeMethod(this, [this, missingIcons]() {
            prewarmIcons(missingIcons);
        }, Qt::QueuedConnection);
    }
}

void Backend::setPeriod(int period) {
    const Store::Period next = Store::Period(qBound(0, period, 3));
    if (next == period_) return;
    period_ = next;
    refresh();
}

void Backend::setPaused(bool paused) {
    if (paused_ == paused) return;
    paused_ = paused;
    if (paused_) store_.closeOpenSession(QDateTime::currentSecsSinceEpoch());
    emit pausedChanged();
    refresh();
}

void Backend::rememberPath(const QString& app, const QString& path) {
    if (app.isEmpty() || path.isEmpty()) return;
    if (!pathByApp_.contains(app)) pathByApp_.insert(app, path);
}

QString Backend::iconDataUrl(const QString& path) {
    // Иконку кладём PNG-файлом в кэш: QML надёжно грузит file://, а data-URL
    // в Image иногда просто не отрисовывается.
    if (path.isEmpty() || !QFileInfo::exists(path)) return QString();

    const QString cacheDir = appDataDir() + QStringLiteral("/icons");
    QDir().mkpath(cacheDir);
    const QString key = QString::number(qHash(path), 16);
    const QString pngPath = cacheDir + QLatin1Char('/') + key + QStringLiteral(".png");

    if (!QFileInfo::exists(pngPath)) {
        QFileIconProvider provider;
        const QIcon icon = provider.icon(QFileInfo(path));
        if (icon.isNull()) return QString();
        QImage image = icon.pixmap(48, 48).toImage();
        if (image.isNull()) return QString();
        image = image.convertToFormat(QImage::Format_ARGB32);
        if (!image.save(pngPath, "PNG")) return QString();
    }

    return QUrl::fromLocalFile(pngPath).toString();
}

void Backend::prewarmIcons(const QStringList& apps) {
    for (const QString& app : apps) {
        if (icons_.contains(app)) continue;

        QString path = pathByApp_.value(app);
        if (path.isEmpty() || !QFileInfo::exists(path)) {
            const QHash<QString, QString> running = runningExePaths();
            path = running.value(app);
        }
        if (path.isEmpty()) continue;

        const QString url = iconDataUrl(path);
        if (url.isEmpty()) continue;
        icons_.insert(app, url);
    }
    emit changed();
}

QString Backend::formatDuration(qint64 seconds) {
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

QString Backend::captionFor(Store::Period period) {
    switch (period) {
    case Store::Period::Day: return QStringLiteral("ВСЕГО ЗА СЕГОДНЯ");
    case Store::Period::Week: return QStringLiteral("ВСЕГО ЗА НЕДЕЛЮ");
    case Store::Period::Month: return QStringLiteral("ВСЕГО ЗА МЕСЯЦ");
    case Store::Period::Year: return QStringLiteral("ВСЕГО ЗА ГОД");
    }
    return QStringLiteral("ВСЕГО");
}
