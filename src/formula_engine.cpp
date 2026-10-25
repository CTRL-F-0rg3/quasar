#include "formula_engine.hpp"
#include "sheet.hpp"
#include <QRegularExpression>
#include <QStringList>
#include <QDate>
#include <QDateTime>
#include <cmath>

namespace {

QStringList tokenize(const QString& s) {
    QStringList out;
    int i = 0;
    while (i < s.size()) {
        QChar c = s[i];
        if (c.isSpace()) { i++; continue; }
        if (c.isDigit() || (c == '.' && i + 1 < s.size() && s[i+1].isDigit())) {
            int j = i;
            while (j < s.size() && (s[j].isDigit() || s[j] == '.')) j++;
            out << s.mid(i, j - i);
            i = j;
            continue;
        }
        if (c == '"') {
            int j = i + 1;
            while (j < s.size() && s[j] != '"') j++;
            out << s.mid(i, j - i + 1);
            i = j + 1;
            continue;
        }
        if (c.isLetter() || c == '_') {
            int j = i;
            while (j < s.size() && (s[j].isLetterOrNumber() || s[j] == '_' || s[j] == '$')) j++;
            out << s.mid(i, j - i);
            i = j;
            continue;
        }
        if (QString("+-*/(),:<>=").contains(c)) {
            out << QString(c);
            i++;
            continue;
        }
        i++;
    }
    return out;
}

double toNum(const QVariant& v) {
    if (v.typeId() == QMetaType::QDate)
        return v.toDate().toJulianDay();
    if (v.typeId() == QMetaType::QDateTime)
        return v.toDateTime().toSecsSinceEpoch() / 86400.0;
    return v.toDouble();
}

class Parser {
public:
    Parser(const QStringList& tokens, Sheet* sheet, const QString& cell, QSet<QString>& visiting)
        : m_tokens(tokens), m_sheet(sheet), m_cell(cell), m_visiting(visiting) {}

    QVariant parse() { return parseExpr(); }

private:
    const QStringList& m_tokens;
    Sheet* m_sheet;
    QString m_cell;
    QSet<QString>& m_visiting;
    int m_pos = 0;

    QString peek() const { return m_pos < m_tokens.size() ? m_tokens[m_pos] : QString(); }
    QString next() { return m_pos < m_tokens.size() ? m_tokens[m_pos++] : QString(); }

    QVariant parseExpr() {
        QVariant left = parseTerm();
        while (peek() == "+" || peek() == "-") {
            QString op = next();
            QVariant right = parseTerm();
            double a = toNum(left), b = toNum(right);
            left = (op == "+") ? a + b : a - b;
        }
        while (peek() == "=" || peek() == "<" || peek() == ">") {
            QString op = next();
            QVariant right = parseTerm();
            double a = toNum(left), b = toNum(right);
            if (op == "=") left = (a == b) ? 1 : 0;
            else if (op == "<") left = (a < b) ? 1 : 0;
            else left = (a > b) ? 1 : 0;
        }
        return left;
    }

    QVariant parseTerm() {
        QVariant left = parseFactor();
        while (peek() == "*" || peek() == "/") {
            QString op = next();
            QVariant right = parseFactor();
            double a = toNum(left), b = toNum(right);
            left = (op == "*") ? a * b : (b != 0 ? a / b : QVariant("ERR:DIV/0"));
        }
        return left;
    }

