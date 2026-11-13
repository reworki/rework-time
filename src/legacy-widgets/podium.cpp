#include "podium.h"

#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QVariantAnimation>

namespace {

const QColor kBg(QStringLiteral("#0f1420"));
const QColor kText(QStringLiteral("#d7dee8"));
const QColor kDim(QStringLiteral("#6a7684"));
const QColor kGold(QStringLiteral("#e5c07b"));
const QColor kSilver(QStringLiteral("#b8c4d4"));
const QColor kBronze(QStringLiteral("#d19a66"));

QColor rankColor(int rank) {
    switch (rank) {
    case 0: return kGold;
    case 1: return kSilver;
    default: return kBronze;
    }
}

QString humanTime(qint64 seconds) {
    if (seconds < 60) return QStringLiteral("%1 с").arg(seconds);
    const qint64 minutes = seconds / 60;
    if (minutes < 60) return QStringLiteral("%1 мин").arg(minutes);
    const qint64 hours = minutes / 60;
    return QStringLiteral("%1 ч %2 мин").arg(hours).arg(minutes % 60);
}

// Порядок на подиуме: 2 место слева, 1 в центре, 3 справа.
int slotForRank(int rank) {
    switch (rank) {
    case 0: return 1;
    case 1: return 0;
    default: return 2;
    }
}

} // namespace

PodiumWidget::PodiumWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(300);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    for (int i = 0; i < 3; ++i) {
        auto* anim = new QVariantAnimation(this);
        anim->setDuration(900);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, &QVariantAnimation::valueChanged, this, [this, i](const QVariant& value) {
            if (i < shownSeconds_.size()) {
                shownSeconds_[i] = value.toReal();
                update();
            }
        });
        anims_.push_back(anim);
    }

    // лёгкое покачивание короны: живёт постоянно, почти не жрёт
    bobTimer_ = new QTimer(this);
    bobTimer_->setInterval(40);
    connect(bobTimer_, &QTimer::timeout, this, [this]() {
        bobPhase_ += 0.09;
        if (isVisible()) update();
    });
    bobTimer_->start();
}

PodiumWidget::~PodiumWidget() = default;

void PodiumWidget::setEntries(const QVector<Entry>& entries) {
    entries_ = entries;
    while (shownSeconds_.size() < 3) shownSeconds_.push_back(0.0);
    animateTo(entries);
}

void PodiumWidget::animateTo(const QVector<Entry>& entries) {
    for (int rank = 0; rank < anims_.size(); ++rank) {
        const qreal target = rank < entries.size() ? qreal(entries[rank].seconds) : 0.0;
        QVariantAnimation* anim = anims_[rank];
        anim->stop();
        anim->setStartValue(shownSeconds_[rank]);
        anim->setEndValue(target);
        anim->start();
    }
}

void PodiumWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), kBg);

    const qreal labelZone = 56.0;
    const qreal baseY = height() - labelZone;
    const qreal topPad = 44.0;                 // место под корону
    const qreal maxHeight = qMax<qreal>(80.0, baseY - topPad - 16.0);

    qreal maxSeconds = 1.0;
    for (const Entry& entry : entries_) maxSeconds = qMax(maxSeconds, qreal(entry.seconds));

    const int columns = qMin(3, entries_.size());
    const qreal totalWidth = width() - 32.0;
    const qreal columnWidth = qMin<qreal>(170.0, totalWidth / 3.4);
    const qreal gap = qMin<qreal>(36.0, (totalWidth - columnWidth * 3) / 3.0);

    for (int rank = 0; rank < columns; ++rank) {
        const int slot = slotForRank(rank);
        const qreal centerX = width() / 2.0 + (slot - 1) * (columnWidth + gap);
        const qreal ratio = qBound<qreal>(0.0, shownSeconds_.value(rank) / maxSeconds, 1.0);
        const qreal columnHeight = 72.0 + ratio * (maxHeight - 72.0);

        const QRectF area(centerX - columnWidth / 2.0, baseY - columnHeight, columnWidth, columnHeight);
        paintColumn(painter, rank, area, ratio);

        // подпись под подиумом: имя и время
        const QRectF caption(centerX - columnWidth / 2.0 - 12.0, baseY + 8.0, columnWidth + 24.0, 20.0);
        QFont captionFont = font();
        captionFont.setPointSizeF(9.5);
        painter.setFont(captionFont);
        painter.setPen(kText);
        const QString name = entries_[rank].name;
        const QString elided = QFontMetrics(captionFont).elidedText(name, Qt::ElideRight, int(caption.width()));
        painter.drawText(caption, Qt::AlignHCenter | Qt::AlignTop, elided);

        QFont timeFont = font();
        timeFont.setPointSizeF(9.0);
        timeFont.setBold(true);
        painter.setFont(timeFont);
        painter.setPen(rankColor(rank));
        painter.drawText(caption.translated(0, 19), Qt::AlignHCenter | Qt::AlignTop,
                         humanTime(qint64(shownSeconds_.value(rank))));
    }

    if (columns > 0) {
        const qreal centerX = width() / 2.0;
        const qreal columnHeight = 72.0 + qBound<qreal>(0.0, shownSeconds_.value(0) / maxSeconds, 1.0) * (maxHeight - 72.0);
        paintCrown(painter, QRectF(centerX - 28.0, baseY - columnHeight - 52.0, 56.0, 40.0));
    }
}

