#pragma once
#include <QString>
#include <QVariant>
#include <QHash>
#include <QSet>
#include <QColor>
#include <QList>

enum class CellType {
    Empty,
    Number,
    Percent,
    Currency,
    Date,
    DateTime,
    Bool,
    Text,
    Formula
};

struct CellFormat {
    bool bold = false;
    bool italic = false;
    QString bgColor;
    QString textColor;
    int alignment = 0;
    QString numberFormat;
};

struct CellValue {
    CellType type = CellType::Empty;
    QVariant value;
    QString displayText;
};

class Sheet {
public:
    Sheet();

    QString name;

    QVariant rawValue(const QString& cell) const;
    void setRawValue(const QString& cell, const QString& raw);

    QVariant computedValue(const QString& cell, QSet<QString>& visiting) const;
    CellValue typedValue(const QString& cell, QSet<QString>& visiting) const;

    CellFormat format(const QString& cell) const;
    void setFormat(const QString& cell, const CellFormat& fmt);

    QStringList cellsInRange(const QString& from, const QString& to) const;

    QHash<QString, QString> allRaw() const { return m_raw; }
    QHash<QString, CellFormat> allFormats() const { return m_formats; }
    void loadRaw(const QHash<QString, QString>& r, const QHash<QString, CellFormat>& f);

    static QPair<int, int> parseRef(const QString& ref);
    static QString makeRef(int row, int col);

    static CellValue detectType(const QString& raw, Sheet* sheet, const QString& ref);

private:
    QHash<QString, QString> m_raw;
    QHash<QString, CellFormat> m_formats;
    mutable QHash<QString, QVariant> m_cache;
};