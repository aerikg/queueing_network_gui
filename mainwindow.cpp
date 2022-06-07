#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QIntValidator>
#include <QSizePolicy>
#include <QMessageBox>
#include <QVector>
#include <QString>
#include <chrono>
#include <sstream>
#include "model.h"
#include "plotwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setWindowTitle("Симуляция сети массового обслуживания");
    QSizePolicy sp = ui->progressBar->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    ui->progressBar->setSizePolicy(sp);
    ui->progressBar->hide();
    QRegularExpression rxDouble;
    rxDouble.setPattern("0|(0[.,][0-9]{0,16})|([1-9][0-9]{0,18}[.,][0-9]{0,16})");
    QRegularExpressionValidator* rxDoubleValidator = new QRegularExpressionValidator(rxDouble, this);
    ui->lambda1_input->setValidator(rxDoubleValidator);
    ui->lambda2_input->setValidator(rxDoubleValidator);
    ui->mu1_input->setValidator(rxDoubleValidator);
    ui->mu2_input->setValidator(rxDoubleValidator);
    ui->gamma1_input->setValidator(rxDoubleValidator);
    ui->gamma2_input->setValidator(rxDoubleValidator);

    QRegularExpression rxProbability;
    rxProbability.setPattern("0|(0[.,][0-9]{0,16})");   // разрешение на ввод вещественных чисел [0, 1)
    QRegularExpressionValidator* rxProbabilityValidator = new QRegularExpressionValidator(rxProbability, this);
    ui->alpha1_input->setValidator(rxProbabilityValidator);
    ui->alpha2_input->setValidator(rxProbabilityValidator);

    QRegularExpression rxIterationCount;
    rxIterationCount.setPattern("[1-9][0-9]{0,8}");
    QRegularExpressionValidator* rxIterationCountValidator = new QRegularExpressionValidator(rxIterationCount, this);
    ui->iteration_count->setValidator(rxIterationCountValidator);

    QRegularExpression rxArrivingJobCount;
    rxArrivingJobCount.setPattern("[1-9][0-9]{0,18}");
    QRegularExpressionValidator* rxArrivingJobCountValidator = new QRegularExpressionValidator(rxArrivingJobCount, this);
    ui->arriving_job_count->setValidator(rxArrivingJobCountValidator);

    plotWindow = new PlotWindow();
    connect(this, SIGNAL(outputTextChanged(const QString&)), this, SLOT(onOutputTextChange(const QString&)));
    connect(plotWindow, SIGNAL(plotReady()), this, SLOT(onPlotReady()));
    connect(this, SIGNAL(setUpPlotData(const QVector<double>&, const QVector<double>&)),
            plotWindow, SLOT(onPlotDataChanged(const QVector<double>&, const QVector<double>&)));
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::on_pushButton_clicked() {
    if (ui->pushButton->text() == "Старт") {
        if (checkInputs()) {
            disableInput();
            ui->progressBar->show();
            // запуск симуляции в отдельном потоке
            QFuture<void> result = QtConcurrent::run(startSimulation, this);
            QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
            connect(watcher, SIGNAL(finished()), this, SLOT(enableInput()));
            connect(watcher, SIGNAL(finished()), watcher, SLOT(deleteLater()));
            watcher->setFuture(result);
        }
    }
    else if (model != nullptr) {
        model->stop();
    }
}

void MainWindow::on_distribution_selection_currentTextChanged(const QString &arg1) {
    if (arg1 == "Экспоненциальное") {
        ui->label_gamma1->setDisabled(true);
        ui->label_gamma2->setDisabled(true);
        ui->gamma1_input->setDisabled(true);
        ui->gamma2_input->setDisabled(true);
        ui->gamma1_input->setText("1");
        ui->gamma2_input->setText("1");
        ui->gamma1->setStyleSheet("color: #787878;");
        ui->gamma2->setStyleSheet("color: #787878;");
    }
    else {
        ui->label_gamma1->setEnabled(true);
        ui->label_gamma2->setEnabled(true);
        ui->gamma1_input->setEnabled(true);
        ui->gamma2_input->setEnabled(true);
        ui->gamma1->setStyleSheet("color: black;");
        ui->gamma2->setStyleSheet("color: black;");
    }
}

