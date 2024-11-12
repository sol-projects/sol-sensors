#include "gui/MainWindow.hpp"
#include "sensors/error.hpp"
#include "sensors/sensors.hpp"
#include <QLayout>
#include <fstream>
#include "gui/ui_mainwindow.h"
#include <QColor>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>

class ColorSquareWidget : public QWidget
{
public:
    explicit ColorSquareWidget(QWidget* parent = nullptr, const QColor& color = Qt::black)
    : QWidget(parent), color(color)
    {
        setFixedSize(20, 20);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setBrush(QBrush(color));
        painter.drawRect(0, 0, width(), height());
    }

private:
    QColor color;
};

MainWindow::MainWindow(QWidget* parent)
: QMainWindow(parent)
, ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QSplitter* mainSplitter = new QSplitter(this);
    ui->centralwidget->setLayout(new QVBoxLayout());
    ui->centralwidget->layout()->addWidget(mainSplitter);

    QWidget* leftPanel = new QWidget(mainSplitter);
    leftPanel->setFixedWidth(300);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);

    QLabel* controlTitle = new QLabel("Graphs Control", leftPanel);
    controlTitle->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(controlTitle);

    std::unordered_map<DeviceType, QCheckBox*> graphCheckBoxes;

    for (const auto& deviceType : {DeviceType::CPUThread, DeviceType::CPU, DeviceType::RAM}) {
        QString graphName;
        switch (deviceType) {
            case DeviceType::CPUThread: graphName = "CPU Threads Usage"; break;
            case DeviceType::CPU: graphName = "CPU"; break;
            case DeviceType::RAM: graphName = "RAM Usage"; break;
        }

        QGroupBox* graphGroup = new QGroupBox(graphName, leftPanel);
        graphGroup->setCheckable(true);
        graphGroup->setChecked(true);
        leftLayout->addWidget(graphGroup);

        QVBoxLayout* groupLayout = new QVBoxLayout(graphGroup);

        QCheckBox* showCheckbox = new QCheckBox("Show", graphGroup);
        showCheckbox->setChecked(true);
        groupLayout->addWidget(showCheckbox);

        connect(showCheckbox, &QCheckBox::toggled, this, [=](bool checked) {
            if (plots.find(deviceType) != plots.end()) {
                plots[deviceType]->setVisible(checked);
            }
        });
        graphCheckBoxes[deviceType] = showCheckbox;
    }

    leftLayout->addStretch(1);

    QWidget* rightPanel = new QWidget(mainSplitter);
    rightLayout = new QVBoxLayout(rightPanel);

    cpuThreads = sensors::getDevices(DeviceType::CPUThread);
    cpuDevice = sensors::getDevices(DeviceType::CPU)[0];
    ramDevice = sensors::getDevices(DeviceType::RAM)[0];

    createPlot(DeviceType::CPUThread, "CPU Threads Usage");
    createPlot(DeviceType::CPU, "CPU");
    createPlot(DeviceType::RAM, "RAM Usage");

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 2);

    connect(&updateTimer, &QTimer::timeout, this, &MainWindow::update);
    updateTimer.start(300);
}

void MainWindow::createPlot(sensors::Device::Type deviceType, const QString& plotTitle)
{
    auto plot = new QCustomPlot();
    plot->yAxis->setRange(0.0, 100.0);
    if (deviceType == DeviceType::RAM) {
        plot->yAxis->setRange(0.0, 20.0);
    }
    plot->setMinimumHeight(200);
    plot->setAntialiasedElements(QCP::aeAll);
    plot->plotLayout()->insertRow(0);
    plot->plotLayout()->addElement(0, 0, new QCPTextElement(plot, plotTitle, QFont("sans", 12, QFont::Bold)));
    plots[deviceType] = plot;

    if (deviceType == DeviceType::CPUThread) {
        QVector<QColor> cpuThreadColors = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta, Qt::yellow};
        for (size_t i = 0; i < cpuThreads.size(); ++i) {
            plot->addGraph();
            plot->graph(i)->setPen(QPen(cpuThreadColors[i % cpuThreadColors.size()]));
        }
    } else if (deviceType == DeviceType::CPU) {
        plot->addGraph();
        plot->graph(0)->setPen(QPen(Qt::darkGreen));
        plot->addGraph();
        plot->graph(1)->setPen(QPen(Qt::darkRed));
    } else if (deviceType == DeviceType::RAM) {
        plot->addGraph();
        plot->graph(0)->setPen(QPen(Qt::blue));
    }

    QWidget* plotContainer = new QWidget();
    QVBoxLayout* containerLayout = new QVBoxLayout(plotContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(6);

    containerLayout->addWidget(plot);

    QHBoxLayout* legendLayout = new QHBoxLayout();

    if (deviceType == DeviceType::CPUThread) {
        QVector<QColor> cpuThreadColors = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta, Qt::yellow};
        for (size_t i = 0; i < cpuThreads.size(); ++i) {
            ColorSquareWidget* colorSquare = new ColorSquareWidget(nullptr, cpuThreadColors[i % cpuThreadColors.size()]);
            legendLayout->addWidget(colorSquare);

            QString labelText = QString("Thread #%1").arg(i);
            QLabel* label = new QLabel(labelText);
            legendLayout->addWidget(label);
        }
    }
    else if (deviceType == DeviceType::CPU) {
        ColorSquareWidget* loadColorSquare = new ColorSquareWidget(nullptr, Qt::darkGreen);
        ColorSquareWidget* tempColorSquare = new ColorSquareWidget(nullptr, Qt::darkRed);
        legendLayout->addWidget(loadColorSquare);
        legendLayout->addWidget(new QLabel(QString::fromStdString(cpuDevice.name) + " Load"));
        legendLayout->addWidget(tempColorSquare);
        legendLayout->addWidget(new QLabel(QString::fromStdString(cpuDevice.name) + " Temperature"));
    }
    else if (deviceType == DeviceType::RAM) {
        ColorSquareWidget* ramColorSquare = new ColorSquareWidget(nullptr, Qt::blue);
        legendLayout->addWidget(ramColorSquare);
        legendLayout->addWidget(new QLabel(QString::fromStdString(ramDevice.name) + " Load"));
    }

    containerLayout->addLayout(legendLayout);

    rightLayout->addWidget(plotContainer);
}

