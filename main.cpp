#include "mainwindow.h"
#include <QApplication>

using namespace std;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    mMainWindow window;
    window.show();
    return app.exec();
}
