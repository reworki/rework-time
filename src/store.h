#pragma once

#include <QString>
#include <QSqlDatabase>
#include <QVector>

// Хранилище сессий: одна открытая строка на приложение, при переключении она
// закрывается. Топ считается одним SQL-запросом по нужному отрезку времени.
class Store {
public:
    struct Row {
        QString app;
        qint64 seconds = 0;
    };

    enum class Period { Day, Week, Month, Year };

    Store();
    ~Store();

    bool open(const QString& filePath, QString* error);
    QString filePath() const { return filePath_; }

    // Продолжает открытую сессию этого приложения или закрывает старую и
    // открывает новую. ts — текущее время в секундах epoch.
    void tick(const QString& app, const QString& title, qint64 ts);

    // Закрывает открытую сессию (например, при выходе).
    void closeOpenSession(qint64 ts);

    QVector<Row> top(Period period, int limit, qint64 nowTs, qint64* totalSeconds) const;

    bool healthy() const { return healthy_; }

private:
    qint64 periodStart(Period period, qint64 nowTs) const;
    void ensureSchema();

    QSqlDatabase db_;
    QString filePath_;
    QString currentApp_;
    qint64 currentStart_ = 0;
    bool healthy_ = false;
};