void PodiumWidget::paintColumn(QPainter& painter, int rank, const QRectF& area, qreal progress) {
    const QColor color = rankColor(rank);

    // свечение под колонкой
    QRadialGradient glow(area.center().x(), area.bottom(), area.width());
    QColor soft = color;
    soft.setAlpha(38);
    glow.setColorAt(0.0, soft);
    glow.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.fillRect(area.adjusted(-30, -30, 30, 10), glow);

    QLinearGradient fill(area.topLeft(), area.bottomLeft());
    QColor top = color;
    top.setAlpha(235);
    QColor bottom = color.darker(155);
    bottom.setAlpha(190);
    fill.setColorAt(0.0, top);
    fill.setColorAt(1.0, bottom);

    QPainterPath path;
    path.addRoundedRect(area, 12, 12);
    painter.fillPath(path, fill);

    // блик сверху
    QLinearGradient sheen(area.topLeft(), area.topLeft() + QPointF(0, area.height() * 0.45));
    sheen.setColorAt(0.0, QColor(255, 255, 255, 46));
    sheen.setColorAt(1.0, QColor(255, 255, 255, 0));
    QPainterPath clipped;
    clipped.addRoundedRect(area, 12, 12);
    painter.save();
    painter.setClipPath(clipped);
    painter.fillRect(area, sheen);
    painter.restore();

    // место + иконка программы
    QFont rankFont = font();
    rankFont.setPointSizeF(28.0);
    rankFont.setBold(true);
    painter.setFont(rankFont);
    QColor ghost(12, 16, 26, 120);
    painter.setPen(ghost);
    painter.drawText(area.adjusted(0, -6, 0, -6), Qt::AlignCenter, QString::number(rank + 1));

    if (rank < entries_.size() && !entries_[rank].icon.isNull()) {
        const QRectF iconRect(area.center().x() - 16.0, area.top() + 14.0, 32.0, 32.0);
        painter.drawPixmap(iconRect.toRect(), entries_[rank].icon.pixmap(32, 32));
    }

    Q_UNUSED(progress);
}

void PodiumWidget::paintCrown(QPainter& painter, const QRectF& area) {
    const qreal bob = qSin(bobPhase_) * 3.0;
    const QRectF crown = area.translated(0, bob);

    // мягкое золотое свечение
    QRadialGradient glow(crown.center(), crown.width());
    QColor gold = kGold;
    gold.setAlpha(70);
    glow.setColorAt(0.0, gold);
    glow.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.fillRect(crown.adjusted(-24, -24, 24, 24), glow);

    QPainterPath crownPath;
    const qreal x = crown.left();
    const qreal y = crown.top();
    const qreal w = crown.width();
    const qreal h = crown.height();

    crownPath.moveTo(x, y + h);                       // низ слева
    crownPath.lineTo(x, y + h * 0.32);                // левый край вверх
    crownPath.lineTo(x + w * 0.22, y + h * 0.62);     // впадина
    crownPath.lineTo(x + w * 0.5, y);                 // центральный зубец
    crownPath.lineTo(x + w * 0.78, y + h * 0.62);     // впадина
    crownPath.lineTo(x + w, y + h * 0.32);            // правый край
    crownPath.lineTo(x + w, y + h);                   // вниз справа
    crownPath.closeSubpath();

    QLinearGradient fill(crown.topLeft(), crown.bottomLeft());
    fill.setColorAt(0.0, QColor(QStringLiteral("#ffe7a3")));
    fill.setColorAt(1.0, kGold);
    painter.fillPath(crownPath, fill);

    painter.setPen(QPen(QColor(QStringLiteral("#fff3c4")), 1.4));
    painter.drawPath(crownPath);

    // три камушка на зубцах
    painter.setBrush(QColor(QStringLiteral("#4d6bfe")));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(x + w * 0.5, y + h * 0.52), 2.6, 2.6);
    painter.setBrush(QColor(QStringLiteral("#98c379")));
    painter.drawEllipse(QPointF(x + w * 0.2, y + h * 0.72), 2.2, 2.2);
    painter.drawEllipse(QPointF(x + w * 0.8, y + h * 0.72), 2.2, 2.2);
}
