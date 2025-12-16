#pragma once

#include <QAbstractButton>

class QPropertyAnimation;

class ToggleSwitch : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(qreal offset READ offset WRITE setOffset)

public:
    explicit ToggleSwitch(QWidget* parent = nullptr);
    QSize sizeHint() const override;

    qreal offset() const { return m_offset; }
    void setOffset(qreal o);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    qreal m_offset;
    QPropertyAnimation* m_anim;
};
