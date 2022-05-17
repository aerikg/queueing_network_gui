#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include "model.h"
#include "plotwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_distribution_selection_currentTextChanged(const QString &arg1);

    void on_show_plot_clicked();

    void on_clear_output_clicked();

    void onOutputTextChange(const QString& text);

    void onProcessInfoChange(const QString& text);

    void onIterationNumberChange();

    void onProgressChange(int value);

    void enableInput();

    void onPlotReady();

signals:
    void stopSimulation();
    void iterationNumberChanged();
    void outputTextChanged(const QString& text);
    void setUpPlotData(const QVector<double>& first, const QVector<double>& second);

protected:
    void closeEvent (QCloseEvent *event) override;

private:
    int iteration_number, iteration_count;
    void disableInput();
    void startSimulation();
    bool checkInputs();
    Ui::MainWindow *ui;
    Model* model = nullptr;
    PlotWindow* plotWindow = nullptr;
};
#endif // MAINWINDOW_H
