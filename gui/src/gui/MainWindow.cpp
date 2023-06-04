#include "gui/MainWindow.hpp"
#include "gui/ui_mainwindow.h"
#include "sensors/error.hpp"
#include "sensors/sensors.hpp"
#include <QLayout>
#include <fstream>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    devices = sensors::getDevices(sensors::Device::Type::Any);
    std::vector<bool> tmpLoad;
    for (const sensors::Device& device : devices)
    {
        auto temp = sensors::getTemp(device);
        auto load = sensors::getLoad(device);

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(150ms);
        load = sensors::getLoad(device);
        bool boolTmpLoad = 0;

        if (temp == sensors::error::code && load == sensors::error::code)
        {
            std::erase_if(devices, [&device](auto& device2) { return device.name == device2.name; });
        }
        else if (temp != sensors::error::code)
        {
            mPlots.push_back(nullptr);
            boolTmpLoad = true;
            tmpLoad.push_back(boolTmpLoad);
            mGraphs.emplace_back(nullptr);
            mTags.push_back(nullptr);
            mPauseStatus.push_back(false);
        }
        if (load != sensors::error::code)
        {
            mPlots.push_back(nullptr);
            boolTmpLoad = false;
            tmpLoad.push_back(boolTmpLoad);
            mGraphs.emplace_back(nullptr);
            mTags.push_back(nullptr);
            mPauseStatus.push_back(false);
        }
    }

    // layout()->addWidget(widget);
    ui->setupUi(this);
    auto* mainLayout = new QVBoxLayout(this);
    connect(ui->saveButton, &QAction::triggered, this, &MainWindow::on_actionSave_triggered);
    for (unsigned long i = 0; i < mPlots.size(); i++)
    {

        auto* nameOfGraph = new QLabel(this);
        if (tmpLoad[i])
        {
            nameOfGraph->setText("Temp of " + QString::fromStdString(devices[i].name));
        }
        else
        {
            nameOfGraph->setText("Load of " + QString::fromStdString(devices[i].name));
        }

        //nameOfGraph->setFixedHeight(30);
        mPlots[i] = new QCustomPlot(this);
        mPlots[i]->setMinimumHeight(200);
        setCentralWidget(mPlots[i]);

        mPlots[i]->yAxis->setTickLabels(false);
        connect(mPlots[i]->yAxis2, SIGNAL(rangeChanged(QCPRange)), mPlots[i]->yAxis, SLOT(setRange(QCPRange)));
        mPlots[i]->yAxis2->setVisible(true);
        mPlots[i]->axisRect()->axis(QCPAxis::atRight, 0)->setPadding(60);

        mGraphs[i] = mPlots[i]->addGraph(mPlots[i]->xAxis, mPlots[i]->axisRect()->axis(QCPAxis::atRight, 0));
        mGraphs[i]->setPen(QPen(QColor(250, 120, 0)));

        mTags[i] = new AxisTag(mGraphs[i]->valueAxis());
        mTags[i]->setPen(mGraphs[i]->pen());
        mainLayout->addWidget(nameOfGraph);

        auto* scrollBar = new QScrollBar(Qt::Horizontal, mPlots[i]);
        scrollBar->setFixedHeight(20);
        scrollBar->setRange(0, 100);
        connect(scrollBar, &QScrollBar::valueChanged, [this, i](int value) {
            double plotDataCount = mPlots[i]->graph(0)->dataCount();
            double visibleDataCount = plotDataCount * (value / 100.0);
            mPlots[i]->xAxis->setRange(plotDataCount - visibleDataCount, plotDataCount, Qt::AlignRight);
            mPlots[i]->replot();
        });

        mainLayout->addWidget(mPlots[i]);
        mainLayout->addWidget(scrollBar);
    }
    auto* centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    connect(&mDataTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
    mDataTimer.start(measurementTime);
}
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionSave_triggered()
{
    std::ofstream outputFile("save.txt");
    if (!outputFile.is_open())
    {
        return;
    }

    for (unsigned long i = 0; i < devices.size(); i++)
    {
            outputFile << "Device: " << devices[i].name << std::endl;

            if (mGraphs[i] != nullptr)
            {
                outputFile << "Graph Data:" << std::endl;
                for (int dataIndex = 0; dataIndex < mGraphs[i]->dataCount(); dataIndex++)
                {
                    double x = mGraphs[i]->dataMainKey(dataIndex);
                    double y = mGraphs[i]->dataMainValue(dataIndex);
                    outputFile << "X: " << x << ", Y: " << y << std::endl;
                }
            }

            outputFile << std::endl;
    }

    outputFile.close();
}


