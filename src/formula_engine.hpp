#pragma once
#include <QString>
#include <QVariant>
#include <QSet>
#include <QList>
#include <QDate>
#include <QDateTime>

class Sheet;

class FormulaEngine {
public:
    static QVariant evaluate(const QString& raw, Sheet* sheet, const QString& currentCell);

    // Publiczne — używane przez wewnętrzny Parser
    static QVariant resolveCell(const QString& ref, Sheet* sheet,
                                const QString& cell, QSet<QString>& visiting);
    static QVariant callFunction(const QString& name, const QList<QVariant>& args);

private:
    static QVariant evalExpression(const QString& expr, Sheet* sheet,
                                    const QString& cell, QSet<QString>& visiting);
};