#include "Model.h"
#include <QDebug>
#include <random>
#include <algorithm>

Model::Model(int job_type_count, int processing_node_count, std::vector<double> lambda_params, std::vector<double> mu_params,
    std::vector<double> class_errors, ProcessDistr err_distr, std::vector<double> scaling_params)
    : job_type_count_(job_type_count), processing_node_count_(processing_node_count) {

    prev_arrival_times_.assign(job_type_count, 0);
    lambda_params_ = lambda_params;
    class_errors_ = class_errors;
    system_time_stats_.resize(processing_node_count);
    final_service_waiting_time_stats_.resize(processing_node_count);
    node_time_spent_.resize(processing_node_count);
    mu_params_ = mu_params;
    scaling_params_ = scaling_params;
    err_distr_ = err_distr;

    for (int i = 0; i < processing_node_count_; ++i) {
        if (err_distr_ == ProcessDistr::exp_plain)
            scaling_params_[i] = 1;
        nodes_.push_back(Node(i, mu_params_[i], err_distr_, scaling_params_[i]));
    }
}

// функция обработки заявок
bool Model::StartSimulation(quint64 count) {
    processInfoChanged("Генерация заявок...");
    bool generation_check = BatchArrival(count);
    if (!generation_check) {
        return false;
    }
    int check_count = 0, progress = 0; // для вывода прогресса
    double last_timepoint = -1;

    processInfoChanged("Обработка заявок...");
    while (jobs_to_process_.size()) {
        if (simulation_stop) {
            return false;
        }
        Job first_arrived_job = jobs_to_process_.front();
        int num_of_node = first_arrived_job.GetAssignedNode();

        Node& current_node = nodes_[num_of_node];
        Node& other_node = nodes_[1 - num_of_node];

        double first_time = first_arrived_job.GetNodeArrival(); // время прибытия текущей заявки
        // выбирается событие, которое произойдет следующим по времени
        if ((current_node.HasFreeServerAvailable() || first_time < current_node.GetProcessJobs().front().GetServiceEnd())
            && (other_node.HasFreeServerAvailable() || first_time < other_node.GetProcessJobs().front().GetServiceEnd())) {
            current_node.JobArrival(first_arrived_job, first_time);
            jobs_to_process_.pop();
            ++check_count;
            if (last_timepoint > first_time) {
                qDebug() << "Ошибка: была нарушена хронологическая последовательность событий во время обработки заявок.";
                return false;
            }
            last_timepoint = first_time;
        }
        else {
            int departing_job_node_type = -1;
            // вычисление номера узла, у которого произошло окончание обслуживания заявки
            if (nodes_[num_of_node].HasFreeServerAvailable()) {
                if (other_node.HasFreeServerAvailable()) {
                    departing_job_node_type = num_of_node;
                }
                else {
                    if (first_time < other_node.GetProcessJobs().front().GetServiceEnd()) {
                        departing_job_node_type = num_of_node;
                    }
                    else {
                        departing_job_node_type = 1 - num_of_node;
                    }
                }
            }
            else {
                if (other_node.HasFreeServerAvailable()) {
                    departing_job_node_type = num_of_node;
                }
                else {
                    if (nodes_[num_of_node].GetProcessJobs().front().GetServiceEnd() <
                        other_node.GetProcessJobs().front().GetServiceEnd()) {
                        departing_job_node_type = num_of_node;
                    }
                    else {
                        departing_job_node_type = 1 - num_of_node;
                    }
                }
            }
            double next_departure_time = nodes_[departing_job_node_type].GetProcessJobs().front().GetServiceEnd();

            if (last_timepoint > next_departure_time) {
                qDebug() << "Ошибка: была нарушена хронологическая последовательность событий во время обработки заявок.";
                return false;
            }
            last_timepoint = next_departure_time;

            Job processed_job = nodes_[departing_job_node_type].JobDeparture();
            if (next_departure_time != processed_job.GetServiceEnd()) {
                qDebug() << "Ошибка: время окончания обслуживания не совпадает с ожидаемым значением.";
                return false;
            }

            node_time_spent_[departing_job_node_type].push_back(processed_job.GetServiceEnd() - processed_job.GetNodeArrival());
            if (node_time_spent_[departing_job_node_type].back() < 0) {
                qDebug() << "Ошибка: время пребывания в узле оказалось отрицательным.";
                return false;
            }

            if (departing_job_node_type == processed_job.GetJobType()) {
                processed_job.SetOperationTime();
                if (processed_job.GetOperationTime() < 0) {
                    qDebug() << "Ошибка: время пребывания в системе оказалось отрицательным.";
                    return false;
                }
                system_time_stats_[departing_job_node_type].push_back(processed_job.GetOperationTime());
                final_service_waiting_time_stats_[departing_job_node_type].push_back(processed_job.GetServiceStart() - processed_job.GetSystemArrival());
            }
            else {
                ReclassifyJob(processed_job);
                nodes_[processed_job.GetAssignedNode()].JobArrival(processed_job, processed_job.GetNodeArrival());
            }
        }

        if (progress < int(double(check_count) * 100 / count)) {
            progress = int(double(check_count) * 100 / count);
            emit progressChanged(progress);
        }
    }
    return true;
}

