#include "photo_sync.hpp"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    PhotoSync window;
    window.show();
    return QApplication::exec();
}
