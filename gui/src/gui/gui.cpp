#include "gui/MainWindow.hpp"
#include <QApplication>
#include <QScrollArea>

namespace gui
{
    void run(int argc, char* argv[])
    {
        QApplication app(argc, argv);

        MainWindow mainWindow;

        QScrollArea* scrollArea = new QScrollArea;
        scrollArea->setWidgetResizable(true);
        scrollArea->setWidget(&mainWindow);
        scrollArea->setStyleSheet("QScrollArea { border: none; }");

        scrollArea->show();

        app.exec();
    }
}

