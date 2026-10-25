#include "spreadsheet_view.hpp"
#include <QHeaderView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QMessageBox>
#include <QBrush>

SpreadsheetView::SpreadsheetView(Sheet* sheet, QWidget* parent)
    : QTableView(parent), m_sheet(sheet) {
    m_model = new SpreadsheetModel(sheet, this);
    setModel(m_model);

    setupStyle();

    horizontalHeader()->setDefaultSectionSize(88);
    verticalHeader()->setDefaultSectionSize(20);
    horizontalHeader()->setMinimumSectionSize(40);
    verticalHeader()->setMinimumSectionSize(18);
    horizontalHeader()->setHighlightSections(false);
    verticalHeader()->setHighlightSections(false);
    horizontalHeader()->setSectionsClickable(true);
    verticalHeader()->setSectionsClickable(true);

    QFont f("Liberation Sans", 10);
    f.setStyleHint(QFont::SansSerif);
    setFont(f);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setAlternatingRowColors(false);
    setShowGrid(true);
    setWordWrap(false);
    setCornerButtonEnabled(false);

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current, const QModelIndex&) {
        if (!current.isValid()) return;
        QString ref = Sheet::makeRef(current.row(), current.column());
        QString raw = m_sheet->rawValue(ref).toString();
        emit cellSelected(ref, raw);
    });

    connect(m_model, &SpreadsheetModel::edited, this, &SpreadsheetView::cellEdited);

    setCurrentIndex(m_model->index(0, 0));
}

void SpreadsheetView::setupStyle() {
    setStyleSheet(R"(
QTableView {
    background: #0e0c14;
    alternate-background-color: #0e0c14;
    gridline-color: #2a2438;
    selection-background-color: #33204d;
    selection-color: #ffffff;
    border: none;
    outline: 0;
}
QTableView::item:selected {
    background: #33204d;
    color: #ffffff;
}
QTableView::item:selected:!active {
    background: #281840;
    color: #e0d8f0;
}
QHeaderView {
    background: #14101c;
    border: none;
}
QHeaderView::section {
    background: #1e1830;
    color: #d4b8f0;
    border: none;
    border-right: 1px solid #2a2438;
    border-bottom: 1px solid #2a2438;
    padding: 4px;
    font-weight: bold;
    font-size: 10px;
}
QHeaderView::section:hover {
    background: #2a2244;
    color: #ff2d95;
}
QHeaderView::section:checked {
    background: #2a1e3a;
    color: #ff2d95;
}
QTableCornerButton::section {
    background: #14101c;
    border: 1px solid #2a2438;
}
QScrollBar:vertical {
    background: #0e0c14;
    width: 14px;
    border: none;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #2a2438;
    border-radius: 7px;
    min-height: 30px;
    margin: 2px;
}
QScrollBar::handle:vertical:hover {
    background: #ff2d95;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: transparent;
}
QScrollBar:horizontal {
    background: #0e0c14;
    height: 14px;
    border: none;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background: #2a2438;
    border-radius: 7px;
    min-width: 30px;
    margin: 2px;
}
QScrollBar::handle:horizontal:hover {
    background: #ff2d95;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    background: transparent;
}
)");
}

void SpreadsheetView::refreshAll() {
    m_model->refreshAll();
}

void SpreadsheetView::applyFormatToSelection(const CellFormat& fmt) {
    auto ranges = selectionModel()->selection();
    qint64 total = 0;
    for (const auto& r : ranges)
        total += (qint64)r.height() * r.width();
    if (total > 500000) {
        QMessageBox::warning(this, "Zbyt duży zakres",
            QString("Zaznaczono %1 komórek. Limit formatowania to 500 000.").arg(total));
        return;
    }
    for (const auto& r : ranges) {
        for (int row = r.top(); row <= r.bottom(); ++row) {
            for (int col = r.left(); col <= r.right(); ++col) {
                QString ref = Sheet::makeRef(row, col);
                m_sheet->setFormat(ref, fmt);
            }
        }
    }
    m_model->refreshAll();
    emit cellEdited();
}

void SpreadsheetView::clearSelectionContent() {
    auto ranges = selectionModel()->selection();
    qint64 total = 0;
    for (const auto& r : ranges)
        total += (qint64)r.height() * r.width();
    if (total > 500000) {
        QMessageBox::warning(this, "Zbyt duży zakres",
            QString("Zaznaczono %1 komórek. Limit czyszczenia to 500 000.").arg(total));
        return;
    }
    for (const auto& r : ranges) {
        for (int row = r.top(); row <= r.bottom(); ++row) {
            for (int col = r.left(); col <= r.right(); ++col) {
                QString ref = Sheet::makeRef(row, col);
                m_sheet->setRawValue(ref, "");
            }
        }
    }
    m_model->refreshAll();
    emit cellEdited();
}

CellFormat SpreadsheetView::currentFormat() const {
    QModelIndex idx = currentIndex();
    if (!idx.isValid()) return {};
    return m_sheet->format(Sheet::makeRef(idx.row(), idx.column()));
}

void SpreadsheetView::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Delete) {
        clearSelectionContent();
        return;
    }
    QTableView::keyPressEvent(e);
}

void SpreadsheetView::mousePressEvent(QMouseEvent* e) {
    if (m_formulaMode && e->button() == Qt::LeftButton) {
        QModelIndex idx = indexAt(e->pos());
        if (idx.isValid()) {
            QString ref = Sheet::makeRef(idx.row(), idx.column());
            emit formulaCellClicked(ref);
            return;
        }
    }
    QTableView::mousePressEvent(e);
}