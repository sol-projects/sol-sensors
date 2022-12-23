#include <QtWidgets/QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QPushButton>
#include "mainwindow.h"

/* Example of Grid Layout */

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget *window = new QWidget;
    window->setWindowTitle("My App");

    QGridLayout *layout = new QGridLayout;

    QLabel *label1 = new QLabel("Name");
    QLineEdit *textName1 = new QLineEdit;

    QLabel *label2 = new QLabel("Name");
    QLineEdit *textName2 = new QLineEdit;

    layout->addWidget(label1, 0, 0);
    layout->addWidget(textName1, 0, 1);

    layout->addWidget(label2, 1, 0);
    layout->addWidget(textName2, 1, 1);

    window->setLayout(layout);
    MainWindow mainWindov = new MainWindow();
    mainWindov.show();
    return app.exec();
}
