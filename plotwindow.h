#ifndef PLOTWINDOW_H
#define PLOTWINDOW_H

#include <QWidget>
#include "qcustomplot.h"

namespace Ui {
class PlotWindow;
}

class PlotWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PlotWindow(QWidget *parent = nullptr);
    ~PlotWindow();

public slots:
    void onPlotDataChanged(const QVector<double>& first, const QVector<double>& second);

signals:
    void plotReady();

private slots:
    void on_save_first_plot_clicked();

    void on_save_second_plot_clicked();

private:
    Ui::PlotWindow *ui;
    QCPBars *barsFirst = nullptr;
    QCPBars *barsSecond = nullptr;
};

#endif // PLOTWINDOW_H