    QVariant parseFactor() {
        QString tok = peek();
        if (tok.isEmpty()) return 0;

        if (tok == "(") {
            next();
            QVariant v = parseExpr();
            if (peek() == ")") next();
            return v;
        }
        if (tok == "-") { next(); return -toNum(parseFactor()); }
        if (tok == "+") { next(); return parseFactor(); }
        if (tok.startsWith("\"")) {
            next();
            return tok.mid(1, tok.size() - 2);
        }

        QString upperTok = tok.toUpper();
        static const QSet<QString> functions = {
            "SUM","AVERAGE","COUNT","COUNTA","COUNTBLANK","MIN","MAX",
            "ROUND","ROUNDUP","ROUNDDOWN","ABS","MOD","POWER","SQRT","PRODUCT",
            "SUMIF","SUMIFS","AVERAGEIF","AVERAGEIFS","COUNTIF","COUNTIFS",
            "IF","IFS","AND","OR","NOT","IFERROR","IFNA","SWITCH",
            "VLOOKUP","HLOOKUP","XLOOKUP","INDEX","MATCH","OFFSET","INDIRECT",
            "CHOOSE","TRANSPOSE","ROW","COLUMN","LOOKUP",
            "CONCAT","CONCATENATE","TEXTJOIN","LEFT","RIGHT","MID","LEN",
            "LOWER","UPPER","PROPER","TRIM","SUBSTITUTE","REPLACE","FIND","SEARCH","TEXT",
            "TODAY","NOW","DATE","YEAR","MONTH","DAY","DATEDIF","WORKDAY","NETWORKDAYS",
            "EOMONTH","EDATE",
            "PMT","PV","FV","RATE","NPER","NPV",
            "FILTER","SORT","SORTBY","UNIQUE","SEQUENCE","XMATCH"
        };
        if (functions.contains(upperTok) && m_pos + 1 < m_tokens.size() && m_tokens[m_pos + 1] == "(") {
            next(); next();
            QList<QVariant> args = parseArgs();
            if (peek() == ")") next();
            return FormulaEngine::callFunction(upperTok, args);
        }

        bool ok;
        double num = tok.toDouble(&ok);
        if (ok) { next(); return num; }

        if (QRegularExpression("^\\$?[A-Z]+\\$?[0-9]+$", QRegularExpression::CaseInsensitiveOption).match(tok).hasMatch()) {
            next();
            return FormulaEngine::resolveCell(tok, m_sheet, m_cell, m_visiting);
        }

        // Spróbuj jako data (YYYY-MM-DD)
        QDate d = QDate::fromString(tok, "yyyy-MM-dd");
        if (d.isValid()) { next(); return d; }

        next();
        return QVariant("ERR:#NAME");
    }

    QList<QVariant> parseArgs() {
        QList<QVariant> args;
        if (peek() == ")") return args;

        while (true) {
            QString tok = peek();
            // Zakres A1:B2?
            if (m_pos + 2 < m_tokens.size() &&
                QRegularExpression("^\\$?[A-Z]+\\$?[0-9]+$", QRegularExpression::CaseInsensitiveOption).match(tok).hasMatch() &&
                m_tokens[m_pos + 1] == ":" &&
                QRegularExpression("^\\$?[A-Z]+\\$?[0-9]+$", QRegularExpression::CaseInsensitiveOption).match(m_tokens[m_pos + 2]).hasMatch()) {
                QString from = tok, to = m_tokens[m_pos + 2];
                m_pos += 3;
                QStringList cells = m_sheet->cellsInRange(from, to);
                for (const QString& c : cells)
                    args.append(FormulaEngine::resolveCell(c, m_sheet, m_cell, m_visiting));
            } else {
                args.append(parseExpr());
            }
            if (peek() == ",") { next(); continue; }
            break;
        }
        return args;
    }
};

} // namespace

QVariant FormulaEngine::evaluate(const QString& raw, Sheet* sheet, const QString& currentCell) {
    if (!raw.startsWith('=')) {
        bool ok;
        double d = raw.toDouble(&ok);
        if (ok) return d;
        return raw;
    }
    QSet<QString> visiting;
    visiting.insert(currentCell.toUpper());
    return evalExpression(raw.mid(1).trimmed(), sheet, currentCell, visiting);
}

QVariant FormulaEngine::evalExpression(const QString& expr, Sheet* sheet,
                                      const QString& cell, QSet<QString>& visiting) {
    QStringList tokens = tokenize(expr);
    if (tokens.isEmpty()) return "";
    Parser p(tokens, sheet, cell, visiting);
    return p.parse();
}

QVariant FormulaEngine::resolveCell(const QString& ref, Sheet* sheet,
                                    const QString& cell, QSet<QString>& visiting) {
    QString key = ref;
    key.remove('$');
    key = key.toUpper();
    if (visiting.contains(key)) return QVariant("ERR:CIRC");
    if (!sheet) return 0;
    visiting.insert(key);
    QVariant result = sheet->computedValue(key, visiting);
    visiting.remove(key);
    return result;
}

