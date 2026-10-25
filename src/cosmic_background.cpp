#include "cosmic_background.hpp"
#include <QPainter>
#include <QRandomGenerator>
#include <QRadialGradient>
#include <QPainterPath>

CosmicBackground::CosmicBackground(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < 350; ++i) {
        m_stars.append({QPointF(rng->generateDouble(), rng->generateDouble()),
                        0.2 + rng->generateDouble() * 0.8, 0.5 + rng->generateDouble() * 1.5});
    }
}
void CosmicBackground::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(rect(), 12, 12);
    p.setClipPath(path);

    p.fillRect(rect(), QColor(4, 4, 10));
    QRadialGradient n1(width()*0.2, height()*0.2, width()*0.8);
    n1.setColorAt(0, QColor(140, 20, 100, 50)); n1.setColorAt(1, Qt::transparent);
    p.fillRect(rect(), n1);
    QRadialGradient n2(width()*0.8, height()*0.8, width()*0.8);
    n2.setColorAt(0, QColor(20, 120, 80, 40)); n2.setColorAt(1, Qt::transparent);
    p.fillRect(rect(), n2);

    p.setPen(Qt::NoPen);
    for (const auto& s : m_stars) {
        p.setBrush(QColor(255, 255, 255, int(s.brightness * 220)));
        p.drawEllipse(QPointF(s.pos.x() * width(), s.pos.y() * height()), s.size, s.size);
    }
    p.setClipping(false);
    p.setPen(QPen(QColor(255, 255, 255, 30), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 12, 12);
}