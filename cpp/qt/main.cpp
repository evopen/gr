#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char* argv[])
{
    QCommandLineParser parser;
    parser.setApplicationDescription("wtf");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
