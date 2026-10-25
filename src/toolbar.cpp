#include "toolbar.hpp"
#include <QHBoxLayout>
#include <QToolButton>
#include <QColorDialog>
#include <QFrame>

static const char* kStyle = R"(
QWidget#spreadsheetToolbar {
    background: rgba(10, 10, 15, 200);
    border-bottom: 1px solid rgba(255,255,255,0.1);
}
QToolButton {
    background: transparent; border: 1px solid transparent;
    border-radius: 4px; color: #ffffff; padding: 4px 8px; font-weight: bold;
}
QToolButton:hover { border-color: rgba(255,255,255,0.3); background: rgba(255,255,255,0.1); }
QToolButton:checked { background: #ff2d95; }
QFrame[frameShape="5"] { color: rgba(255,255,255,0.1); }
)";

Toolbar::Toolbar(QWidget* parent) : QWidget(parent) {
    setObjectName("spreadsheetToolbar");
    setStyleSheet(kStyle);
    setFixedHeight(48);

    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(12, 6, 12, 6);
    l->setSpacing(4);

    auto* newBtn = new QToolButton(this);
    newBtn->setText("New Sheet");
    connect(newBtn, &QToolButton::clicked, this, &Toolbar::newSheetRequested);
    l->addWidget(newBtn);

    auto* openBtn = new QToolButton(this);
    openBtn->setText("Open");
    connect(openBtn, &QToolButton::clicked, this, &Toolbar::openRequested);
    l->addWidget(openBtn);

    auto* saveBtn = new QToolButton(this);
    saveBtn->setText("Save");
    saveBtn->setStyleSheet("QToolButton { background: #ff2d95; padding: 4px 12px; }");
    connect(saveBtn, &QToolButton::clicked, this, &Toolbar::saveRequested);
    l->addWidget(saveBtn);

    auto* vline = new QFrame(this); vline->setFrameShape(QFrame::VLine); l->addWidget(vline);

    m_bold = new QToolButton(this);
    m_bold->setText("B"); m_bold->setCheckable(true);
    m_bold->setStyleSheet("QToolButton { font-weight: 900; font-family: serif; }");
    connect(m_bold, &QToolButton::toggled, this, &Toolbar::boldToggled);
    l->addWidget(m_bold);

    m_italic = new QToolButton(this);
    m_italic->setText("I"); m_italic->setCheckable(true);
    m_italic->setStyleSheet("QToolButton { font-style: italic; font-family: serif; }");
    connect(m_italic, &QToolButton::toggled, this, &Toolbar::italicToggled);
    l->addWidget(m_italic);

    auto* vline2 = new QFrame(this); vline2->setFrameShape(QFrame::VLine); l->addWidget(vline2);

    m_alignL = new QToolButton(this); m_alignL->setText("L");
    m_alignC = new QToolButton(this); m_alignC->setText("C");
    m_alignR = new QToolButton(this); m_alignR->setText("R");
    connect(m_alignL, &QToolButton::clicked, this, [this]{ emit alignChanged(0); });
    connect(m_alignC, &QToolButton::clicked, this, [this]{ emit alignChanged(1); });
    connect(m_alignR, &QToolButton::clicked, this, [this]{ emit alignChanged(2); });
    l->addWidget(m_alignL); l->addWidget(m_alignC); l->addWidget(m_alignR);

    auto* vline3 = new QFrame(this); vline3->setFrameShape(QFrame::VLine); l->addWidget(vline3);

    auto* bgBtn = new QToolButton(this); bgBtn->setText("BG");
    connect(bgBtn, &QToolButton::clicked, this, [this]{
        QColor c = QColorDialog::getColor(Qt::yellow, this, "Background");
        if (c.isValid()) emit bgColorChanged(c.name());
    });
    l->addWidget(bgBtn);

    auto* fgBtn = new QToolButton(this); fgBtn->setText("FG");
    connect(fgBtn, &QToolButton::clicked, this, [this]{
        QColor c = QColorDialog::getColor(Qt::black, this, "Text color");
        if (c.isValid()) emit textColorChanged(c.name());
    });
    l->addWidget(fgBtn);

    l->addStretch();
}

void Toolbar::setFormat(const CellFormat& fmt) {
    m_bold->blockSignals(true);
    m_italic->blockSignals(true);
    m_bold->setChecked(fmt.bold);
    m_italic->setChecked(fmt.italic);
    m_bold->blockSignals(false);
    m_italic->blockSignals(false);
}