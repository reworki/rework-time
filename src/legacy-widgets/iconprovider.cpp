#include "iconprovider.h"

#include <QPainter>
#include <QPainterPath>

#include "backend.h"

IconProvider::IconProvider(Backend* backend)
    : QQuickImageProvider(QQuickImageProvider::Image), backend_(backend) {}

QImage IconProvider::requestImage(const QString& id, QSize* size, const QSize& requested) {
    const QString name = QString::fromUtf8(QByteArray::fromPercentEncoding(id.toUtf8()));
    QImage image = backend_ ? backend_->iconFor(name) : QImage();

    const QSize target = requested.isValid() ? requested : QSize(64, 64);
    if (image.isNull()) image = placeholder(name, target);
    else image = image.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    if (size) *size = image.size();
    return image;
}

QImage IconProvider::placeholder(const QString& name, const QSize& requested) {
    const QSize size = requested.isValid() ? requested : QSize(64, 64);
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#1b2433")));
    painter.drawRoundedRect(QRectF(0, 0, size.width(), size.height()), size.width() * 0.22,
                            size.height() * 0.22);

    const QChar first = name.isEmpty() ? QChar('?') : name.at(0).toUpper();
    QFont font;
    font.setPointSizeF(size.height() * 0.45);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(QColor(QStringLiteral("#8fa3ff")));
    painter.drawText(QRectF(0, 0, size.width(), size.height()), Qt::AlignCenter, QString(first));
    painter.end();
    return image;
}