void MainWindow::disableInput() {
    ui->pushButton->setText("Стоп");
    ui->progressBar->setValue(0);
    ui->show_plot->setDisabled(true);
    ui->checkBox->setDisabled(true);
    ui->lambda1_input->setDisabled(true);
    ui->lambda2_input->setDisabled(true);
    ui->mu1_input->setDisabled(true);
    ui->mu2_input->setDisabled(true);
    ui->alpha1_input->setDisabled(true);
    ui->alpha2_input->setDisabled(true);
    ui->gamma1_input->setDisabled(true);
    ui->gamma2_input->setDisabled(true);
    ui->arriving_job_count->setDisabled(true);
    ui->iteration_count->setDisabled(true);
    ui->distribution_selection->setDisabled(true);
}

void MainWindow::enableInput() {
    ui->iteration_info->setText("");
    ui->process_info->setText("");
    ui->progressBar->hide();
    ui->pushButton->setText("Старт");
    ui->checkBox->setEnabled(true);
    ui->lambda1_input->setEnabled(true);
    ui->lambda2_input->setEnabled(true);
    ui->mu1_input->setEnabled(true);
    ui->mu2_input->setEnabled(true);
    ui->alpha1_input->setEnabled(true);
    ui->alpha2_input->setEnabled(true);
    if (ui->distribution_selection->currentText() != "Экспоненциальное") {
        ui->gamma1_input->setEnabled(true);
        ui->gamma2_input->setEnabled(true);
    }
    ui->arriving_job_count->setEnabled(true);
    ui->iteration_count->setEnabled(true);
    ui->distribution_selection->setEnabled(true);
}

