#include "mainwindow.h"

//#include "gnuplot-iostream.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);    
    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale::Russian, "qt", "_",
        QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&qtTranslator);
    }
    MainWindow w;
    w.show();
    return app.exec();
}
