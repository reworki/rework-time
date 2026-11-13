#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>

#include "store.h"
#include "tracker.h"

class QTimer;

// Мост между C++ и QML: сэмплер, база и агрегаты живут здесь, QML только рисует.
// Иконки программ отдаются готовыми data-URL строками — никаких провайдеров
// картинок и фоновых потоков, из-за которых приложение падало.
class Backend final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList top READ top NOTIFY changed)
    Q_PROPERTY(QString total READ total NOTIFY changed)
    Q_PROPERTY(QString caption READ caption NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(int period READ period WRITE setPeriod NOTIFY changed)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)

public:
    explicit Backend(QObject* parent = nullptr);
    ~Backend() override;

    QVariantList top() const { return top_; }
    QString total() const { return total_; }
    QString caption() const { return caption_; }
    QString status() const { return status_; }
    int period() const { return int(period_); }
    bool paused() const { return paused_; }

    void setPeriod(int period);
    void setPaused(bool paused);

signals:
    void changed();
    void pausedChanged();

private slots:
    void onTick();
    void refresh();

private:
    static QString formatDuration(qint64 seconds);
    static QString captionFor(Store::Period period);
    static QString iconDataUrl(const QString& path);

    void prewarmIcons(const QStringList& apps);
    void rememberPath(const QString& app, const QString& path);

    Tracker tracker_;
    Store store_;
    QTimer* tickTimer_ = nullptr;
    QTimer* refreshTimer_ = nullptr;

    QVariantList top_;
    QString total_ = QStringLiteral("—");
    QString caption_ = QStringLiteral("ВСЕГО ЗА СЕГОДНЯ");
    QString status_ = QStringLiteral("запускаюсь");
    Store::Period period_ = Store::Period::Day;
    bool paused_ = false;

    QHash<QString, QString> pathByApp_;   // имя -> exe
    QHash<QString, QString> icons_;       // имя -> data:image/png;base64,...
};
