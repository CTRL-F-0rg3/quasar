#pragma once
#include <QAbstractTableModel>
#include "sheet.hpp"

class SpreadsheetModel : public QAbstractTableModel {
    Q_OBJECT
public:
    static constexpr int MAX_ROWS = 1048576;
    static constexpr int MAX_COLS = 16384;

    explicit SpreadsheetModel(Sheet* sheet, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& idx, int role) const override;
    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& idx) const override;
    bool setData(const QModelIndex& idx, const QVariant& value, int role) override;

    Sheet* sheet() const { return m_sheet; }

    void refreshAll();
    void refreshCell(int row, int col);

signals:
    void edited();

private:
    Sheet* m_sheet;
    static QString columnLabel(int col);
};