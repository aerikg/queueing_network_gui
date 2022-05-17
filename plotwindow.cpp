#include "plotwindow.h"
#include "ui_plotwindow.h"
#include <QVector>
#include <algorithm>

PlotWindow::PlotWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlotWindow) {
    ui->setupUi(this);
    setWindowTitle("Графики доли времени в состояниях");
    QMargins *mapMargins = new QMargins(65, 25, 65, 45);
    QSharedPointer<QCPAxisTickerFixed> fixedTicker(new QCPAxisTickerFixed);
    fixedTicker->setTickStep(1);
    fixedTicker->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);

    barsFirst = new QCPBars(ui->plot_frame_first->xAxis, ui->plot_frame_first->yAxis);
    barsFirst->setAntialiased(false);
    barsFirst->setWidth(0.4);
    barsFirst->setBrush(QColor(25, 120, 180));
    barsSecond = new QCPBars(ui->plot_frame_second->xAxis, ui->plot_frame_second->yAxis);
    barsSecond->setAntialiased(false);
    barsSecond->setWidth(0.4);
    barsSecond->setBrush(QColor(25, 120, 180));

    QFont labelFont = QFont(font().family(), 10);
    QString xAxisLabel = "Количество заявок";
    QString yAxisLabel = "Доля времени";
    ui->plot_frame_first->xAxis->setTicker(fixedTicker);
    ui->plot_frame_first->xAxis->setLabelFont(labelFont);
    ui->plot_frame_first->xAxis->setLabel(xAxisLabel);
    ui->plot_frame_first->yAxis->setLabelFont(labelFont);
    ui->plot_frame_first->yAxis->setLabel(yAxisLabel);
    ui->plot_frame_second->xAxis->setTicker(fixedTicker);
    ui->plot_frame_second->xAxis->setLabelFont(labelFont);
    ui->plot_frame_second->xAxis->setLabel(xAxisLabel);
    ui->plot_frame_second->yAxis->setLabelFont(labelFont);
    ui->plot_frame_second->yAxis->setLabel(yAxisLabel);
}

PlotWindow::~PlotWindow() {
    ui->plot_frame_first->clearPlottables();
    ui->plot_frame_second->clearPlottables();
    delete ui;
    delete barsSecond;
}

void PlotWindow::onPlotDataChanged(const QVector<double>& first, const QVector<double>& second) {
    ui->plot_frame_first->axisRect()->setAutoMargins(QCP::msAll);
    ui->plot_frame_second->axisRect()->setAutoMargins(QCP::msAll);
    barsFirst->data()->clear();
    barsSecond->data()->clear();
    QVector<double> ticksFirst, ticksSecond;
    for (int i = 0; i < first.size(); ++i) {
        ticksFirst.push_back(i);
    }
    for (int i = 0; i < second.size(); ++i) {
        ticksSecond.push_back(i);
    }

    if (first.size() != 0) {
        barsFirst->setData(ticksFirst, first);
        double maxFirst = *std::max_element(first.constBegin(), first.constEnd());
        ui->plot_frame_first->xAxis->setRange(-0.99, ticksFirst.size() + 1);
        ui->plot_frame_first->yAxis->setRange(0, maxFirst + maxFirst / 20);
        ui->plot_frame_first->replot(); // для автоматического определения отступов
        double leftMargin = ui->plot_frame_first->axisRect()->margins().left();
        double bottomMargin = ui->plot_frame_first->axisRect()->margins().bottom();
        ui->plot_frame_first->axisRect()->setAutoMargins(QCP::msNone);
        ui->plot_frame_first->axisRect()->setMargins(QMargins(leftMargin, 25, leftMargin, bottomMargin));
    }
    if (second.size() != 0) {
        barsSecond->setData(ticksSecond, second);
        double maxSecond = *std::max_element(second.constBegin(), second.constEnd());
        ui->plot_frame_second->xAxis->setRange(-0.99, ticksSecond.size() + 1);
        ui->plot_frame_second->yAxis->setRange(0, maxSecond + maxSecond / 20);
        ui->plot_frame_second->replot(); // для автоматического определения отступов
        double leftMargin = ui->plot_frame_second->axisRect()->margins().left();
        double bottomMargin = ui->plot_frame_second->axisRect()->margins().bottom();
        ui->plot_frame_second->axisRect()->setAutoMargins(QCP::msNone);
        ui->plot_frame_second->axisRect()->setMargins(QMargins(leftMargin, 25, leftMargin, bottomMargin));
    }
    ui->plot_frame_first->replot();
    ui->plot_frame_second->replot();
    emit plotReady();
}

void PlotWindow::on_save_first_plot_clicked() {
    QString filename = QFileDialog::getSaveFileName(this, tr("Сохранить график"),
                                                    "plot1.png", tr("Images (*.png)"));
    if (filename != "") {
        ui->plot_frame_first->savePng(filename);
    }
}

void PlotWindow::on_save_second_plot_clicked() {
    QString filename = QFileDialog::getSaveFileName(this, tr("Сохранить график"),
                                                    "plot2.png", tr("Images (*.png)"));
    if (filename != "") {
        ui->plot_frame_second->savePng(filename);
    }
}
