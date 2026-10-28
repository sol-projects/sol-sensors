#include "gui/MainWindow.hpp"
#include "sensors/error.hpp"
#include "ui_mainwindow.h"
#include <QCheckBox>
#include <QFontDatabase>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStatusBar>
#include <QSysInfo>
#include <cmath>
#include <limits>

// Hallmark: Rim, native Qt app screen. Colours and fonts are defined in this theme block.
namespace
{
    namespace theme
    {
        const QColor paper("#151719");
        const QColor panel("#1d2023");
        const QColor raised("#282d31");
        const QColor ink("#ece9e3");
        const QColor muted("#aaa9a5");
        const QColor rule("#383d40");
        const QColor accent("#f6a064");
        const QColor cool("#81b7dc");
        const QVector<QColor> threads { cool, accent, QColor("#9bc8ad"), QColor("#d4b8d5"), QColor("#c8c18b"), QColor("#b8c4cc") };
        QFont body() { return QApplication::font(); }
        QFont numbers() { return QFontDatabase::systemFont(QFontDatabase::FixedFont); }
    }

    QString measurement(int value, const QString& unit)
    {
        return value == sensors::error::code ? QStringLiteral("Unavailable") : QString::number(value / 100.0, 'f', 1) + " " + unit;
    }
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->menubar->hide();
    setWindowTitle("SOL Sensors");
    resize(1240, 850);
    setMinimumSize(540, 420);
    QFont font = theme::body();
    font.setPointSize(11);
    setFont(font);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, theme::paper);
    palette.setColor(QPalette::WindowText, theme::ink);
    palette.setColor(QPalette::Base, theme::panel);
    palette.setColor(QPalette::Text, theme::ink);
    palette.setColor(QPalette::Button, theme::raised);
    palette.setColor(QPalette::ButtonText, theme::ink);
    palette.setColor(QPalette::Highlight, theme::accent);
    palette.setColor(QPalette::HighlightedText, theme::paper);
    setPalette(palette);
    setStyleSheet(QString(
        "QMainWindow, QWidget#centralwidget { background: %1; color: %2; }"
        "QWidget { color: %2; }"
        "QWidget#plotPanel { background: %3; border: 1px solid %4; }"
        "QLabel { background: transparent; border: none; }"
        "QLabel[role=muted], QStatusBar { color: %5; }"
        "QLabel[role=title] { font-size: 18px; font-weight: 500; }"
        "QLabel#brand { font-size: 24px; font-weight: 500; }"
        "QCheckBox { spacing: 8px; padding: 8px 10px; min-height: 28px; border: 1px solid transparent; }"
        "QCheckBox:hover { background: %6; }"
        "QCheckBox:focus { border: 1px solid %7; }"
        "QCheckBox:disabled { color: %5; }"
        "QScrollArea { border: none; background: %1; }"
        "QScrollBar:vertical { width: 10px; background: %1; }"
        "QScrollBar::handle:vertical { background: %4; min-height: 32px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QStatusBar { background: %1; border-top: 1px solid %4; padding: 4px; }")
            .arg(theme::paper.name(), theme::ink.name(), theme::panel.name(), theme::rule.name(), theme::muted.name(), theme::raised.name(), theme::accent.name()));

    auto layout = new QVBoxLayout(ui->centralwidget);
    layout->setContentsMargins(28, 24, 28, 12);
    layout->setSpacing(16);
    auto heading = new QHBoxLayout;
    auto brand = new QLabel("SOL Sensors");
    brand->setObjectName("brand");
    heading->addWidget(brand);
    heading->addStretch();
    auto platform = new QLabel(QSysInfo::productType());
    platform->setProperty("role", "muted");
    heading->addWidget(platform);
    layout->addLayout(heading);

    auto controls = new QHBoxLayout;
    controls->setSpacing(4);
    layout->addLayout(controls);
    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto content = new QWidget;
    content->setObjectName("centralwidget");
    content->setMaximumWidth(1500);
    plotLayout = new QGridLayout(content);
    plotLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    plotLayout->setContentsMargins(0, 0, 0, 0);
    plotLayout->setSpacing(16);
    scroll->setWidget(content);
    scroll->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    layout->addWidget(scroll);

    auto cpus = sensors::getDevices(DeviceType::CPU);
    auto ram = sensors::getDevices(DeviceType::RAM);
    auto gpus = sensors::getDevices(DeviceType::GPU);
    auto vram = sensors::getDevices(DeviceType::VRAM);
    auto threads = sensors::getDevices(DeviceType::CPUThread);
    for (const auto& cpu : cpus)
    {
        createPlot({ cpu }, "Processor");
    }
    for (const auto& memory : ram)
    {
        createPlot({ memory }, "Memory");
    }
    for (std::size_t i = 0; i < gpus.size(); i++)
    {
        createPlot({ gpus[i] }, "Graphics");
        if (i < vram.size())
        {
            createPlot({ vram[i] }, "Video memory");
        }
    }
    if (!threads.empty())
    {
        createPlot(threads, "CPU threads");
    }
    emptyState = new QLabel(plots.empty() ? "No hardware sensors found on this system." : "Select a graph above to display its measurements.");
    emptyState->setWordWrap(true);
    emptyState->setProperty("role", "muted");
    emptyState->setAlignment(Qt::AlignCenter);
    plotLayout->addWidget(emptyState, 0, 0);
    for (const auto& [type, title] : std::vector<std::pair<DeviceType, QString>> {
             { DeviceType::CPU, "Processor" }, { DeviceType::RAM, "Memory" },
             { DeviceType::GPU, "Graphics" }, { DeviceType::CPUThread, "Threads" } })
    {
        auto checkbox = new QCheckBox(title);
        checkbox->setObjectName("show" + QString::number(static_cast<int>(type)));
        bool available = std::any_of(plots.begin(), plots.end(), [type](const Plot& plot) { return plot.type == type; });
        checkbox->setEnabled(available);
        checkbox->setChecked(available);
        checkbox->setToolTip(available ? "Show or hide " + title.toLower() + " graphs" : "No sensors detected");
        controls->addWidget(checkbox);
        connect(checkbox, &QCheckBox::toggled, this, [this, type](bool checked) {
            for (auto& plot : plots)
            {
                if (plot.type == type || (type == DeviceType::GPU && plot.type == DeviceType::VRAM))
                {
                    plot.container->setHidden(!checked);
                }
            }
            arrangePlots();
        });
    }
    controls->addStretch();
    statusBar()->showMessage("Refresh: 300 ms    History: 60 seconds    Unavailable: not reported by the sensor");
    elapsed.start();
    arrangePlots();
    connect(&updateTimer, &QTimer::timeout, this, &MainWindow::update);
    updateTimer.start(300);
}

