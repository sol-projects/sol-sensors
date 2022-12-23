#ifndef SETTINGS_H
#define SETTINGS_H
#include <QDialog>

namespace Ui
{
    class MyDialog;
}

class MyDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MyDialog(QWidget* parent = 0);
    virtual ~MyDialog();

private slots:
    void on_cpuTemp_stateChanged(int arg1);

    void on_cpuUsage_stateChanged(int arg1);

    void on_gpuTemp_stateChanged(int arg1);

    void on_gpuUsage_stateChanged(int arg1);

    void on_spinBox_valueChanged(int arg1);

private:
    Ui::MyDialog* ui;
};
#endif // SETTINGS_H
