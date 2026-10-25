#include "title_bar.hpp"
#include <QHBoxLayout>
#include <QPushButton>
#include <QMouseEvent>
#include <QWindow>
#include <QPainter>
#include <QLabel>
#include <QMenuBar>

class TrafficLight : public QPushButton {
public:
    TrafficLight(const QColor& bg, const QColor& hoverBg, const QString& symbol, QWidget* parent = nullptr)
        : QPushButton(parent), m_bg(bg), m_hoverBg(hoverBg), m_symbol(symbol) {
        setFixedSize(12, 12);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);
        setStyleSheet("QPushButton { background: transparent; border: none; padding: 0; margin: 0; }");
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(underMouse() ? m_hoverBg : m_bg);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(0.5, 0.5, width() - 1.0, height() - 1.0));
        if (underMouse() && !m_symbol.isEmpty()) {
            QFont f = font(); f.setPointSizeF(6.5); f.setBold(true);
            p.setFont(f);
            p.setPen(QColor(0, 0, 0, 180));
            p.drawText(rect(), Qt::AlignCenter, m_symbol);
        }
    }
private:
    QColor m_bg, m_hoverBg;
    QString m_symbol;
};

TitleBar::TitleBar(QWidget* parent) : QWidget(parent) {
    setFixedHeight(36);
    setStyleSheet("QWidget { background: rgba(10, 10, 15, 180); border-bottom: 1px solid rgba(255,45,149, 0.3); border-top-left-radius: 12px; border-top-right-radius: 12px; }");
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(8);

    auto* closeBtn = new TrafficLight(QColor(0xff, 0x5f, 0x57), QColor(0xff, 0x7a, 0x72), "×", this);
    auto* minBtn = new TrafficLight(QColor(0xfe, 0xbc, 0x2e), QColor(0xff, 0xd0, 0x60), "−", this);
    auto* maxBtn = new TrafficLight(QColor(0x28, 0xc8, 0x40), QColor(0x4d, 0xe0, 0x60), "+", this);

    layout->addWidget(closeBtn);
    layout->addWidget(minBtn);
    layout->addWidget(maxBtn);
    layout->addSpacing(12);

    m_menuBar = new QMenuBar(this);
    m_menuBar->setStyleSheet("QMenuBar { background: transparent; color: #ffffff; } QMenuBar::item { padding: 4px 8px; } QMenuBar::item:selected { background: rgba(255,255,255,20); border-radius: 4px; }");
    layout->addWidget(m_menuBar);
    layout->addStretch();

    m_title = new QLabel("Uran PDF", this);
    m_title->setStyleSheet("QLabel { color: #ffffff; font-weight: bold; font-size: 13px; }");
    layout->addWidget(m_title);
    layout->addStretch();

    auto* spacer = new QWidget(this);
    spacer->setFixedWidth(12 * 3 + 8 * 2 + 12);
    layout->addWidget(spacer);

    connect(closeBtn, &QPushButton::clicked, this, [this]{ window()->close(); });
    connect(minBtn, &QPushButton::clicked, this, [this]{ window()->showMinimized(); });
    connect(maxBtn, &QPushButton::clicked, this, [this]{
        window()->isMaximized() ? window()->showNormal() : window()->showMaximized();
    });
}

void TitleBar::setTitle(const QString& title) { m_title->setText(title); }
void TitleBar::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && window()->windowHandle())
        window()->windowHandle()->startSystemMove();
}
void TitleBar::mouseDoubleClickEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton)
        window()->isMaximized() ? window()->showNormal() : window()->showMaximized();
}