void MainWindow::createPlot(const std::vector<sensors::Device>& devices, const QString& title)
{
    auto type = devices.front().type;
    bool memory = type == DeviceType::RAM || type == DeviceType::VRAM;
    bool temperature = type == DeviceType::CPU || type == DeviceType::GPU;
    auto container = new QWidget;
    container->setObjectName("plotPanel");
    auto layout = new QVBoxLayout(container);
    layout->setContentsMargins(20, 16, 20, 12);
    layout->setSpacing(6);
    auto header = new QHBoxLayout;
    auto label = new QLabel(title);
    label->setProperty("role", "title");
    header->addWidget(label);
    header->addStretch();
    auto temp = new QLabel(temperature ? "Temperature: waiting" : "");
    temp->setProperty("role", "muted");
    header->addWidget(temp);
    layout->addLayout(header);
    auto subtitle = new QLabel(type == DeviceType::CPUThread ? QString("%1 logical processors").arg(devices.size()) : QString::fromStdString(devices.front().name));
    subtitle->setWordWrap(true);
    subtitle->setProperty("role", "muted");
    layout->addWidget(subtitle);
    auto value = new QLabel("Waiting for measurement");
    QFont numberFont = theme::numbers();
    numberFont.setPointSize(23);
    value->setFont(numberFont);
    layout->addWidget(value);

    auto plot = new QCustomPlot;
    plot->setMinimumHeight(145);
    plot->setMinimumWidth(0);
    plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    plot->setFixedHeight(156);
    plot->setBackground(theme::panel);
    plot->axisRect()->setBackground(theme::panel);
    for (auto axis : { plot->xAxis, plot->yAxis, plot->xAxis2, plot->yAxis2 })
    {
        axis->setBasePen(QPen(theme::rule));
        axis->setTickPen(QPen(theme::rule));
        axis->setSubTicks(false);
        axis->setTickLabelColor(theme::muted);
        axis->setLabelColor(theme::muted);
        axis->setTickLabelFont(theme::body());
        axis->setLabelFont(theme::body());
        axis->grid()->setPen(QPen(theme::rule, 1, Qt::DotLine));
        axis->grid()->setZeroLinePen(QPen(theme::rule));
    }
    plot->xAxis->grid()->setVisible(false);
    plot->xAxis->setRange(-60, 0);
    plot->xAxis->setLabel("Seconds");
    plot->yAxis->setRange(0, memory ? 1 : 100);
    plot->yAxis->setLabel(memory ? "GB" : "%");
    plot->yAxis2->setVisible(temperature);
    plot->yAxis2->setRange(0, 120);
    plot->yAxis2->setLabel(QString::fromUtf8("°C"));
    plot->yAxis2->grid()->setVisible(false);
    for (std::size_t i = 0; i < devices.size(); i++)
    {
        auto graph = plot->addGraph();
        graph->setPen(QPen(type == DeviceType::CPUThread ? theme::threads[i % theme::threads.size()] : theme::cool, 1.8));
        graph->setName(QString::fromStdString(devices[i].name));
        if (type != DeviceType::CPUThread)
        {
            QColor fill = theme::cool;
            fill.setAlpha(18);
            graph->setBrush(fill);
        }
    }
    if (temperature)
    {
        plot->addGraph(plot->xAxis, plot->yAxis2)->setPen(QPen(theme::accent, 1.5));
    }
    layout->addWidget(plot);
    auto legend = new QLabel;
    legend->setProperty("role", "muted");
    legend->setWordWrap(true);
    if (temperature)
    {
        legend->setText(QString("<span style='color:%1'>Usage</span> / <span style='color:%2'>Temperature</span>").arg(theme::cool.name(), theme::accent.name()));
    }
    else if (type == DeviceType::CPUThread)
    {
        QStringList labels;
        for (std::size_t i = 0; i < devices.size(); i++)
        {
            labels << QString("<span style='color:%1'>Thread #%2</span>").arg(theme::threads[i % theme::threads.size()].name()).arg(i);
        }
        legend->setText(labels.join(" &nbsp; "));
    }
    else
    {
        legend->setText("Used memory");
    }
    layout->addWidget(legend);
    plots.push_back({ container, plot, value, temp, devices, type });
    plotLayout->addWidget(container);
}

