#include "gui/Settings.hpp"
#include "gui/ui_settings.h"

MyDialog::MyDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::MyDialog)
{
    ui->setupUi(this);
}

MyDialog::~MyDialog()
{
    delete ui;
}

void MyDialog::on_cpuTemp_stateChanged(int arg1)
{
}

void MyDialog::on_cpuUsage_stateChanged(int arg1)
{
}

void MyDialog::on_gpuTemp_stateChanged(int arg1)
{
}

void MyDialog::on_gpuUsage_stateChanged(int arg1)
{
}

void MyDialog::on_spinBox_valueChanged(int arg1)
{
}
