#ifndef JOB_H
#define JOB_H

class Job {
public:
    Job(int job_type, double system_arrival_time);

    void SetNodeArrival(double time);
    void SetServiceStart(double time);
    void SetServiceEnd(double time);
    void SetQueueStart(double time);
    void SetQueueEnd(double time);
    void SetAssignedNode(int node_index);
    void SetOperationTime();

    int GetAssignedNode() const;
    int GetJobType() const;
    double GetSystemArrival() const;
    double GetNodeArrival() const;
    double GetServiceStart() const;
    double GetServiceEnd() const;
    double GetQueueStart() const;
    double GetQueueEnd() const;
    double GetOperationTime() const;

private:
    int job_type_ = -1;
    double system_arrival_time_ = -1;
    int assigned_node_ = -1;
    double node_arrival_time_ = -1;
    double service_start_time_ = -1;
    double service_end_time_ = -1;
    double operation_time_ = -1;
    double queue_start_time_ = -1;
    double queue_end_time_ = -1;
};

#endif // JOB_H