void MainWindow::startSimulation() {
    // считываем данные из полей
    double lambda1 = ui->lambda1_input->text().replace(',', ".").toDouble();
    double lambda2 = ui->lambda2_input->text().replace(',', ".").toDouble();
    double mu1 = ui->mu1_input->text().replace(',', ".").toDouble();
    double mu2 = ui->mu2_input->text().replace(',', ".").toDouble();
    double alpha1 = ui->alpha1_input->text().replace(',', ".").toDouble();
    double alpha2 = ui->alpha2_input->text().replace(',', ".").toDouble();
    double gamma1 = ui->gamma1_input->text().replace(',', ".").toDouble();
    double gamma2 = ui->gamma2_input->text().replace(',', ".").toDouble();
    quint64 arriving_job_count = ui->arriving_job_count->text().toULongLong();
    iteration_count = ui->iteration_count->text().toUInt();
    ProcessDistr err_distr;
    bool output_every_iteration = ui->checkBox->isChecked();

    if (ui->distribution_selection->currentText() == "Экспоненциальное") {
        err_distr = ProcessDistr::exp_plain;
    }
    else if (ui->distribution_selection->currentText() == "Экспоненциальное масштабированное") {
        err_distr = ProcessDistr::exp_scaled;
    }
    else {
        err_distr = ProcessDistr::uniform;
    }

    // подготавливаем текст про входные данные системы
    std::stringstream system_info_text;
    system_info_text << "Параметры системы: " << "\n";
    system_info_text << "λ1: " << lambda1 << '\t' << "λ2: " << lambda2 << "\n";
    system_info_text << "μ1: " << mu1 << '\t' << "μ2: " << mu2 << "\n";
    system_info_text << "α1: " << alpha1 << '\t' << "α2: " << alpha2 << "\n";
    if (err_distr != ProcessDistr::exp_plain) {
        system_info_text << "γ1: " << gamma1 << '\t' << "γ2: " << gamma2 << "\n";
    }

    system_info_text << "\nРаспределение обслуживания ошибочных заявок:\n";
    if (err_distr == ProcessDistr::exp_scaled) {
        system_info_text << "экспоненциальное с параметрами " << mu1 * gamma1
            << " и " << mu2 * gamma2 << "\n";
    }
    else if (err_distr == ProcessDistr::uniform) {
        system_info_text << "равномерное [0, " << 2 / (mu1 * gamma1)
            << "] и [0, " << 2 / (mu2 * gamma2) << "]\n";
    }
    else {
        system_info_text << "экспоненциальное с параметрами " << mu1 << " и " << mu2 << "\n";
    }
    system_info_text << "\nКоличество заявок на вход системы: " << arriving_job_count;
    system_info_text << "\nКоличество итераций: " << iteration_count;
    emit outputTextChanged(QString::fromStdString(system_info_text.str()));

    std::vector<double> combined_system_time_stats(2, 0);
    std::vector<double> combined_final_service_waiting_time_stats(2, 0);
    std::vector<double> combined_node_time_spent_times(2, 0);
    std::vector<double> combined_queue_lengths(2, 0);
    std::vector<double> combined_queue_waiting_times(2, 0);
    std::vector<double> combined_idle_times(2, 0);
    std::vector<double> combined_job_count(2, 0);
    std::vector<std::vector<double>> combined_job_count_durations(2);

    model = new Model(
                2,								// количество типов заявок
                2,								// количество обрабатывающих узлов
                { lambda1, lambda2 },			// интенсивности прибытия заявок
                { mu1, mu2 },					// параметры экспоненциального распределения для облуживания заявок
                { alpha1, alpha2 },				// ошибка классификации
                err_distr,						// распределение обработки ошибочных заявок
                { gamma1, gamma2 }				// значения, используемые для распределения обработки ошибочных заявках
            );

    // подключаем сигналы
    connect(model, &Model::progressChanged, this, onProgressChange, Qt::BlockingQueuedConnection);
    connect(model, &Model::processInfoChanged, this, onProcessInfoChange);
    connect(this, iterationNumberChanged, this, onIterationNumberChange);

    bool simulation_check;
    auto simulation_start = std::chrono::steady_clock::now();
    iteration_number = 0;
    for (; iteration_number < iteration_count; ++iteration_number) {
        emit iterationNumberChanged();
        auto iteration_start = std::chrono::steady_clock::now();
        simulation_check = model->StartSimulation(arriving_job_count);
        auto iteration_finish = std::chrono::steady_clock::now();

        if (!simulation_check) {
            break;
        }
        if (output_every_iteration) {
            QString str = "----------------------------------------------------------------------";
            str += "Результаты итерации #" + QString::number(iteration_number + 1) + ":\n";
            emit outputTextChanged(str);
        }
        // получаем результаты симуляции
        std::vector<std::vector<double>> system_time_stats = model->GetSystemTimeStats();
        std::vector<std::vector<double>> final_service_waiting_time_stats = model->GetFinalServiceWaitingTimeStats();
        std::vector<std::vector<double>> node_time_spent_stats = model->GetNodeTimeSpentStats();
        std::vector<std::vector<double>> job_count_durations;
        std::vector<Node> nodes = model->GetNodes();

        for (int i = 0; i < nodes.size(); ++i) {
            job_count_durations.push_back(nodes[i].GetJobCountDuration());
        }

        std::stringstream results_text; // поток для вывода результатов
        for (int i = 0; i < system_time_stats.size(); ++i) {
            double average_system_time = std::accumulate(begin(system_time_stats[i]), end(system_time_stats[i]), 0.0)
                                         / system_time_stats[i].size();
            double average_final_service_waiting_time = std::accumulate(begin(final_service_waiting_time_stats[i]), end(final_service_waiting_time_stats[i]), 0.0)
                                                        / final_service_waiting_time_stats[i].size();
            combined_system_time_stats[i] += average_system_time;
            combined_final_service_waiting_time_stats[i] += average_final_service_waiting_time;
            results_text << "Среднее время пребывания заявок " << i+1 <<  "-го типа: " << average_system_time << "\n";
            results_text << "Среднее время ожидания верной обработки заявок " << i+1 <<  "-го типа: " << average_final_service_waiting_time << "\n";
        }

        for (int i = 0; i < nodes.size(); ++i) {
            results_text << "\n" << "Узел #" << i+1 << ":" << "\n";
            double idle_time = nodes[i].GetIdleTime();
            double total_system_time = nodes[i].GetTotalSystemTime();
            std::vector<double> queue_waiting_times = nodes[i].GetQueueWaitingTimes();
            std::vector<double> job_count_duration = job_count_durations[i];

            double avg_node_time_spent = std::accumulate(begin(node_time_spent_stats[i]), end(node_time_spent_stats[i]), 0.0)
                                        / node_time_spent_stats[i].size();
            double avg_queue_time_spent = std::accumulate(begin(queue_waiting_times), end(queue_waiting_times), 0.0)
                                        / queue_waiting_times.size();
            double avg_idle_time = idle_time / total_system_time;
            double average_job_count = 0;
            double avg_queue_length = 0;
            for (int j = 0; j < job_count_duration.size(); ++j) {
                job_count_duration[j] /= total_system_time;
                if (j > 1) {
                    avg_queue_length += job_count_duration[j] * (j-1);
                }
                average_job_count += job_count_duration[j] * j;

                if (combined_job_count_durations[i].size() < j + 1) {
                    combined_job_count_durations[i].push_back(job_count_duration[j]);
                }
                else {
                    combined_job_count_durations[i][j] += job_count_duration[j];
                }
            }
            results_text << "Средняя доля времени в простое: " << avg_idle_time << "\n";
            results_text << "Средняя длина очереди: " << avg_queue_length << "\n";
            results_text << "Среднее время ожидания в очереди: " << avg_queue_time_spent << "\n";
//            results_text << "Среднее время, проведенное в узле: " << avg_node_time_spent << "\n";
            results_text << "Среднее количество заявок в узле: " << average_job_count << "\n";

            combined_node_time_spent_times[i] += avg_node_time_spent;
            combined_queue_lengths[i] += avg_queue_length;
            combined_queue_waiting_times[i] += avg_queue_time_spent;
            combined_idle_times[i] += avg_idle_time;
            combined_job_count[i] += average_job_count;
        }
        results_text << "\nДлительность " << std::to_string(iteration_number+1) << "-й итерации: " <<
                       std::chrono::duration_cast<std::chrono::milliseconds>(iteration_finish-iteration_start).count()
                       << " мс";
        if (output_every_iteration) {
            emit outputTextChanged(QString::fromStdString(results_text.str()));
        }
        model->reset();
    }

    if (!simulation_check) {
        QString simulation_stop_text = "";
        simulation_stop_text += "=======================================\n";
        simulation_stop_text += "Симуляция была прервана во время " + QString::number(iteration_number + 1) + "-й итерации";
        if (iteration_number == 0) {
            simulation_stop_text += "\n=======================================\n";
        }
        emit outputTextChanged(simulation_stop_text);
    }

    if (simulation_check || iteration_number > 0) {
        auto simulation_finish = std::chrono::steady_clock::now();
        std::stringstream simulation_results_text;
        if (simulation_check) {
            simulation_results_text << "=======================================\n";
        }
        simulation_results_text << "Результаты симуляции:\n\n";

        for (int i = 0; i < combined_system_time_stats.size(); ++i) {
            simulation_results_text << "Среднее время пребывания заявок " << i+1 << "-го типа: " <<
                                       combined_system_time_stats[i] / iteration_number << "\n";
            simulation_results_text << "Среднее время ожидания верной обработки заявок " << i+1 << "-го типа: " <<
                                       combined_final_service_waiting_time_stats[i] / iteration_number << "\n";
        }

        for (int i = 0; i < combined_node_time_spent_times.size(); ++i) {
            simulation_results_text << "\nУзел #" << i+1 << ":\n";
            simulation_results_text << "Средняя доля времени в простое: " << combined_idle_times[i] / iteration_number<< "\n";
            simulation_results_text << "Средняя длина очереди: " << combined_queue_lengths[i] / iteration_number << "\n";
            simulation_results_text << "Среднее время ожидания в очереди: " << combined_queue_waiting_times[i] / iteration_number << "\n";
//            simulation_results_text << "Среднее время, проведенное в узле: " << combined_node_time_spent_times[i] / iteration_number << "\n";
        }

        simulation_results_text << "\nДлительность симуляции: " <<
              std::chrono::duration_cast<std::chrono::milliseconds>(simulation_finish-simulation_start).count() << " мс" <<
              "\n"; // << "----------------------------------------------------------------------" << "\n" << "Конец симуляции";
        simulation_results_text << "=======================================\n";

        for (int i = 0; i <  combined_job_count_durations.size(); ++i) {
            for (int j = 0; j < combined_job_count_durations[i].size(); ++j) {
                combined_job_count_durations[i][j] /= iteration_number;
            }
        }
        emit outputTextChanged(QString::fromStdString(simulation_results_text.str()));

        QVector<double> first = QVector<double>(combined_job_count_durations[0].begin(), combined_job_count_durations[0].end());
        QVector<double> second = QVector<double>(combined_job_count_durations[1].begin(), combined_job_count_durations[1].end());
        emit setUpPlotData(first, second);
    }
    if (model != nullptr) {
        delete model;
        model = nullptr;
    }
}

