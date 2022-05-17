#ifndef NODE_H
#define NODE_H

#include "job.h"
#include <vector>
#include <queue>

#include <iostream>

enum class ProcessDistr { exp_plain, exp_scaled, uniform };

class Node {
public:
    Node(int node_type, double mu, ProcessDistr err_distr, double scaling_par);
    void JobArrival(Job& j, double time);
    void JobProcessing(Job& j, double time);
    Job JobDeparture();
    bool HasFreeServerAvailable() const;

    int GetNodeType() const;
    double GetMu() const;
    ProcessDistr GetErrorDistr() const;
    double GetScalingPar() const;
    double GetIdleStart() const;
    double GetIdleEnd() const;
    double GetIdleTime() const;
    double GetTotalSystemTime() const;
    double GetAverageQueueLength() const;
    std::queue<Job> GetProcessJobs() const;
    std::queue<Job> GetQueuedJobs() const;
    std::vector<double> GetQueueWaitingTimes() const;
    std::vector<double> GetJobCountDuration() const;
    int GetBusyUponArrivalCount() const;
    int GetFalseArrivalCount() const;

private:
    int node_type_;
    double mu_;
    std::queue<Job> process_jobs_;
    std::queue<Job> queued_jobs_;

    double idle_start_ = 0;
    double idle_end_ = 0;
    double idle_time_ = 0;
    double total_system_time_ = 0;
    double current_job_count_time_ = 0;

    ProcessDistr err_distr_;
    double scaling_par_;

    std::vector<double> job_count_duration_;
    std::vector<double> queue_waiting_times_;

    int busy_upon_arrival_count_ = 0;
    int false_arrival_count_ = 0;

    double GetCurrentQueueLengthTime() const;
    double GetQueueLengthSum() const;
};

#endif // NODE_H
