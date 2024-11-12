#pragma once
#include "gui/AxisTag.hpp"
#include "qcustomplot.h"
#include <QMainWindow>
#include <QWidget>
#include <QWidgetData>
#include <thread>
#include <vector>
#include "sensors/sensors.hpp"
#include <unordered_map>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    // void addWidgetFunc(QWidget *widget);
    virtual ~MainWindow();

private slots:
    void update();
private:
    using DeviceType = sensors::Device::Type;
    void createPlot(sensors::Device::Type deviceType, const QString& plotTitle);

    Ui::MainWindow* ui;
    QVBoxLayout* rightLayout;
    std::unordered_map<DeviceType, QCustomPlot*> plots;

    std::vector<sensors::Device> cpuThreads;
    sensors::Device cpuDevice;
    sensors::Device ramDevice;
    QTimer updateTimer;
};

