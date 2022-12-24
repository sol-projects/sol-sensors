#include "gui/MainWindow.hpp"
#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace gui
{
    void run(int argc, char* argv[])
    {
        QApplication app(argc, argv);

        QWidget* window = new QWidget;
        window->setWindowTitle("SOL Sensors");

        QGridLayout* layout = new QGridLayout;

        QLabel* label1 = new QLabel("Name");
        QLineEdit* textName1 = new QLineEdit;

        QLabel* label2 = new QLabel("Name");
        QLineEdit* textName2 = new QLineEdit;

        layout->addWidget(label1, 0, 0);
        layout->addWidget(textName1, 0, 1);

        layout->addWidget(label2, 1, 0);
        layout->addWidget(textName2, 1, 1);

        window->setLayout(layout);
        MainWindow mainWindov = new MainWindow();
        mainWindov.show();

        app.exec();
    }
}
