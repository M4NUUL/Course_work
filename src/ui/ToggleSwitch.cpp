#include "ToggleSwitch.h"

#include <QPainter>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QStyleOption>
#include <QApplication>

ToggleSwitch::ToggleSwitch(QWidget* parent)
    : QAbstractButton(parent), m_offset(0.0)
{
    setCheckable(true);
    setChecked(false);
    setCursor(Qt::PointingHandCursor);

    m_anim = new QPropertyAnimation(this, "offset", this);
    m_anim->setDuration(150);
    m_anim->setEasingCurve(QEasingCurve::InOutQuad);

    // При смене состояния запускаем анимацию
    connect(this, &QAbstractButton::toggled, this, [this](bool on){
        m_anim->stop();
        m_anim->setStartValue(m_offset);
        m_anim->setEndValue(on ? 1.0 : 0.0);
        m_anim->start();
    });

    // установить стартовое положение
    m_offset = isChecked() ? 1.0 : 0.0;
}

QSize ToggleSwitch::sizeHint() const
{
    return QSize(60, 34);
}

void ToggleSwitch::setOffset(qreal o)
{
    if (qFuzzyCompare(m_offset, o)) return;
    m_offset = o;
    update();
}

void ToggleSwitch::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF r = rect();
    const qreal h = r.height();
    const qreal w = r.width();
    const qreal margin = h * 0.08;
    const QRectF trackRect(margin, margin, w - 2*margin, h - 2*margin);
    const qreal trackRadius = trackRect.height() / 2.0;

    // Цвета (можно подправить)
    QColor trackOff = palette().color(QPalette::Window).lighter(120); // светлый трек
    QColor trackOn  = QColor("#4a90e2"); // цвет при включении (синий)
    QColor knobOff  = palette().color(QPalette::WindowText);
    QColor knobOn   = QColor("#ffffff");

    const QColor trackColor = isChecked() ? trackOn : trackOff;
    const QColor knobColor  = isChecked() ? knobOn : knobOff;

    // Рисуем трек
    p.setPen(Qt::NoPen);
    p.setBrush(trackColor);
    p.drawRoundedRect(trackRect, trackRadius, trackRadius);

    // Размер ручки (knob)
    qreal knobDiameter = trackRect.height() - 4.0;
    qreal x1 = trackRect.left() + 2.0;
    qreal x2 = trackRect.right() - knobDiameter - 2.0;
    qreal y = trackRect.top() + (trackRect.height() - knobDiameter) / 2.0;
    qreal x = x1 + (x2 - x1) * m_offset;

    QRectF knobRect(x, y, knobDiameter, knobDiameter);

    // Рисуем ручку
    p.setBrush(knobColor);
    p.drawEllipse(knobRect);
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // toggle — QAbstractButton::setChecked уже выдаст сигнал toggled, на который мы подписаны
        setChecked(!isChecked());
    }
    QAbstractButton::mouseReleaseEvent(event);
}
