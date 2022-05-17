#include "node.h"
#include <random>

Node::Node(int node_type, double mu, ProcessDistr err_distr, double scaling_par) {
    node_type_ = node_type;
    mu_ = mu;
    err_distr_ = err_distr;
    scaling_par_ = scaling_par;
    if (err_distr == ProcessDistr::exp_plain) {
        scaling_par_ = 1;
    }
}

// прибытие заявки
void Node::JobArrival(Job& job, double arrival_time) {
    if (job_count_duration_.size() < process_jobs_.size() + queued_jobs_.size() + 1) {
        job_count_duration_.push_back(arrival_time - current_job_count_time_);
    }
    else {
        job_count_duration_[process_jobs_.size() + queued_jobs_.size()] += arrival_time - current_job_count_time_;
    }

    if (job.GetJobType() != node_type_)
        ++false_arrival_count_;

    // если сервер свободен, то заявка туда попадает, иначе в очередь
    if (HasFreeServerAvailable()) {
        queue_waiting_times_.push_back(0);
        idle_end_ = arrival_time;
        idle_time_ += idle_end_ - idle_start_;
        JobProcessing(job, arrival_time);
    }
    else {
        ++busy_upon_arrival_count_;
        job.SetQueueStart(arrival_time);
        queued_jobs_.push(job);

    }
    current_job_count_time_ = arrival_time;
    total_system_time_ = arrival_time;
}

// обработка заявки, а также определение времени окончания обработки
// второй параметр здесь нужен для заявок, пришедших из очереди, так как для них
// время начала обработки равно времени ухода предыдущей заявки, и это нужно учитывать
void Node::JobProcessing(Job& job, double start_time) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::exponential_distribution<> distribution(mu_);

    // если заявка верно классифицирована, используем экспоненциальное распределение без доп. коэфф-ов
    if (node_type_ == job.GetJobType() || err_distr_ == ProcessDistr::exp_plain) {
        job.SetServiceStart(start_time);
        job.SetServiceEnd(start_time + distribution(gen));
    }
    else {
        if (err_distr_ == ProcessDistr::exp_scaled) {
            std::exponential_distribution<> err_distribution(mu_ * scaling_par_);
            job.SetServiceStart(start_time);
            job.SetServiceEnd(start_time + err_distribution(gen));
        }
        else if (err_distr_ == ProcessDistr::uniform) {
            // в случае равномерного распределения используем такой параметр, чтобы
            // среднее значение было 1 / (mu * scaling_par) и совпадало со случаем exp_scaled
            std::uniform_real_distribution<> err_distribution(0, 2 / (mu_ * scaling_par_));
            job.SetServiceStart(start_time);
            job.SetServiceEnd(start_time + err_distribution(gen));
        }
    }
    process_jobs_.push(job);
}

// выход заявки из узла
Job Node::JobDeparture() {
    Job finished_job = process_jobs_.front();
    if (job_count_duration_.size() < process_jobs_.size() + queued_jobs_.size() + 1) {
        job_count_duration_.push_back(finished_job.GetServiceEnd() - current_job_count_time_);
    }
    else {
        job_count_duration_[process_jobs_.size() + queued_jobs_.size()] += finished_job.GetServiceEnd() - current_job_count_time_;
    }
    process_jobs_.pop();

    // если очередь непуста, то освободившийся сервер занимает первая заявка оттуда
    if (queued_jobs_.size()) {
        Job next = queued_jobs_.front();
        queued_jobs_.pop();

        double process_start_time = finished_job.GetServiceEnd();
        next.SetQueueEnd(process_start_time);
        queue_waiting_times_.push_back(next.GetQueueEnd() - next.GetQueueStart());
        JobProcessing(next, process_start_time); // заявка из очереди начинает обработку, как только освободился сервер
    }
    else {
        idle_start_ = finished_job.GetServiceEnd();
    }
    total_system_time_ = finished_job.GetServiceEnd();
    current_job_count_time_ = total_system_time_;
    return finished_job;
}

bool Node::HasFreeServerAvailable() const {
    return process_jobs_.size() == 0; // т.к. в узле один сервер
}

double Node::GetIdleStart() const {
    return idle_start_;
}

double Node::GetIdleEnd() const {
    return idle_end_;
}

double Node::GetIdleTime() const {
    return idle_time_;
}

double Node::GetTotalSystemTime() const {
    return total_system_time_;
}

std::vector<double> Node::GetQueueWaitingTimes() const {
    return queue_waiting_times_;
}

ProcessDistr Node::GetErrorDistr() const {
    return err_distr_;
}

double Node::GetScalingPar() const {
    return scaling_par_;
}

double Node::GetMu() const {
    return mu_;
}

int Node::GetNodeType() const {
    return node_type_;
}

std::queue<Job> Node::GetProcessJobs() const {
    return process_jobs_;
}

std::queue<Job> Node::GetQueuedJobs() const {
    return queued_jobs_;
}

std::vector<double> Node::GetJobCountDuration() const {
    return job_count_duration_;
}

int Node::GetBusyUponArrivalCount() const {
    return busy_upon_arrival_count_;
}

int Node::GetFalseArrivalCount() const {
    return false_arrival_count_;
}

double Node::GetCurrentQueueLengthTime() const {
    return current_job_count_time_;
}
