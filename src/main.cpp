#include "mainwindow.h"
#include <QApplication>

using namespace std;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QFile f("../submodules/QDarkStyleSheet/qdarkstyle/style.qss");

    if (!f.exists())
    {
        printf("Unable to set stylesheet, file not found\n");
    }
    else
    {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);
        app.setStyleSheet(ts.readAll());
    }

    QResource::registerResource("../submodules/QDarkStyleSheet/qdarkstyle/style.qrc");

    mMainWindow window;
    window.show();
    return app.exec();
}
