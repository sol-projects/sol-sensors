#include "gui/MainWindow.hpp"
#include "sensors/error.hpp"
#include "sensors/sensors.hpp"
#include <QLayout>
#include <fstream>
#include "gui/ui_mainwindow.h"
#include <QColor>  // Include for QColor
#include <QLabel>  // Include for QLabel
#include <QHBoxLayout>  // Include for QHBoxLayout
#include <QVBoxLayout>  // Include for QVBoxLayout
#include <QPainter>  // Include for QPainter (for custom painting)

class ColorSquareWidget : public QWidget
{
public:
    explicit ColorSquareWidget(QWidget* parent = nullptr, const QColor& color = Qt::black)
        : QWidget(parent), color(color)
    {
        setFixedSize(20, 20);  // Set size for the color square
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QPainter painter(this);
        painter.setBrush(QBrush(color));
        painter.drawRect(0, 0, width(), height());  // Draw a filled rectangle as color square
    }

private:
    QColor color;
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Create a splitter to divide the main window into two parts
    QSplitter* mainSplitter = new QSplitter(this);
    ui->centralwidget->setLayout(new QVBoxLayout());
    ui->centralwidget->layout()->addWidget(mainSplitter);

    // Left side: list widget for graph controls (30% of width)
    QWidget* leftPanel = new QWidget(mainSplitter);
    leftPanel->setFixedWidth(300);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);

    // Title for the left panel
    QLabel* controlTitle = new QLabel("Graphs Control", leftPanel);
    controlTitle->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(controlTitle);

    // Map to keep track of checkboxes for graph visibility
    std::unordered_map<DeviceType, QCheckBox*> graphCheckBoxes;

    // Create a collapsible section for each graph type
    for (const auto& deviceType : {DeviceType::CPUThread, DeviceType::CPU, DeviceType::RAM}) {
        QString graphName;
        switch (deviceType) {
            case DeviceType::CPUThread: graphName = "CPU Threads Usage"; break;
            case DeviceType::CPU: graphName = "CPU"; break;
            case DeviceType::RAM: graphName = "RAM Usage"; break;
        }

        // Create a collapsible group box for each graph type
        QGroupBox* graphGroup = new QGroupBox(graphName, leftPanel);
        graphGroup->setCheckable(true);
        graphGroup->setChecked(true);  // Show graph by default
        leftLayout->addWidget(graphGroup);

        // Layout for the group box content
        QVBoxLayout* groupLayout = new QVBoxLayout(graphGroup);

        // Show checkbox inside each group
        QCheckBox* showCheckbox = new QCheckBox("Show", graphGroup);
        showCheckbox->setChecked(true);  // Show graph by default
        groupLayout->addWidget(showCheckbox);

        // Connect the checkbox to toggle graph visibility
        connect(showCheckbox, &QCheckBox::toggled, this, [=](bool checked) {
            if (plots.find(deviceType) != plots.end()) {
                plots[deviceType]->setVisible(checked);
            }
        });
        graphCheckBoxes[deviceType] = showCheckbox;
    }

    leftLayout->addStretch(1);  // Add space at the bottom of the left panel

    // Right side: main area for plots (70% of width)
    QWidget* rightPanel = new QWidget(mainSplitter);
    rightLayout = new QVBoxLayout(rightPanel);

    // Initialize sensors and create plots on the right side
    cpuThreads = sensors::getDevices(DeviceType::CPUThread);
    cpuDevice = sensors::getDevices(DeviceType::CPU)[0];
    ramDevice = sensors::getDevices(DeviceType::RAM)[0];

    createPlot(DeviceType::CPUThread, "CPU Threads Usage");
    createPlot(DeviceType::CPU, "CPU");
    createPlot(DeviceType::RAM, "RAM Usage");

    // Add plots to right layout
    //for (const auto& plotPair : plots) {
    //    rightLayout->addWidget(plotPair.second);
    //}

    // Set the layouts in the splitter
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0, 1);  // 30% for left panel
    mainSplitter->setStretchFactor(1, 2);  // 70% for right panel

    // Set up the update timer
    connect(&updateTimer, &QTimer::timeout, this, &MainWindow::update);
    updateTimer.start(300);
}

