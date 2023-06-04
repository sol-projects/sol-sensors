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
        scrollArea->setWidgetResizable(true); // Allow the scroll area to resize its contents
        scrollArea->setWidget(&mainWindow);
        scrollArea->setStyleSheet("QScrollArea { border: none; }"); // Remove the border around the scroll area

        scrollArea->show();

        app.exec();
    }
}

