#ifdef NOGUI
#include "nogui/include/nogui/nogui.hpp"
#include "nogui/include/nogui/options.hpp"
#ifdef TESTS
#include "tests/nogui.hpp"
#endif
#endif

#ifdef TESTS
#include "tests/config.hpp"
#include "tests/csv.hpp"
#include "tests/sensors.hpp"
#include "tests/stats.hpp"
#include <DOCTEST/doctest.h>
#endif

#ifdef GUI
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QPushButton>
#include "gui/include/gui/mainwindow.h"
#include <thread>
#endif
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
#ifdef NOGUI
    nogui::run(argc, argv);
#endif

#ifdef GUI
    QApplication app(argc, argv);

    QWidget *window = new QWidget;
    window->setWindowTitle("SOL Sensors");

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
#endif
#ifdef TESTS
    doctest::Context doctestContext;
    [[maybe_unused]]int result = doctestContext.run();

    if (doctestContext.shouldExit())
    {
        #ifndef GUI
        return result;
        #endif
    }

    #ifndef GUI
    return result;
    #endif
#endif

#ifdef GUI
    app.exec();
#endif
    return 0;
}
