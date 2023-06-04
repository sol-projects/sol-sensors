#pragma once
#include "gui/AxisTag.hpp"
#include "gui/Settings.hpp"
#include "qcustomplot.h"
#include <QMainWindow>
#include <QWidget>
#include <QWidgetData>
#include <thread>
#include <vector>
#include "sensors/sensors.hpp"
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
    void on_actionSave_triggered();
    void timerSlot();
    void on_actionSettings_triggered();
    void on_actionPause_triggered();
    void on_actionResume_triggered();


private:
    Ui::MainWindow* ui;
    QWidget* widget;
    MyDialog* mDialog;
    std::vector<QCustomPlot*> mPlots;
    std::vector<QPointer<QCPGraph>> mGraphs;
    std::vector<AxisTag*> mTags;
    QTimer mDataTimer;
    QVector<bool> mPauseStatus;
    std::vector<sensors::Device> devices;
    int measurementTime = 100;

    void setupGraph(QCustomPlot* customPlot);
    QCustomPlot* createCustomPlot();

};

