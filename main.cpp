#include "mainwindow.h"
#include "application_controller.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    application_controller c;
    w.setController(&c);
    w.start();
    w.show();
    return a.exec();
}
