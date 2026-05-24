#include "PhotoSync.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    PhotoSync window;
    window.show();
    return app.exec();
}
