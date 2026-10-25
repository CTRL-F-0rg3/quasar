#pragma once
#include <QTableView>
#include "sheet.hpp"
#include "spreadsheet_model.hpp"

class SpreadsheetView : public QTableView {
    Q_OBJECT
public:
    explicit SpreadsheetView(Sheet* sheet, QWidget* parent = nullptr);

    void refreshAll();
    void applyFormatToSelection(const CellFormat& fmt);
    void clearSelectionContent();
    CellFormat currentFormat() const;

    Sheet* sheet() const { return m_sheet; }
    SpreadsheetModel* sheetModel() const { return m_model; }

    int currentRow() const { return currentIndex().row(); }
    int currentColumn() const { return currentIndex().column(); }

    void setFormulaMode(bool on) { m_formulaMode = on; }

signals:
    void cellSelected(const QString& ref, const QString& rawFormula);
    void cellEdited();
    void formulaCellClicked(const QString& ref);

protected:
    void keyPressEvent(QKeyEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;

private:
    void setupStyle();
    Sheet* m_sheet;
    SpreadsheetModel* m_model;
    bool m_formulaMode = false;
};