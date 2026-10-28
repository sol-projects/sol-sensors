#pragma once
#include "qcustomplot.h"
#include "sensors/sensors.hpp"
#include <QElapsedTimer>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QTimer>
#include <vector>

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
    virtual ~MainWindow();

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void update();

private:
    using DeviceType = sensors::Device::Type;
    struct Plot
    {
        QWidget* container;
        QCustomPlot* graph;
        QLabel* value;
        QLabel* temperature;
        std::vector<sensors::Device> devices;
        DeviceType type;
    };

    void createPlot(const std::vector<sensors::Device>& devices, const QString& title);
    void arrangePlots();

    Ui::MainWindow* ui;
    QGridLayout* plotLayout;
    QLabel* emptyState;
    std::vector<Plot> plots;
    QTimer updateTimer;
    QElapsedTimer elapsed;
    int columns = 0;
};
