#include <QApplication>

#include "window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Window window;
    if(window.error())
        return 1;

    window.show();
    return app.exec();
}
