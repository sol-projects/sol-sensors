#include "gui/MainWindow.hpp"
#include "sensors/error.hpp"
#include <QApplication>
#include <QCheckBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QThread>
#include <cstdio>
#include <cstdlib>

#ifndef LIVE_SENSORS
namespace
{
    bool noDevices = false;
}
namespace sensors
{
    std::vector<Device> getDevices(Device::Type type)
    {
        if (noDevices)
        {
            return {};
        }
        return { { Device::generateID(), "Test device", type, 0, 0 } };
    }
    int getLoad(const Device& device, int)
    {
        return device.type == Device::Type::GPU ? error::code : 1250;
    }
    int getTemp(const Device&, int) { return error::code; }
}
#endif

void check(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "%s\n", message);
        std::abort();
    }
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    for (int i = 0; i < 12; i++)
    {
        QThread::msleep(100);
        app.processEvents();
    }
    auto plots = window.findChildren<QCustomPlot*>();
    check(!plots.empty(), "No plots created");
    for (auto plot : plots)
    {
        check(plot->graphCount() > 0, "Empty plot");
        check(plot->graph(0)->dataCount() > 0, "Timer did not collect samples");
        for (int i = 0; i < plot->graphCount(); i++)
        {
            for (const auto& point : *plot->graph(i)->data())
            {
                check(std::isnan(point.value) || point.value >= 0, "Unsupported reading was plotted as a negative value");
            }
            plot->graph(i)->addData(-100, 0);
        }
    }
    QThread::msleep(310);
    app.processEvents();
    for (auto plot : plots)
    {
        check(plot->graph(0)->data()->constBegin()->key >= 0, "Old graph data was retained");
    }
    if (argc > 1)
    {
        check(window.grab().save(argv[1]), "Could not save desktop screenshot");
    }
    if (argc > 3)
    {
        check(window.findChild<QScrollArea*>()->widget()->grab().save(argv[3]), "Could not save full dashboard screenshot");
    }
    window.resize(600, 800);
    app.processEvents();
    app.processEvents();
    auto scroll = window.findChild<QScrollArea*>();
    if (scroll && scroll->widget()->width() > scroll->viewport()->width())
    {
        std::fprintf(stderr, "Content %d, viewport %d, minimum %d\n", scroll->widget()->width(), scroll->viewport()->width(), scroll->widget()->minimumSizeHint().width());
    }
    check(scroll && scroll->widget()->width() <= scroll->viewport()->width(), "Narrow layout overflows");
    if (argc > 2)
    {
        check(window.grab().save(argv[2]), "Could not save narrow screenshot");
    }
    for (auto box : window.findChildren<QCheckBox*>())
    {
        if (box->isEnabled())
        {
            box->setFocus();
            check(box->hasFocus(), "Checkbox cannot receive keyboard focus");
            box->click();
        }
    }
    app.processEvents();
    for (auto plot : plots)
    {
        check(!plot->isVisible(), "Hidden graph is still visible");
    }
    bool emptyMessage = false;
    for (auto label : window.findChildren<QLabel*>())
    {
        emptyMessage |= label->isVisible() && label->text().startsWith("Select a graph");
    }
    check(emptyMessage, "Missing empty selection message");
    for (auto box : window.findChildren<QCheckBox*>())
    {
        if (box->isEnabled())
        {
            box->click();
        }
    }
    app.processEvents();
    for (auto plot : plots)
    {
        check(plot->isVisible(), "Graph did not return after toggling");
    }
#ifndef LIVE_SENSORS
    noDevices = true;
    MainWindow empty;
    empty.show();
    app.processEvents();
    check(empty.findChildren<QCustomPlot*>().empty(), "Missing hardware created fake graphs");
    for (auto box : empty.findChildren<QCheckBox*>())
    {
        check(!box->isEnabled(), "Missing hardware left its control enabled");
    }
#endif
    return 0;
}