bool MainWindow::checkInputs() {
    double lambda1 = ui->lambda1_input->text().replace(',', ".").toDouble();
    double lambda2 = ui->lambda2_input->text().replace(',', ".").toDouble();
    double mu1 = ui->mu1_input->text().replace(',', ".").toDouble();
    double mu2 = ui->mu2_input->text().replace(',', ".").toDouble();
    double gamma1 = ui->gamma1_input->text().replace(',', ".").toDouble();
    double gamma2 = ui->gamma2_input->text().replace(',', ".").toDouble();
    uint iteration_count = ui->iteration_count->text().toUInt();
    quint64 arriving_job_count = ui->arriving_job_count->text().toULongLong();

    QString errorText = "";
    if (lambda1 == 0 && lambda2 == 0) {
        errorText += "\n-хотя бы одно из значений λi не должно быть равно нулю";
    }
    if (mu1 == 0) {
        errorText += "\n-значение μ1 не должно быть равно нулю";
    }
    if (mu2 == 0) {
        errorText += "\n-значение μ2 не должно быть равно нулю";
    }
    if (gamma1 == 0) {
        errorText += "\n-значение γ1 не должно быть равно нулю";
    }
    if (gamma2 == 0) {
        errorText += "\n-значение γ2 не должно быть равно нулю";
    }
    if (iteration_count == 0) {
        errorText += "\n-количество итераций должно быть указано";
    }
    if (arriving_job_count == 0) {
        errorText += "\n-количество прибывающих заявок должно быть указано";
    }

    if (errorText != "") {
        QMessageBox::warning(this, "Ошибочные данные", "Среди введенных данных были найдены следующие ошибки:"
                             + errorText + "\n\nИсправьте значения и попробуйте снова.", QMessageBox::StandardButton::Ok);
        return false;
    }
    else {
        bool check_condition = true;
        QString stability_condition = "Предупреждение:";
        double alpha1 = ui->alpha1_input->text().replace(',', ".").toDouble();
        double alpha2 = ui->alpha2_input->text().replace(',', ".").toDouble();
        if (lambda1/mu1 + lambda2*alpha2 / (1 - alpha2) / (gamma1 * mu1) >= 1) {
            check_condition = false;
            stability_condition += "\nУсловие устойчивости для первого узла может не выполняться.";
        }

        if (lambda2/mu2 + lambda1*alpha1 / (1 - alpha1) / (gamma2 * mu2) >= 1) {
            check_condition = false;
            stability_condition += "\nУсловие устойчивости для второго узла может не выполняться.";
        }

        if (check_condition == false) {
            int answer = QMessageBox::warning(this, "Предупреждение", stability_condition +
                                              "\n\nРезультаты могут быть нестабильны. Все равно начать симуляцию?",
                                              QMessageBox::Yes| QMessageBox::Cancel, QMessageBox::Cancel);
            if (answer == QMessageBox::Yes) {
                return true;
            }
            else {
                return false;
            }
        }
    }
    return true;
}

void MainWindow::onProgressChange(int progress_value) {
    ui->progressBar->setValue(progress_value);
}

void MainWindow::on_clear_output_clicked() {
    ui->simulation_info->clear();
}

void MainWindow::onOutputTextChange(const QString &text) {
    ui->simulation_info->append(text);
}

void MainWindow::onIterationNumberChange() {
    QString text = "Итерация #" + QString::number(iteration_number + 1) + "/" +
                         QString::number(iteration_count);
    ui->iteration_info->setText(text);
}

void MainWindow::onProcessInfoChange(const QString& text) {
    ui->process_info->setText(text);
}

void MainWindow::closeEvent(QCloseEvent *event) {
   Q_UNUSED(event);
   if (model != nullptr) {
       model->stop();
   }
   event->accept();
}

void MainWindow::on_show_plot_clicked() {
    plotWindow->show();
}

void MainWindow::onPlotReady() {
    ui->show_plot->setEnabled(true);
}
