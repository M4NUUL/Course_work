#include <QApplication>
#include "ui/LoginWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    LoginWindow w;
    w.show();

    return app.exec();
}
