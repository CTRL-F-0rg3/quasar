#pragma once
#include <QWidget>
class QLabel;
class QMenuBar;

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget* parent = nullptr);
    void setTitle(const QString& title);
    QMenuBar* menuBar() const { return m_menuBar; }
protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
private:
    QLabel* m_title;
    QMenuBar* m_menuBar;
};