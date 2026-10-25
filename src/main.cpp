#include <QApplication>
#include <QStyleFactory>
#include "main_window.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("Quasar");
    app.setOrganizationName("Trangorge");
    app.setStyle(QStyleFactory::create("Fusion"));
    MainWindow w;
    w.show();
    return app.exec();
}