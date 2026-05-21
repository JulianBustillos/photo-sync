#include "PhotoSync.h"
#include "NodeContainer.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<NodeContainer>();

    PhotoSync window;
    window.show();
    return app.exec();
}
