#include "store.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

#include "tracker.h"
#include <QVariant>

Store::Store() = default;

Store::~Store() {
    if (db_.isValid()) {
        db_.close();
    }
    const QString name = db_.connectionName();
    db_ = QSqlDatabase();
    if (!name.isEmpty()) QSqlDatabase::removeDatabase(name);
}

bool Store::open(const QString& filePath, QString* error) {
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    filePath_ = filePath;

    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("reworktime"));
    db_.setDatabaseName(filePath);
    if (!db_.open()) {
        if (error) *error = db_.lastError().text();
        healthy_ = false;
        return false;
    }

    QSqlQuery pragma(db_);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));

    ensureSchema();

    // склеиваем старые «сырые» имена с человеческими: WindowsTerminal -> Windows Terminal
    const auto aliases = Tracker::aliases();
    for (auto it = aliases.constBegin(); it != aliases.constEnd(); ++it) {
        QSqlQuery fix(db_);
        fix.prepare(QStringLiteral("UPDATE sessions SET app = ? WHERE app = ?"));
        fix.addBindValue(it.value());
        fix.addBindValue(it.key());
        fix.exec();
    }
    healthy_ = true;
    return true;
}

void Store::ensureSchema() {
    QSqlQuery query(db_);
    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS sessions ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  app TEXT NOT NULL,"
        "  title TEXT,"
        "  start_ts INTEGER NOT NULL,"
        "  end_ts INTEGER NOT NULL)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_sessions_start ON sessions(start_ts)"));
    query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_sessions_app ON sessions(app)"));
}

void Store::tick(const QString& app, const QString& title, qint64 ts) {
    if (!healthy_) return;

    if (app.isEmpty()) {
        closeOpenSession(ts);
        return;
    }

    if (app == currentApp_) {
        // heartbeat: продлеваем открытую сессию, иначе цифры замирают до
        // следующего переключения окна
        QSqlQuery beat(db_);
        beat.prepare(QStringLiteral("UPDATE sessions SET end_ts = ? WHERE app = ? AND start_ts = ?"));
        beat.addBindValue(ts);
        beat.addBindValue(currentApp_);
        beat.addBindValue(currentStart_);
        beat.exec();
        return;
    }

    closeOpenSession(ts);

    QSqlQuery insert(db_);
    insert.prepare(QStringLiteral("INSERT INTO sessions(app, title, start_ts, end_ts) VALUES(?, ?, ?, ?)"));
    insert.addBindValue(app);
    insert.addBindValue(title);
    insert.addBindValue(ts);
    insert.addBindValue(ts + 1);   // хотя бы секунда, чтобы строка появилась в топе сразу
    if (insert.exec()) {
        currentApp_ = app;
        currentStart_ = ts;
    }
}

void Store::closeOpenSession(qint64 ts) {
    if (!healthy_ || currentApp_.isEmpty()) return;

    QSqlQuery update(db_);
    update.prepare(QStringLiteral(
        "UPDATE sessions SET end_ts = ? WHERE app = ? AND start_ts = ?"));
    update.addBindValue(ts);
    update.addBindValue(currentApp_);
    update.addBindValue(currentStart_);
    update.exec();

    currentApp_.clear();
    currentStart_ = 0;
}

qint64 Store::periodStart(Period period, qint64 nowTs) const {
    const QDateTime now = QDateTime::fromSecsSinceEpoch(nowTs);
    const QDate today = now.date();
    QDate start = today;

    switch (period) {
    case Period::Day:
        break;
    case Period::Week:
        start = today.addDays(-(today.dayOfWeek() - 1));  // понедельник
        break;
    case Period::Month:
        start = QDate(today.year(), today.month(), 1);
        break;
    case Period::Year:
        start = QDate(today.year(), 1, 1);
        break;
    }

    return QDateTime(start, QTime(0, 0)).toSecsSinceEpoch();
}

QVector<Store::Row> Store::top(Period period, int limit, qint64 nowTs, qint64* totalSeconds) const {
    QVector<Row> rows;
    if (totalSeconds) *totalSeconds = 0;
    if (!healthy_) return rows;

    const qint64 from = periodStart(period, nowTs);

    QSqlQuery total(db_);
    total.prepare(QStringLiteral("SELECT COALESCE(SUM(end_ts - start_ts), 0) FROM sessions WHERE start_ts >= ?"));
    total.addBindValue(from);
    if (total.exec() && total.next() && totalSeconds) {
        *totalSeconds = total.value(0).toLongLong();
    }

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT app, SUM(end_ts - start_ts) AS spent FROM sessions "
        "WHERE start_ts >= ? GROUP BY app ORDER BY spent DESC LIMIT ?"));
    query.addBindValue(from);
    query.addBindValue(limit);
    if (!query.exec()) return rows;

    while (query.next()) {
        Row row;
        row.app = query.value(0).toString();
        row.seconds = query.value(1).toLongLong();
        if (row.seconds > 0) rows.push_back(row);
    }
    return rows;
}
