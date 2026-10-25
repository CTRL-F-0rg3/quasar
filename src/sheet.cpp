#include "sheet.hpp"
#include "formula_engine.hpp"
#include <QRegularExpression>
#include <QDate>
#include <QDateTime>

Sheet::Sheet() : name("Sheet1") {}

QVariant Sheet::rawValue(const QString& cell) const {
    return m_raw.value(cell.toUpper(), "");
}

void Sheet::setRawValue(const QString& cell, const QString& raw) {
    m_raw[cell.toUpper()] = raw;
    m_cache.clear();
}

QVariant Sheet::computedValue(const QString& cell, QSet<QString>& visiting) const {
    QString key = cell.toUpper();
    if (m_cache.contains(key)) return m_cache[key];
    QString raw = m_raw.value(key, "");
    QVariant v = FormulaEngine::evaluate(raw, const_cast<Sheet*>(this), key);
    m_cache[key] = v;
    return v;
}

CellValue Sheet::typedValue(const QString& cell, QSet<QString>& visiting) const {
    QString raw = m_raw.value(cell.toUpper(), "");
    Q_UNUSED(visiting);
    return detectType(raw, const_cast<Sheet*>(this), cell);
}

CellValue Sheet::detectType(const QString& raw, Sheet* sheet, const QString& ref) {
    CellValue cv;
    QString s = raw.trimmed();
    if (s.isEmpty()) { cv.type = CellType::Empty; return cv; }

    // Formuła?
    if (s.startsWith('=')) {
        cv.type = CellType::Formula;
        QSet<QString> visiting;
        visiting.insert(ref.toUpper());
        QVariant v = sheet ? sheet->computedValue(ref, visiting)
                           : FormulaEngine::evaluate(s, nullptr, ref);
        cv.value = v;
        if (v.typeId() == QMetaType::QDate)
            cv.displayText = v.toDate().toString("yyyy-MM-dd");
        else if (v.typeId() == QMetaType::QDateTime)
            cv.displayText = v.toDateTime().toString("yyyy-MM-dd hh:mm");
        else
            cv.displayText = v.toString();
        return cv;
    }

    // Procent?
    if (s.endsWith('%')) {
        bool ok;
        double v = s.left(s.size() - 1).trimmed().toDouble(&ok);
        if (ok) { cv.type = CellType::Percent; cv.value = v / 100.0; cv.displayText = s; return cv; }
    }

    // Waluta?
    if (s.startsWith('$') || s.startsWith("zł") || s.startsWith("PLN")) {
        QString num = s;
        num.remove('$').remove("zł").remove("PLN").remove(' '); num = num.trimmed();
        bool ok;
        double v = num.toDouble(&ok);
        if (ok) { cv.type = CellType::Currency; cv.value = v; cv.displayText = s; return cv; }
    }

    // Bool
    QString low = s.toLower();
    if (low == "true" || low == "prawda" || low == "tak") {
        cv.type = CellType::Bool; cv.value = true; cv.displayText = "TRUE"; return cv;
    }
    if (low == "false" || low == "fałsz" || low == "nie") {
        cv.type = CellType::Bool; cv.value = false; cv.displayText = "FALSE"; return cv;
    }

    // Liczba
    bool ok;
    double d = s.toDouble(&ok);
    if (ok) { cv.type = CellType::Number; cv.value = d; cv.displayText = QString::number(d, 'g', 15); return cv; }

    // Data
    QDate date = QDate::fromString(s, "yyyy-MM-dd");
    if (date.isValid()) {
        cv.type = CellType::Date; cv.value = date; cv.displayText = s; return cv;
    }
    date = QDate::fromString(s, "dd.MM.yyyy");
    if (date.isValid()) {
        cv.type = CellType::Date; cv.value = date; cv.displayText = s; return cv;
    }
    date = QDate::fromString(s, "dd/MM/yyyy");
    if (date.isValid()) {
        cv.type = CellType::Date; cv.value = date; cv.displayText = s; return cv;
    }

    // Data + czas
    QDateTime dt = QDateTime::fromString(s, "yyyy-MM-dd hh:mm");
    if (dt.isValid()) {
        cv.type = CellType::DateTime; cv.value = dt; cv.displayText = s; return cv;
    }

    cv.type = CellType::Text;
    cv.value = s;
    cv.displayText = s;
    return cv;
}

CellFormat Sheet::format(const QString& cell) const {
    return m_formats.value(cell.toUpper());
}

void Sheet::setFormat(const QString& cell, const CellFormat& fmt) {
    m_formats[cell.toUpper()] = fmt;
}

QPair<int, int> Sheet::parseRef(const QString& ref) {
    QString r = ref.toUpper();
    r.remove('$');
    QRegularExpression re("^([A-Z]+)([0-9]+)$");
    auto m = re.match(r);
    if (!m.hasMatch()) return {-1, -1};
    QString colStr = m.captured(1);
    int row = m.captured(2).toInt() - 1;
    int col = 0;
    for (QChar c : colStr) col = col * 26 + (c.unicode() - 'A' + 1);
    return {row, col - 1};
}

QString Sheet::makeRef(int row, int col) {
    QString colStr;
    col++;
    while (col > 0) {
        int rem = (col - 1) % 26;
        colStr.prepend(QChar('A' + rem));
        col = (col - 1) / 26;
    }
    return colStr + QString::number(row + 1);
}

QStringList Sheet::cellsInRange(const QString& from, const QString& to) const {
    auto [r1, c1] = parseRef(from);
    auto [r2, c2] = parseRef(to);
    if (r1 < 0 || r2 < 0) return {};
    if (r1 > r2) std::swap(r1, r2);
    if (c1 > c2) std::swap(c1, c2);
    QStringList out;
    for (int r = r1; r <= r2; ++r)
        for (int c = c1; c <= c2; ++c)
            out.append(makeRef(r, c));
    return out;
}

void Sheet::loadRaw(const QHash<QString, QString>& r, const QHash<QString, CellFormat>& f) {
    m_raw = r;
    m_formats = f;
    m_cache.clear();
}