void MainWindow::on_actionSettings_triggered()
{
    mDialog = new MyDialog(this);
    mDialog->show();
}

void MainWindow::timerSlot()
{
    int precision = 2;

    for (unsigned long i = 0, j = 0; i < devices.size(); i++)
    {
        auto temp = sensors::getTemp(devices[i], precision);
        auto load = sensors::getLoad(devices[i], precision);
        if (!mPauseStatus[j])
        {
            if (temp != sensors::error::code && load != sensors::error::code)
            {
                double axisPoint = temp / std::pow(10, precision);
                mGraphs[j]->addData(mGraphs[j]->dataCount(), axisPoint);

                mPlots[j]->xAxis->rescale();
                mGraphs[j]->rescaleValueAxis(false, true);
                mPlots[j]->xAxis->setRange(mGraphs[j]->dataCount(), 100, Qt::AlignRight);

                double graph1Value = mGraphs[j]->dataMainValue(mGraphs[j]->dataCount() - 1);
                mTags[j]->updatePosition(graph1Value);
                mTags[j]->setText(QString::number(graph1Value, 'f', precision));

                mPlots[j]->replot();
                j++;

                axisPoint = load / std::pow(10, precision);
                mGraphs[j]->addData(mGraphs[j]->dataCount(), axisPoint);

                mPlots[j]->xAxis->rescale();
                mGraphs[j]->rescaleValueAxis(false, true);
                mPlots[j]->xAxis->setRange(mGraphs[j]->dataCount(), 100, Qt::AlignRight);

                graph1Value = mGraphs[j]->dataMainValue(mGraphs[j]->dataCount() - 1);
                mTags[j]->updatePosition(graph1Value);
                mTags[j]->setText(QString::number(graph1Value, 'f', precision));

                mPlots[j]->replot();
                j++;
            }
            else if (temp != sensors::error::code)
            {
                double axisPoint = temp / std::pow(10, precision);
                mGraphs[j]->addData(mGraphs[j]->dataCount(), axisPoint);

                mPlots[j]->xAxis->rescale();
                mGraphs[j]->rescaleValueAxis(false, true);
                mPlots[j]->xAxis->setRange(mGraphs[j]->dataCount(), 100, Qt::AlignRight);

                double graph1Value = mGraphs[j]->dataMainValue(mGraphs[j]->dataCount() - 1);
                mTags[j]->updatePosition(graph1Value);
                mTags[j]->setText(QString::number(graph1Value, 'f', precision));

                mPlots[j]->replot();
                j++;
            }
            else if (load != sensors::error::code)
            {
                double axisPoint = load / std::pow(10, precision);
                mGraphs[j]->addData(mGraphs[j]->dataCount(), axisPoint);

                mPlots[j]->xAxis->rescale();
                mGraphs[j]->rescaleValueAxis(false, true);
                mPlots[j]->xAxis->setRange(mGraphs[j]->dataCount(), 100, Qt::AlignRight);

                double graph1Value = mGraphs[j]->dataMainValue(mGraphs[j]->dataCount() - 1);
                mTags[j]->updatePosition(graph1Value);
                mTags[j]->setText(QString::number(graph1Value, 'f', precision));

                mPlots[j]->replot();
                j++;
            }
        }
    }
}

void MainWindow::on_actionPause_triggered()
{
    for (unsigned long i = 0; i < mPauseStatus.size(); i++)
    {
        mPauseStatus[i] = true;
    }
}

void MainWindow::on_actionResume_triggered()
{
    for (unsigned long i = 0; i < mPauseStatus.size(); i++)
    {
        mPauseStatus[i] = false;
    }
}

void MainWindow::setupGraph(QCustomPlot* customPlot)
{
    customPlot->setInteraction(QCP::iRangeDrag, true);
    customPlot->setInteraction(QCP::iRangeZoom, true);
    customPlot->axisRect()->setRangeDragAxes(customPlot->xAxis, nullptr);
    customPlot->axisRect()->setRangeZoomAxes(customPlot->xAxis, nullptr);
    customPlot->axisRect()->setRangeDrag(Qt::Horizontal);
    customPlot->axisRect()->setRangeZoom(Qt::Horizontal);
}
