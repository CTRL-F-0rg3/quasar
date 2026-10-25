#include "main_window.hpp"
#include "title_bar.hpp"
#include "cosmic_background.hpp"
#include "spreadsheet_view.hpp"
#include "toolbar.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QMenuBar>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStatusBar>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(1400, 950);
    setWindowTitle("Quasar");

    auto* central = new CosmicBackground(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_titleBar = new TitleBar(central);
    layout->addWidget(m_titleBar);

    auto* fileMenu = m_titleBar->menuBar()->addMenu("File");
    fileMenu->addAction("New Sheet", QKeySequence::New, this, &MainWindow::onNewSheetRequested);
    fileMenu->addAction("Open", QKeySequence::Open, this, &MainWindow::onOpenRequested);
    fileMenu->addAction("Save", QKeySequence::Save, this, &MainWindow::onSaveRequested);

    m_toolbar = new Toolbar(central);
    layout->addWidget(m_toolbar);

    // === Formula bar ze skórką ===
    auto* formulaBar = new QWidget(central);
    formulaBar->setObjectName("formulaBar");
    formulaBar->setStyleSheet(
        "QWidget#formulaBar {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 rgba(30, 20, 45, 230),"
        "    stop:1 rgba(15, 10, 22, 230));"
        "  border-bottom: 1px solid rgba(255, 45, 149, 80);"
        "}"
        "QLineEdit#cellRefBox {"
        "  background: rgba(255, 45, 149, 200);"
        "  color: white;"
        "  border: 1px solid #ff2d95;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "}"
        "QLabel#fxLabel {"
        "  color: #94e2d5;"
        "  font-style: italic;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QLineEdit#formulaBox {"
        "  background: rgba(0, 0, 0, 200);"
        "  color: #f0e8ff;"
        "  border: 1px solid #3a2a4d;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  font-family: 'Liberation Mono', monospace;"
        "  font-size: 12px;"
        "  selection-background-color: #ff2d95;"
        "}"
        "QLineEdit#formulaBox:focus {"
        "  border: 1px solid #ff2d95;"
        "}"
    );
    auto* fbLayout = new QHBoxLayout(formulaBar);
    fbLayout->setContentsMargins(12, 8, 12, 8);
    fbLayout->setSpacing(8);

    m_cellRefBox = new QLineEdit(formulaBar);
    m_cellRefBox->setObjectName("cellRefBox");
    m_cellRefBox->setFixedWidth(90);
    m_cellRefBox->setReadOnly(true);
    m_cellRefBox->setAlignment(Qt::AlignCenter);
    fbLayout->addWidget(m_cellRefBox);

    auto* fxLabel = new QLabel("fx", formulaBar);
    fxLabel->setObjectName("fxLabel");
    fbLayout->addWidget(fxLabel);

    m_formulaBox = new QLineEdit(formulaBar);
    m_formulaBox->setObjectName("formulaBox");
    fbLayout->addWidget(m_formulaBox);

    m_formulaBox->installEventFilter(this);

    connect(m_formulaBox, &QLineEdit::returnPressed, this, &MainWindow::onFormulaEntered);
    connect(m_formulaBox, &QLineEdit::textChanged, this, [this](const QString& t){
        if (auto* v = currentView()) v->setFormulaMode(t.startsWith('='));
    });

    layout->addWidget(formulaBar);

    m_tabs = new QTabWidget(central);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->setStyleSheet(
        "QTabWidget::pane { border: none; background: rgba(0,0,0,0.3); }"
        "QTabBar::tab { background: rgba(0,0,0,0.5); color: #8a7a95; padding: 8px 22px; margin-right: 2px; margin-top: 6px; border: 1px solid rgba(255,255,255,0.1); border-bottom: none; border-top-left-radius: 8px; border-top-right-radius: 8px; }"
        "QTabBar::tab:selected { background: transparent; color: #ff2d95; padding: 12px 22px 8px 22px; margin-top: 0; border: 1px solid #ff2d95; border-bottom: none; }"
    );
    layout->addWidget(m_tabs, 1);

    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    setCentralWidget(central);

    m_status = new QLabel("Ready", this);
    m_status->setStyleSheet("QLabel { color: #8a7a95; padding: 4px 12px; }");
    statusBar()->addWidget(m_status);

    connect(m_toolbar, &Toolbar::boldToggled, this, &MainWindow::onBoldToggled);
    connect(m_toolbar, &Toolbar::italicToggled, this, &MainWindow::onItalicToggled);
    connect(m_toolbar, &Toolbar::alignChanged, this, &MainWindow::onAlignChanged);
    connect(m_toolbar, &Toolbar::bgColorChanged, this, &MainWindow::onBgColorChanged);
    connect(m_toolbar, &Toolbar::textColorChanged, this, &MainWindow::onTextColorChanged);
    connect(m_toolbar, &Toolbar::saveRequested, this, &MainWindow::onSaveRequested);
    connect(m_toolbar, &Toolbar::openRequested, this, &MainWindow::onOpenRequested);
    connect(m_toolbar, &Toolbar::newSheetRequested, this, &MainWindow::onNewSheetRequested);

    addNewSheet("Sheet1");
}

