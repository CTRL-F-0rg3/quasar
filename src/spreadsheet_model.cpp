#include <QBrush>
#include <QFont>
#include <QColor>
#include "spreadsheet_model.hpp"

SpreadsheetModel::SpreadsheetModel(Sheet* sheet, QObject* parent)
    : QAbstractTableModel(parent), m_sheet(sheet) {}

int SpreadsheetModel::rowCount(const QModelIndex&) const { return MAX_ROWS; }
int SpreadsheetModel::columnCount(const QModelIndex&) const { return MAX_COLS; }

QString SpreadsheetModel::columnLabel(int col) {
    QString s;
    col++;
    while (col > 0) {
        int r = (col - 1) % 26;
        s.prepend(QChar('A' + r));
        col = (col - 1) / 26;
    }
    return s;
}

QVariant SpreadsheetModel::data(const QModelIndex& idx, int role) const {
    if (!idx.isValid() || !m_sheet) return {};

    QString ref = Sheet::makeRef(idx.row(), idx.column());
    QString raw = m_sheet->rawValue(ref).toString();

    if (role == Qt::EditRole) return raw;

    if (role == Qt::DisplayRole) {
        CellValue cv = m_sheet->detectType(raw, m_sheet, ref);
        return cv.displayText;
    }

    if (role == Qt::TextAlignmentRole) {
        CellFormat fmt = m_sheet->format(ref);
        int align = Qt::AlignVCenter | Qt::AlignLeft;
        if (fmt.alignment == 1) align = Qt::AlignVCenter | Qt::AlignHCenter;
        else if (fmt.alignment == 2) align = Qt::AlignVCenter | Qt::AlignRight;
        else {
            CellValue cv = m_sheet->detectType(raw, m_sheet, ref);
            if (cv.type == CellType::Number ||
                cv.type == CellType::Currency ||
                cv.type == CellType::Percent ||
                cv.type == CellType::Date ||
                cv.type == CellType::DateTime)
                align = Qt::AlignVCenter | Qt::AlignRight;
        }
        return align;
    }

    if (role == Qt::BackgroundRole) {
        CellFormat fmt = m_sheet->format(ref);
        if (!fmt.bgColor.isEmpty()) return QBrush(QColor(fmt.bgColor));
        // Subtelna naprzemienność - jak w Excelu, ale ciemna
        if (idx.row() % 2 == 1) return QBrush(QColor(0x17, 0x14, 0x22));
        return QBrush(QColor(0x13, 0x11, 0x1b));
    }

    if (role == Qt::ForegroundRole) {
        CellFormat fmt = m_sheet->format(ref);
        if (!fmt.textColor.isEmpty()) return QBrush(QColor(fmt.textColor));

        CellValue cv = m_sheet->detectType(raw, m_sheet, ref);
        switch (cv.type) {
            case CellType::Number:   return QBrush(QColor(0xa6, 0xe3, 0xa1)); // zielony
            case CellType::Percent:  return QBrush(QColor(0xfa, 0xb3, 0x87)); // pomarańcz
            case CellType::Currency: return QBrush(QColor(0xf9, 0xe2, 0xaf)); // złoty
            case CellType::Date:
            case CellType::DateTime: return QBrush(QColor(0x94, 0xe2, 0xd5)); // cyan
            case CellType::Bool:     return QBrush(QColor(0x89, 0xb4, 0xfa)); // niebieski
            case CellType::Formula:  return QBrush(QColor(0xff, 0x5e, 0xb0)); // magenta
            case CellType::Text:
            default:                 return QBrush(QColor(0xe0, 0xdd, 0xe8));
        }
    }

    if (role == Qt::FontRole) {
        CellFormat fmt = m_sheet->format(ref);
        QFont f("Liberation Sans", 10);
        f.setBold(fmt.bold);
        f.setItalic(fmt.italic);
        return f;
    }

    return {};
}

QVariant SpreadsheetModel::headerData(int section, Qt::Orientation o, int role) const {
    if (role == Qt::DisplayRole) {
        if (o == Qt::Horizontal) return columnLabel(section);
        return QString::number(section + 1);
    }
    if (role == Qt::BackgroundRole) {
        if (o == Qt::Horizontal) return QBrush(QColor(0x20, 0x18, 0x30));
        return QBrush(QColor(0x1a, 0x14, 0x28));
    }
    if (role == Qt::ForegroundRole) {
        if (o == Qt::Horizontal) return QBrush(QColor(0xd4, 0xb8, 0xf0));
        return QBrush(QColor(0xa8, 0x90, 0xc4));
    }
    if (role == Qt::FontRole) {
        QFont f("Liberation Sans", 9);
        f.setBold(true);
        return f;
    }
    return {};
}

Qt::ItemFlags SpreadsheetModel::flags(const QModelIndex& idx) const {
    if (!idx.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool SpreadsheetModel::setData(const QModelIndex& idx, const QVariant& value, int role) {
    if (role != Qt::EditRole || !idx.isValid()) return false;
    QString ref = Sheet::makeRef(idx.row(), idx.column());
    m_sheet->setRawValue(ref, value.toString());
    emit dataChanged(idx, idx);
    emit edited();
    return true;
}

void SpreadsheetModel::refreshAll() {
    emit dataChanged(index(0, 0), index(MAX_ROWS - 1, MAX_COLS - 1));
}

void SpreadsheetModel::refreshCell(int row, int col) {
    QModelIndex idx = index(row, col);
    emit dataChanged(idx, idx);
}