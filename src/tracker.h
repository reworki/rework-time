#pragma once

#include <QHash>
#include <QString>

// Опрос активного окна через WinAPI. Раз в секунду: кто сейчас на переднем
// плане и трогал ли пользователь мышь/клавиатуру.
class Tracker {
public:
    struct Sample {
        QString app;        // человекочитаемое имя программы
        QString path;       // полный путь к exe (для иконки)
        QString title;      // заголовок окна
        quint32 pid = 0;    // pid процесса активного окна
        bool idle = false;  // давно не было ввода
    };

    explicit Tracker(int idleSeconds = 120);

    Sample take() const;

    // Одно место правды для имён: и сэмплер, и миграция базы берут отсюда.
    static const QHash<QString, QString>& aliases();
    static QString prettyName(const QString& baseName);

private:
    int idleLimitSeconds_;
};