bool MainWindow::eventFilter(QObject* obj, QEvent* e) {
    if (obj == m_formulaBox && e->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(e);
        if (ke->key() == Qt::Key_Escape) {
            m_formulaBox->clear();
            if (auto* v = currentView()) v->setFormulaMode(false);
            if (auto* v = currentView()) {
                int r = v->currentRow(), c = v->currentColumn();
                if (r >= 0 && c >= 0) {
                    QString ref = Sheet::makeRef(r, c);
                    m_formulaBox->setText(currentSheet()->rawValue(ref).toString());
                }
            }
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, e);
}

SpreadsheetView* MainWindow::currentView() const {
    int idx = m_tabs->currentIndex();
    if (idx < 0 || idx >= m_sheets.size()) return nullptr;
    return m_sheets[idx].view;
}

Sheet* MainWindow::currentSheet() const {
    int idx = m_tabs->currentIndex();
    if (idx < 0 || idx >= m_sheets.size()) return nullptr;
    return m_sheets[idx].sheet;
}

void MainWindow::addNewSheet(const QString& name) {
    auto* sheet = new Sheet();
    sheet->name = name;
    auto* view = new SpreadsheetView(sheet);
    connect(view, &SpreadsheetView::cellSelected, this, &MainWindow::onCellSelected);
    connect(view, &SpreadsheetView::cellEdited, this, [this]{
        setWindowTitle("Quasar *");
    });
    connect(view, &SpreadsheetView::formulaCellClicked, this, &MainWindow::onFormulaCellClicked);
    int idx = m_tabs->addTab(view, name);
    m_tabs->setCurrentIndex(idx);
    m_sheets.append({sheet, view});
}

void MainWindow::onTabChanged(int idx) {
    if (idx < 0 || idx >= m_sheets.size()) return;
    auto bundle = m_sheets[idx];
    int r = bundle.view->currentRow(), c = bundle.view->currentColumn();
    if (r >= 0 && c >= 0) {
        QString ref = Sheet::makeRef(r, c);
        onCellSelected(ref, bundle.sheet->rawValue(ref).toString());
    }
}

void MainWindow::onCellSelected(const QString& ref, const QString& raw) {
    m_cellRefBox->setText(ref);
    // W trybie edycji formuły nie nadpisujemy formula boxa
    if (m_formulaBox->hasFocus() && m_formulaBox->text().startsWith('=')) return;
    m_formulaBox->setText(raw);
}

void MainWindow::onFormulaEntered() {
    auto* view = currentView();
    auto* sheet = currentSheet();
    if (!view || !sheet) return;
    int r = view->currentRow(), c = view->currentColumn();
    if (r < 0 || c < 0) return;
    QString ref = Sheet::makeRef(r, c);
    sheet->setRawValue(ref, m_formulaBox->text());
    view->refreshAll();
    if (auto* m = view->sheetModel()) m->refreshAll();
    setWindowTitle("Quasar *");
}

void MainWindow::onFormulaCellClicked(const QString& ref) {
    if (!m_formulaBox->text().startsWith('=')) return;
    m_cellRefBox->setText(ref);
    m_formulaBox->setText(m_formulaBox->text() + ref);
    m_formulaBox->setFocus();
    m_formulaBox->setCursorPosition(m_formulaBox->text().length());
}

void MainWindow::onBoldToggled(bool b) {
    auto* view = currentView(); if (!view) return;
    CellFormat fmt = view->currentFormat(); fmt.bold = b;
    view->applyFormatToSelection(fmt);
}

void MainWindow::onItalicToggled(bool b) {
    auto* view = currentView(); if (!view) return;
    CellFormat fmt = view->currentFormat(); fmt.italic = b;
    view->applyFormatToSelection(fmt);
}

void MainWindow::onAlignChanged(int a) {
    auto* view = currentView(); if (!view) return;
    CellFormat fmt = view->currentFormat(); fmt.alignment = a;
    view->applyFormatToSelection(fmt);
}

void MainWindow::onBgColorChanged(const QString& c) {
    auto* view = currentView(); if (!view) return;
    CellFormat fmt = view->currentFormat(); fmt.bgColor = c;
    view->applyFormatToSelection(fmt);
}

void MainWindow::onTextColorChanged(const QString& c) {
    auto* view = currentView(); if (!view) return;
    CellFormat fmt = view->currentFormat(); fmt.textColor = c;
    view->applyFormatToSelection(fmt);
}

void MainWindow::onNewSheetRequested() {
    addNewSheet(QString("Sheet%1").arg(m_sheets.size() + 1));
}

void MainWindow::onSaveRequested() {
    QString path = QFileDialog::getSaveFileName(this, "Save",
        QDir::homePath() + "/document.qsheet", "Quasar (*.qsheet);;JSON (*.json)");
    if (path.isEmpty()) return;

    QJsonObject root;
    QJsonArray sheetsArr;
    for (const auto& b : m_sheets) {
        QJsonObject sObj;
        sObj["name"] = b.sheet->name;
        QJsonObject cells;
        for (auto it = b.sheet->allRaw().begin(); it != b.sheet->allRaw().end(); ++it)
            cells[it.key()] = it.value();
        sObj["cells"] = cells;
        QJsonObject formats;
        for (auto it = b.sheet->allFormats().begin(); it != b.sheet->allFormats().end(); ++it) {
            QJsonObject f;
            f["bold"] = it.value().bold;
            f["italic"] = it.value().italic;
            f["bg"] = it.value().bgColor;
            f["fg"] = it.value().textColor;
            f["align"] = it.value().alignment;
            formats[it.key()] = f;
        }
        sObj["formats"] = formats;
        sheetsArr.append(sObj);
    }
    root["sheets"] = sheetsArr;
    root["version"] = 1;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(QJsonDocument(root).toJson());
    f.close();
    m_filePath = path;
    setWindowTitle("Quasar — " + QFileInfo(path).fileName());
    m_status->setText("Saved: " + path);
}

void MainWindow::onOpenRequested() {
    QString path = QFileDialog::getOpenFileName(this, "Open",
        QDir::homePath(), "Quasar (*.qsheet);;JSON (*.json)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) { QMessageBox::warning(this, "Error", "Not valid Quasar file"); return; }

    while (m_tabs->count() > 0) { m_tabs->removeTab(0); m_sheets.removeLast(); }

    QJsonArray sheetsArr = doc.object()["sheets"].toArray();
    for (const auto& v : sheetsArr) {
        QJsonObject sObj = v.toObject();
        auto* sheet = new Sheet();
        sheet->name = sObj["name"].toString();
        QHash<QString, QString> raw;
        QHash<QString, CellFormat> fmts;
        for (auto it = sObj["cells"].toObject().begin(); it != sObj["cells"].toObject().end(); ++it)
            raw[it.key()] = it.value().toString();
        for (auto it = sObj["formats"].toObject().begin(); it != sObj["formats"].toObject().end(); ++it) {
            QJsonObject ff = it.value().toObject();
            CellFormat fmt;
            fmt.bold = ff["bold"].toBool();
            fmt.italic = ff["italic"].toBool();
            fmt.bgColor = ff["bg"].toString();
            fmt.textColor = ff["fg"].toString();
            fmt.alignment = ff["align"].toInt();
            fmts[it.key()] = fmt;
        }
        sheet->loadRaw(raw, fmts);
        auto* view = new SpreadsheetView(sheet);
        connect(view, &SpreadsheetView::cellSelected, this, &MainWindow::onCellSelected);
        connect(view, &SpreadsheetView::cellEdited, this, [this]{ setWindowTitle("Quasar *"); });
        connect(view, &SpreadsheetView::formulaCellClicked, this, &MainWindow::onFormulaCellClicked);
        int idx = m_tabs->addTab(view, sheet->name);
        m_sheets.append({sheet, view});
        m_tabs->setCurrentIndex(idx);
        view->refreshAll();
    }
    m_filePath = path;
    setWindowTitle("Quasar — " + QFileInfo(path).fileName());
    m_status->setText("Opened: " + path);
}