void MainWindow::update()
{
    int precision = 2;

    auto& cpuThreadPlot = plots[DeviceType::CPUThread];
    int i = 0;
    for(const auto& thread : cpuThreads)
    {
        auto load = sensors::getLoad(thread, precision);
        double point = load / std::pow(10, precision);

        int dataCount = cpuThreadPlot->graph(i)->dataCount();
        double lastPoint = (dataCount > 0) ? cpuThreadPlot->graph(i)->dataMainValue(dataCount - 1) : point;

        int numInterpolatedPoints = 5;
        for (int j = 1; j <= numInterpolatedPoints; ++j)
        {
            double interpolatedValue = lastPoint + (point - lastPoint) * (j / static_cast<double>(numInterpolatedPoints));
            cpuThreadPlot->graph(i)->addData(dataCount + j, interpolatedValue);
        }
        i++;
    }
    cpuThreadPlot->xAxis->rescale();
    cpuThreadPlot->xAxis->setRange(cpuThreadPlot->graph(0)->dataCount(), 100, Qt::AlignRight);
    cpuThreadPlot->replot();

    auto& cpuPlot = plots[DeviceType::CPU];
    double cpuLoad = sensors::getLoad(cpuDevice, precision) / std::pow(10, precision);
    double cpuTemp = sensors::getTemp(cpuDevice, precision) / std::pow(10, precision);

    int dataCount = cpuPlot->graph(0)->dataCount();
    int numInterpolatedPoints = 5;
    double lastLoadPoint = (dataCount > 0) ? cpuPlot->graph(0)->dataMainValue(dataCount - 1) : cpuLoad;
    double lastTempPoint = (dataCount > 0) ? cpuPlot->graph(1)->dataMainValue(dataCount - 1) : cpuTemp;

    for (int j = 1; j <= numInterpolatedPoints; ++j)
    {
        double interpolatedLoad = lastLoadPoint + (cpuLoad - lastLoadPoint) * (j / static_cast<double>(numInterpolatedPoints));
        double interpolatedTemp = lastTempPoint + (cpuTemp - lastTempPoint) * (j / static_cast<double>(numInterpolatedPoints));
        cpuPlot->graph(0)->addData(dataCount + j, interpolatedLoad);
        cpuPlot->graph(1)->addData(dataCount + j, interpolatedTemp);
    }
    cpuPlot->xAxis->rescale();
    cpuPlot->xAxis->setRange(cpuPlot->graph(0)->dataCount(), 100, Qt::AlignRight);
    cpuPlot->replot();

    auto& ramPlot = plots[DeviceType::RAM];
    double ramLoad = sensors::getLoad(ramDevice, precision) / std::pow(10, precision);

    dataCount = ramPlot->graph(0)->dataCount();
    double lastRamPoint = (dataCount > 0) ? ramPlot->graph(0)->dataMainValue(dataCount - 1) : ramLoad;

    for (int j = 1; j <= numInterpolatedPoints; ++j)
    {
        double interpolatedRam = lastRamPoint + (ramLoad - lastRamPoint) * (j / static_cast<double>(numInterpolatedPoints));
        ramPlot->graph(0)->addData(dataCount + j, interpolatedRam);
    }
    ramPlot->xAxis->rescale();
    ramPlot->xAxis->setRange(ramPlot->graph(0)->dataCount(), 100, Qt::AlignRight);
    ramPlot->replot();
}

MainWindow::~MainWindow()
{
    delete ui;
}
