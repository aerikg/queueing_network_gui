#ifndef MODEL_H
#define MODEL_H

#include "node.h"
#include <QObject>

class Model : public QObject {
    Q_OBJECT
public:
    Model(int num_of_process_nodes = 2, int num_of_job_types = 2,
        std::vector<double> lambda_params = { 1.0, 1.0 }, std::vector<double> mu_params = { 2.0, 2.0 },
        std::vector<double> class_errors = { 0.0, 0.0 }, ProcessDistr err_distr = ProcessDistr::exp_plain,
        std::vector<double> scaling_params = { 1, 1 });

    bool StartSimulation(quint64 count);
    bool BatchArrival(quint64 count);
    void ReclassifyJob(Job& job);

    std::vector<Node> GetNodes() const;
    int GetJobTypeCount() const;
    int GetProcessingNodeCount() const;
    std::vector<double> GetLambdaParams() const;
    std::vector<double> GetClassErrors() const;
    std::vector<double> GetMuParams() const;
    std::vector<double> GetScalingParams() const;
    ProcessDistr GetErrDistr() const;
    std::vector<std::vector<double>> GetSystemTimeStats() const;
    std::vector<std::vector<double>> GetFinalServiceWaitingTimeStats() const;
    std::vector<std::vector<double>> GetNodeTimeSpentStats() const;

    void reset();

signals:
    void progressChanged(int value);
    void processInfoChanged(const QString& text);

public slots:
    void stop();

private:
    const int job_type_count_;
    const int processing_node_count_; // помимо классификатора (т.е. только обрабатывающие узлы)
    std::vector<double> lambda_params_;
    std::vector<double> mu_params_;
    std::vector<double> class_errors_;
    std::vector<double> scaling_params_;
    std::vector<double> prev_arrival_times_;
    ProcessDistr err_distr_;
    std::vector<Node> nodes_;
    std::vector<std::vector<double>> system_time_stats_;
    std::vector<std::vector<double>> final_service_waiting_time_stats_;
    std::vector<std::vector<double>> node_time_spent_; // среднее время, которое заявка проводит в конкретном узле (а не в системе)
    std::queue<Job> jobs_to_process_;
    bool simulation_stop = false;

    int GetRespectiveNodeId(int job_type) const;
};

#endif // MODEL_H
