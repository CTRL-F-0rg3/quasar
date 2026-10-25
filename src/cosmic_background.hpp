#pragma once
#include <QWidget>
#include <QPointF>
#include <QList>
class CosmicBackground : public QWidget {
    Q_OBJECT
public:
    explicit CosmicBackground(QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent* e) override;
private:
    struct Star { QPointF pos; qreal brightness; qreal size; };
    QList<Star> m_stars;
};