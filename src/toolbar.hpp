#pragma once
#include <QWidget>
#include "sheet.hpp"

class QToolButton;

class Toolbar : public QWidget {
    Q_OBJECT
public:
    explicit Toolbar(QWidget* parent = nullptr);
    void setFormat(const CellFormat& fmt);

signals:
    void boldToggled(bool);
    void italicToggled(bool);
    void alignChanged(int);
    void bgColorChanged(const QString&);
    void textColorChanged(const QString&);
    void clearSelectionRequested();
    void saveRequested();
    void openRequested();
    void newSheetRequested();

private:
    QToolButton* m_bold;
    QToolButton* m_italic;
    QToolButton* m_alignL;
    QToolButton* m_alignC;
    QToolButton* m_alignR;
};