#pragma once
#include <QMainWindow>
#include "sheet.hpp"

class QTabWidget;
class TitleBar;
class Toolbar;
class SpreadsheetView;
class QLineEdit;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

private slots:
    void onCellSelected(const QString& ref, const QString& raw);
    void onFormulaEntered();
    void onFormulaCellClicked(const QString& ref);
    void onBoldToggled(bool);
    void onItalicToggled(bool);
    void onAlignChanged(int);
    void onBgColorChanged(const QString&);
    void onTextColorChanged(const QString&);
    void onSaveRequested();
    void onOpenRequested();
    void onNewSheetRequested();
    void onTabChanged(int);

private:
    void addNewSheet(const QString& name);
    SpreadsheetView* currentView() const;
    Sheet* currentSheet() const;

    TitleBar* m_titleBar;
    Toolbar* m_toolbar;
    QTabWidget* m_tabs;
    QLineEdit* m_cellRefBox;
    QLineEdit* m_formulaBox;
    QLabel* m_status;

    struct SheetBundle {
        Sheet* sheet;
        SpreadsheetView* view;
    };
    QList<SheetBundle> m_sheets;
    QString m_filePath;
};