QVariant FormulaEngine::callFunction(const QString& name, const QList<QVariant>& args) {
    auto num = [](const QVariant& v) { return toNum(v); };
    auto isNum = [](const QVariant& v) {
        if (v.typeId() == QMetaType::Double || v.typeId() == QMetaType::Int) return true;
        bool ok; v.toString().toDouble(&ok); return ok;
    };
    auto toStr = [](const QVariant& v) {
        if (v.typeId() == QMetaType::QDate) return v.toDate().toString("yyyy-MM-dd");
        return v.toString();
    };

    // === MATEMATYCZNE ===
    if (name == "SUM") { double s = 0; for (auto& a : args) s += num(a); return s; }
    if (name == "AVERAGE") {
        int n = 0; double s = 0;
        for (auto& a : args) if (isNum(a)) { s += num(a); n++; }
        return n ? s / n : QVariant("ERR:DIV/0");
    }
    if (name == "COUNT") {
        int n = 0; for (auto& a : args) if (isNum(a)) n++; return n;
    }
    if (name == "COUNTA") {
        int n = 0; for (auto& a : args) if (!a.toString().isEmpty()) n++; return n;
    }
    if (name == "COUNTBLANK") {
        int n = 0; for (auto& a : args) if (a.toString().isEmpty()) n++; return n;
    }
    if (name == "MIN") {
        double m = 1e308; bool found = false;
        for (auto& a : args) if (isNum(a)) { m = qMin(m, num(a)); found = true; }
        return found ? m : 0;
    }
    if (name == "MAX") {
        double m = -1e308; bool found = false;
        for (auto& a : args) if (isNum(a)) { m = qMax(m, num(a)); found = true; }
        return found ? m : 0;
    }
    if (name == "ROUND") {
        if (args.size() < 2) return 0;
        double v = num(args[0]); int d = args[1].toInt();
        double f = std::pow(10, d);
        return std::round(v * f) / f;
    }
    if (name == "ROUNDUP") {
        if (args.size() < 2) return 0;
        double v = num(args[0]); int d = args[1].toInt();
        double f = std::pow(10, d);
        return std::ceil(v * f) / f;
    }
    if (name == "ROUNDDOWN") {
        if (args.size() < 2) return 0;
        double v = num(args[0]); int d = args[1].toInt();
        double f = std::pow(10, d);
        return std::floor(v * f) / f;
    }
    if (name == "ABS") return args.isEmpty() ? 0 : std::abs(num(args[0]));
    if (name == "MOD") return args.size() < 2 ? 0 : std::fmod(num(args[0]), num(args[1]));
    if (name == "POWER") return args.size() < 2 ? 0 : std::pow(num(args[0]), num(args[1]));
    if (name == "SQRT") return args.isEmpty() ? 0 : std::sqrt(num(args[0]));
    if (name == "PRODUCT") { double p = 1; for (auto& a : args) p *= num(a); return p; }
    if (name == "SUMIF") {
        if (args.size() < 2) return 0;
        QString crit = args[0].toString();
        bool ok; double cv = crit.toDouble(&ok);
        double sum = 0;
        for (int i = 1; i < args.size(); ++i) {
            double v = num(args[i]);
            if ((ok && v == cv) || (!ok && args[i].toString() == crit)) sum += v;
        }
        return sum;
    }
    if (name == "COUNTIF") {
        if (args.size() < 2) return 0;
        QString crit = args[1].toString();
        bool ok; double cv = crit.toDouble(&ok);
        int n = 0;
        for (int i = 0; i < 1; ++i) {
            for (auto& a : args) {
                if (&a == &args[0]) continue;
            }
        }
        // Prostsze: licz argumenty (bez pierwszego) które spełniają kryterium
        for (int i = 1; i < args.size(); ++i) {
            double v = num(args[i]);
            if ((ok && v == cv) || (!ok && args[i].toString() == crit)) n++;
        }
        return n;
    }
    if (name == "SUMIFS" || name == "AVERAGEIFS" || name == "COUNTIFS" || name == "AVERAGEIF") {
        // Uproszczona implementacja: suma/avg/count argumentów liczbowych
        double s = 0; int n = 0;
        for (auto& a : args) if (isNum(a)) { s += num(a); n++; }
        if (name == "SUMIFS") return s;
        if (name == "AVERAGEIFS") return n ? s / n : 0;
        if (name == "COUNTIFS") return n;
        if (name == "AVERAGEIF") return n ? s / n : 0;
    }

    // === LOGICZNE ===
    if (name == "IF") {
        if (args.size() < 3) return "ERR:IF";
        double cond = num(args[0]);
        return cond != 0 ? args[1] : args[2];
    }
    if (name == "IFS") {
        for (int i = 0; i + 1 < args.size(); i += 2) {
            if (num(args[i]) != 0) return args[i + 1];
        }
        return "ERR:IFS";
    }
    if (name == "AND") {
        for (auto& a : args) if (num(a) == 0) return 0;
        return 1;
    }
    if (name == "OR") {
        for (auto& a : args) if (num(a) != 0) return 1;
        return 0;
    }
    if (name == "NOT") return args.isEmpty() ? 1 : (num(args[0]) == 0 ? 1 : 0);
    if (name == "IFERROR") {
        if (args.size() < 2) return 0;
        QString s = args[0].toString();
        if (s.startsWith("ERR:")) return args[1];
        return args[0];
    }
    if (name == "IFNA") {
        if (args.size() < 2) return 0;
        return args[0].toString() == "ERR:#N/A" ? args[1] : args[0];
    }
    if (name == "SWITCH") {
        if (args.size() < 2) return "ERR:SWITCH";
        QVariant v = args[0];
        for (int i = 1; i + 1 < args.size(); i += 2) {
            if (v == args[i]) return args[i + 1];
        }
        if (args.size() % 2 == 0) return args.last();
        return "ERR:SWITCH";
    }

    // === WYSZUKIWANIE ===
    if (name == "INDEX") {
        if (args.size() < 3) return "ERR:INDEX";
        // INDEX(array, row, col) - uproszczenie: args są już wartościami
        int idx = args[1].toInt() - 1;
        int col = args.size() > 2 ? args[2].toInt() - 1 : 0;
        int total = args.size() - 1;
        int pos = idx + col;
        if (pos >= 0 && pos < args.size()) return args[pos + 1];
        return "ERR:INDEX";
    }
    if (name == "MATCH") {
        if (args.size() < 2) return "ERR:MATCH";
        QVariant target = args[0];
        for (int i = 1; i < args.size(); ++i) {
            if (args[i] == target) return i;
        }
        return "ERR:#N/A";
    }
    if (name == "CHOOSE") {
        if (args.size() < 2) return "ERR:CHOOSE";
        int idx = args[0].toInt();
        if (idx >= 1 && idx < args.size()) return args[idx];
        return "ERR:CHOOSE";
    }
    if (name == "ROW" || name == "COLUMN") {
        if (args.isEmpty()) return 1;
        auto [r, c] = Sheet::parseRef(args[0].toString());
        return name == "ROW" ? r + 1 : c + 1;
    }
    if (name == "VLOOKUP" || name == "HLOOKUP" || name == "XLOOKUP" ||
        name == "LOOKUP" || name == "OFFSET" || name == "INDIRECT" ||
        name == "TRANSPOSE" || name == "XMATCH") {
        return "ERR:NOTIMPL";
    }

    // === TEKSTOWE ===
    if (name == "CONCAT" || name == "CONCATENATE") {
        QString s;
        for (auto& a : args) s += toStr(a);
        return s;
    }
    if (name == "TEXTJOIN") {
        if (args.size() < 2) return "";
        QString sep = args[0].toString();
        bool ignoreEmpty = num(args[1]) != 0;
        QStringList parts;
        for (int i = 2; i < args.size(); ++i) {
            QString v = toStr(args[i]);
            if (ignoreEmpty && v.isEmpty()) continue;
            parts.append(v);
        }
        return parts.join(sep);
    }
    if (name == "LEFT") {
        if (args.isEmpty()) return "";
        QString s = toStr(args[0]);
        int n = args.size() > 1 ? args[1].toInt() : 1;
        return s.left(n);
    }
    if (name == "RIGHT") {
        if (args.isEmpty()) return "";
        QString s = toStr(args[0]);
        int n = args.size() > 1 ? args[1].toInt() : 1;
        return s.right(n);
    }
    if (name == "MID") {
        if (args.size() < 3) return "";
        QString s = toStr(args[0]);
        int start = args[1].toInt() - 1;
        int len = args[2].toInt();
        return s.mid(start, len);
    }
    if (name == "LEN") return args.isEmpty() ? 0 : toStr(args[0]).size();
    if (name == "LOWER") return args.isEmpty() ? "" : toStr(args[0]).toLower();
    if (name == "UPPER") return args.isEmpty() ? "" : toStr(args[0]).toUpper();
    if (name == "PROPER") {
        if (args.isEmpty()) return "";
        QStringList words = toStr(args[0]).split(' ', Qt::SkipEmptyParts);
        for (auto& w : words) w[0] = w[0].toUpper();
        return words.join(' ');
    }
    if (name == "TRIM") {
        if (args.isEmpty()) return "";
        return toStr(args[0]).trimmed().replace(QRegularExpression("\\s+"), " ");
    }
    if (name == "SUBSTITUTE") {
        if (args.size() < 3) return "";
        return toStr(args[0]).replace(toStr(args[1]), toStr(args[2]));
    }
    if (name == "REPLACE") {
        if (args.size() < 4) return "";
        QString s = toStr(args[0]);
        int pos = args[1].toInt() - 1;
        int len = args[2].toInt();
        QString rep = toStr(args[3]);
        return s.left(pos) + rep + s.mid(pos + len);
    }
    if (name == "FIND" || name == "SEARCH") {
        if (args.size() < 2) return "ERR:#VALUE!";
        QString needle = toStr(args[0]), hay = toStr(args[1]);
        int pos = (name == "FIND")
            ? hay.indexOf(needle)
            : hay.indexOf(needle, 0, Qt::CaseInsensitive);
        if (pos < 0) return "ERR:#VALUE!";
        return pos + 1;
    }
    if (name == "TEXT") {
        if (args.isEmpty()) return "";
        return toStr(args[0]);
    }

    // === DATY ===
    if (name == "TODAY") return QDate::currentDate();
    if (name == "NOW") return QDateTime::currentDateTime();
    if (name == "DATE") {
        if (args.size() < 3) return QVariant();
        return QDate(args[0].toInt(), args[1].toInt(), args[2].toInt());
    }
    if (name == "YEAR") {
        if (args.isEmpty()) return 0;
        QDate d = args[0].toDate();
        if (d.isValid()) return d.year();
        return QDate::fromString(args[0].toString(), "yyyy-MM-dd").year();
    }
    if (name == "MONTH") {
        if (args.isEmpty()) return 0;
        QDate d = args[0].toDate();
        if (d.isValid()) return d.month();
        return QDate::fromString(args[0].toString(), "yyyy-MM-dd").month();
    }
    if (name == "DAY") {
        if (args.isEmpty()) return 0;
        QDate d = args[0].toDate();
        if (d.isValid()) return d.day();
        return QDate::fromString(args[0].toString(), "yyyy-MM-dd").day();
    }
    if (name == "DATEDIF") {
        if (args.size() < 3) return 0;
        QDate d1 = args[0].toDate(), d2 = args[1].toDate();
        if (!d1.isValid() || !d2.isValid()) return "ERR:DATE";
        QString unit = toStr(args[2]).toUpper();
        qint64 days = d1.daysTo(d2);
        if (unit == "D") return int(days);
        if (unit == "M") return int(days / 30);
        if (unit == "Y") return int(days / 365);
        return int(days);
    }
    if (name == "WORKDAY") {
        if (args.size() < 2) return QVariant();
        QDate start = args[0].toDate();
        int n = args[1].toInt();
        while (n > 0) {
            start = start.addDays(1);
            if (start.dayOfWeek() != 6 && start.dayOfWeek() != 7) n--;
        }
        return start;
    }
    if (name == "NETWORKDAYS") {
        if (args.size() < 2) return 0;
        QDate d1 = args[0].toDate(), d2 = args[1].toDate();
        int n = 0;
        while (d1 <= d2) {
            if (d1.dayOfWeek() != 6 && d1.dayOfWeek() != 7) n++;
            d1 = d1.addDays(1);
        }
        return n;
    }
    if (name == "EOMONTH") {
        if (args.size() < 2) return QVariant();
        QDate d = args[0].toDate();
        int months = args[1].toInt();
        return d.addMonths(months + 1).addDays(-d.day());
    }
    if (name == "EDATE") {
        if (args.size() < 2) return QVariant();
        return args[0].toDate().addMonths(args[1].toInt());
    }

    // === FINANSOWE ===
    if (name == "PMT") {
        if (args.size() < 3) return 0;
        double rate = num(args[0]), nper = num(args[1]), pv = num(args[2]);
        if (rate == 0) return -pv / nper;
        double p = std::pow(1 + rate, nper);
        return -pv * rate * p / (p - 1);
    }
    if (name == "PV") {
        if (args.size() < 3) return 0;
        double rate = num(args[0]), nper = num(args[1]), pmt = num(args[2]);
        if (rate == 0) return -pmt * nper;
        return -pmt * (1 - std::pow(1 + rate, -nper)) / rate;
    }
    if (name == "FV") {
        if (args.size() < 3) return 0;
        double rate = num(args[0]), nper = num(args[1]), pmt = num(args[2]);
        if (rate == 0) return -pmt * nper;
        return -pmt * (std::pow(1 + rate, nper) - 1) / rate;
    }
    if (name == "RATE" || name == "NPER" || name == "NPV") {
        return "ERR:NOTIMPL";
    }

    // === TABLICOWE ===
    if (name == "SEQUENCE") {
        if (args.isEmpty()) return 0;
        int n = args[0].toInt();
        if (n > 100) n = 100;
        QStringList out;
        for (int i = 1; i <= n; ++i) out << QString::number(i);
        return out.join(",");
    }
    if (name == "UNIQUE" || name == "FILTER" || name == "SORT" ||
        name == "SORTBY" || name == "XMATCH") {
        return "ERR:NOTIMPL";
    }

    return "ERR:#NAME";
}