void MainWindow::arrangePlots()
{
    int count = 0;
    columns = width() < 950 ? 1 : 2;
    for (auto& plot : plots)
    {
        plotLayout->removeWidget(plot.container);
    }
    plotLayout->removeWidget(emptyState);
    for (auto& plot : plots)
    {
        if (!plot.container->isHidden())
        {
            if (plot.type == DeviceType::CPUThread)
            {
                plotLayout->addWidget(plot.container, (count + columns - 1) / columns, 0, 1, columns);
            }
            else
            {
                plotLayout->addWidget(plot.container, count / columns, count % columns);
            }
            count++;
        }
    }
    emptyState->setVisible(count == 0);
    if (count == 0)
    {
        plotLayout->addWidget(emptyState, 0, 0, 1, columns);
    }
    plotLayout->setColumnStretch(0, 1);
    plotLayout->setColumnStretch(1, columns == 2 ? 1 : 0);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (columns && columns != (width() < 950 ? 1 : 2))
    {
        arrangePlots();
    }
}

void MainWindow::update()
{
    double time = elapsed.elapsed() / 1000.0;
    for (auto& plot : plots)
    {
        bool memory = plot.type == DeviceType::RAM || plot.type == DeviceType::VRAM;
        double total = 0;
        int measured = 0;
        for (std::size_t i = 0; i < plot.devices.size(); i++)
        {
            int load = sensors::getLoad(plot.devices[i], 2);
            double value = load == sensors::error::code ? std::numeric_limits<double>::quiet_NaN() : load / 100.0;
            plot.graph->graph(static_cast<int>(i))->addData(time, value);
            if (load != sensors::error::code)
            {
                total += load;
                measured++;
                if (memory && value > plot.graph->yAxis->range().upper * 0.85)
                {
                    plot.graph->yAxis->setRange(0, value * 1.3);
                }
            }
        }
        plot.value->setText(measurement(measured ? static_cast<int>(total / measured) : sensors::error::code, memory ? "GB" : "%"));
        if (plot.type == DeviceType::CPU || plot.type == DeviceType::GPU)
        {
            int temp = sensors::getTemp(plot.devices.front(), 2);
            plot.temperature->setText("Temperature: " + measurement(temp, QString::fromUtf8("°C")));
            plot.graph->graph(1)->addData(time, temp == sensors::error::code ? std::numeric_limits<double>::quiet_NaN() : temp / 100.0);
        }
        else if (plot.type == DeviceType::VRAM)
        {
            int temp = sensors::getTemp(plot.devices.front(), 2);
            plot.temperature->setText("Temperature: " + measurement(temp, QString::fromUtf8("°C")));
        }
        for (int i = 0; i < plot.graph->graphCount(); i++)
        {
            plot.graph->graph(i)->data()->removeBefore(time - 60);
        }
        plot.graph->xAxis->setRange(std::max(0.0, time - 60), std::max(1.0, time));
        if (plot.container->isVisible())
        {
            plot.graph->replot();
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
