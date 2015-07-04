#include <QApplication>
#include <QMessageBox>

#include "window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Window window;
    if(window.error())
    {
        QMessageBox::critical(0, QObject::tr("Error"), QObject::tr("I'm sorry, something went wrong when I was trying to open the database. This isn't something you're likely to be able to solve without the developer.") );
        return 1;
    }

    window.show();
    return app.exec();
}

