#pragma once

#include <QImage>
#include <QQuickImageProvider>

class Backend;

// image://appicon/<имя программы> — иконка exe, а если её нет, аккуратная
// буквенная заглушка в цвете приложения.
class IconProvider final : public QQuickImageProvider {
public:
    explicit IconProvider(Backend* backend);

    QImage requestImage(const QString& id, QSize* size, const QSize& requested) override;

private:
    static QImage placeholder(const QString& name, const QSize& requested);

    Backend* backend_ = nullptr;
};
