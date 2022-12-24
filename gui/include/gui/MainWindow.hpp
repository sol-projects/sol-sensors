#pragma once
#include "gui/AxisTag.hpp"
#include "gui/Settings.hpp"
#include "qcustomplot.h"
#include <QMainWindow>
#include <QWidget>
#include <QWidgetData>

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

private:
    Ui::MainWindow* ui;
    QWidget* widget;
    MyDialog* mDialog;
    QCustomPlot* mPlot;
    QPointer<QCPGraph> mGraph1;
    QPointer<QCPGraph> mGraph2;
    AxisTag* mTag1;
    AxisTag* mTag2;
    QTimer mDataTimer;
};
