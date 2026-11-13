#pragma once

#include <QIcon>
#include <QVector>
#include <QWidget>

class QVariantAnimation;
class QTimer;

// Подиум победителей: три колонки, которые растут вверх, корона над первым
// местом. Высоты анимируются, поэтому смена топа выглядит как соревнование,
// а не как перерисованный список.
class PodiumWidget final : public QWidget {
    Q_OBJECT

public:
    struct Entry {
        QString name;
        QIcon icon;
        qint64 seconds = 0;
    };

    explicit PodiumWidget(QWidget* parent = nullptr);
    ~PodiumWidget() override;

    void setEntries(const QVector<Entry>& entries);

    QSize sizeHint() const override { return QSize(720, 320); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void animateTo(const QVector<Entry>& entries);
    void paintColumn(QPainter& painter, int rank, const QRectF& area, qreal progress);
    void paintCrown(QPainter& painter, const QRectF& area);

    QVector<Entry> entries_;
    QVector<qreal> shownSeconds_;         // плавно догоняющие значения
    QVector<QVariantAnimation*> anims_;
    QTimer* bobTimer_ = nullptr;
    qreal bobPhase_ = 0.0;
};
