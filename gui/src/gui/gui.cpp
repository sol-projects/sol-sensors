#include "gui/MainWindow.hpp"
#include <QApplication>

namespace gui
{
    void run(int argc, char* argv[])
    {
        QApplication app(argc, argv);
        MainWindow mainWindow;
        mainWindow.show();
        app.exec();
    }
}