void MainWindow::createPlot(sensors::Device::Type deviceType, const QString& plotTitle)
{
    // Initialize and configure the plot
    auto plot = new QCustomPlot();
    plot->yAxis->setRange(0.0, 100.0);
    if (deviceType == DeviceType::RAM) {
        plot->yAxis->setRange(0.0, 20.0);  // RAM-specific range
    }
    plot->setMinimumHeight(200);
    plot->setAntialiasedElements(QCP::aeAll);
    plot->plotLayout()->insertRow(0);
    plot->plotLayout()->addElement(0, 0, new QCPTextElement(plot, plotTitle, QFont("sans", 12, QFont::Bold)));
    plots[deviceType] = plot;  // Add plot to the map

    // Check if graphs should be added based on device type
if (deviceType == DeviceType::CPUThread) {
    QVector<QColor> cpuThreadColors = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta, Qt::yellow}; // Graph colors
    for (size_t i = 0; i < cpuThreads.size(); ++i) {
        plot->addGraph();  // Add a graph for each CPU thread
        plot->graph(i)->setPen(QPen(cpuThreadColors[i % cpuThreadColors.size()])); // Set graph color
    }
} else if (deviceType == DeviceType::CPU) {
    plot->addGraph();  // CPU Load graph
    plot->graph(0)->setPen(QPen(Qt::darkGreen)); // Set CPU Load graph color

    plot->addGraph();  // CPU Temp graph
    plot->graph(1)->setPen(QPen(Qt::darkRed)); // Set CPU Temp graph color
} else if (deviceType == DeviceType::RAM) {
    plot->addGraph();  // RAM Usage graph
    plot->graph(0)->setPen(QPen(Qt::blue)); // Set RAM Usage graph color
}

    // **Create a container widget for the plot and its legend**
    QWidget* plotContainer = new QWidget();
    QVBoxLayout* containerLayout = new QVBoxLayout(plotContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0); // Remove spacing
    containerLayout->setSpacing(6); // Add a bit of space between plot and legend

    // Add plot to the container layout
    containerLayout->addWidget(plot);

    // Create custom legend layout
    QHBoxLayout* legendLayout = new QHBoxLayout();

    // Set up legend items based on the device type
    if (deviceType == DeviceType::CPUThread) {
        QVector<QColor> cpuThreadColors = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta, Qt::yellow}; // Graph colors
        for (size_t i = 0; i < cpuThreads.size(); ++i) {
            // Create color square and label for each CPU thread
            ColorSquareWidget* colorSquare = new ColorSquareWidget(nullptr, cpuThreadColors[i % cpuThreadColors.size()]);
            legendLayout->addWidget(colorSquare);

            QString labelText = QString("Thread #%1").arg(i);  // Simplified label conversion
            QLabel* label = new QLabel(labelText);
            legendLayout->addWidget(label);
        }
    }
    else if (deviceType == DeviceType::CPU) {
        // Create color squares and labels for CPU Load and Temperature
        ColorSquareWidget* loadColorSquare = new ColorSquareWidget(nullptr, Qt::darkGreen);
        ColorSquareWidget* tempColorSquare = new ColorSquareWidget(nullptr, Qt::darkRed);
        legendLayout->addWidget(loadColorSquare);
        legendLayout->addWidget(new QLabel(QString::fromStdString(cpuDevice.name) + " Load"));
        legendLayout->addWidget(tempColorSquare);
        legendLayout->addWidget(new QLabel(QString::fromStdString(cpuDevice.name) + " Temperature"));
    }
    else if (deviceType == DeviceType::RAM) {
        // Create color square and label for RAM Load
        ColorSquareWidget* ramColorSquare = new ColorSquareWidget(nullptr, Qt::blue);
        legendLayout->addWidget(ramColorSquare);
        legendLayout->addWidget(new QLabel(QString::fromStdString(ramDevice.name) + " Load"));
    }

    // Add legend layout to the container layout
    containerLayout->addLayout(legendLayout);

    // Add the plot container to the right layout (instead of adding plot and legend separately)
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