// производит повторную классификацию заявки
void Model::ReclassifyJob(Job& job) {
    int corresp_node_ind = GetRespectiveNodeId(job.GetJobType());
    job.SetAssignedNode(corresp_node_ind);
    job.SetNodeArrival(job.GetServiceEnd());
}

// создание 'count' заявок в хронологической последовательности
bool Model::BatchArrival(quint64 count) {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::vector<int> node_count(processing_node_count_, 0);
    std::vector<double> temp_arrival_times(job_type_count_, 0); // массив с предвратительным временем прибытия для каждой заявки
    int prev_job_type = -1;

    // сначала рассчитываем предварительное время прибытия для каждой заявки
    for (int i = 0; i < job_type_count_; ++i) {
        std::exponential_distribution<double> distribution(lambda_params_[i]);
        temp_arrival_times[i] = prev_arrival_times_[i] + distribution(gen);
    }
    int progress = 0;
    for (int k = 0; k < count; ++k) {
        if (simulation_stop) {
            return false;
        }

        if (prev_job_type != -1) { // на первой итерации пропускаем этот шаг
            // только для предыдущей заявки рассчитываем следующее предварительное время прибытия
            std::exponential_distribution<double> distribution(lambda_params_[prev_job_type]);
            temp_arrival_times[prev_job_type] = prev_arrival_times_[prev_job_type] + distribution(gen);
        }

        // получаем тип следующей в очереди заявки
        int next_job_type = distance(begin(temp_arrival_times),
                                     min_element(begin(temp_arrival_times), end(temp_arrival_times)));

        double arrival_time = temp_arrival_times[next_job_type];
        Job job(next_job_type, arrival_time);
        int corresp_node_ind = GetRespectiveNodeId(next_job_type); // классифицируем заявку
        job.SetAssignedNode(corresp_node_ind);
        ++node_count[corresp_node_ind];
        prev_arrival_times_[next_job_type] = arrival_time;
        jobs_to_process_.push(job);
        prev_job_type = next_job_type; // обновляем значение предыдущего типа заявки

        if (progress < static_cast<int>(static_cast<double>(k) * 100 / count)) {
            progress = static_cast<int>(static_cast<double>(k) * 100 / count);
            emit progressChanged(progress); // оповещаем о состоянии прогресса обработки заявок
        }
    }
    return true;
}

// функция-классификатор, возвращает номер назначенного узла
int Model::GetRespectiveNodeId(int job_type) const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    double value = dis(gen);

    if (value > class_errors_[job_type])
        return job_type;
    else
        return 1 - job_type;
}

std::vector<Node> Model::GetNodes() const {
    return nodes_;
}

int Model::GetJobTypeCount() const {
    return job_type_count_;
}

int Model::GetProcessingNodeCount() const {
    return processing_node_count_;
}

std::vector<double> Model::GetLambdaParams() const {
    return lambda_params_;
}

std::vector<double> Model::GetMuParams() const {
    return mu_params_;
}

std::vector<double> Model::GetScalingParams() const {
    return scaling_params_;
}

ProcessDistr Model::GetErrDistr() const {
    return err_distr_;
}

std::vector<double> Model::GetClassErrors() const {
    return class_errors_;
}

std::vector<std::vector<double>> Model::GetSystemTimeStats() const {
    return system_time_stats_;
}

std::vector<std::vector<double> > Model::GetFinalServiceWaitingTimeStats() const {
    return final_service_waiting_time_stats_;
}

std::vector<std::vector<double>> Model::GetNodeTimeSpentStats() const {
    return node_time_spent_;
}

// обнуляем модель для повторного использования
void Model::reset() {
    std::queue<Job>().swap(jobs_to_process_);
    for (int i = 0; i < processing_node_count_; ++i ) {
        system_time_stats_[i].clear();
        node_time_spent_[i].clear();
        prev_arrival_times_[i] = 0;
    }

    for (int i = 0; i < processing_node_count_; ++i) {
        nodes_[i] = Node(i, mu_params_[i], err_distr_, scaling_params_[i]);
    }
}

// функция для вызова остановки процесса симуляции
void Model::stop() {
    simulation_stop = true;
}
