#include "Job.h"
#include <vector>

Job::Job(int job_type, double system_arrival_time) :
    job_type_(job_type), system_arrival_time_(system_arrival_time) {
    node_arrival_time_ = system_arrival_time;
}

int Job::GetJobType() const {
    return job_type_;
}

int Job::GetAssignedNode() const {
    return assigned_node_;
}

void Job::SetAssignedNode(int node_index) {
    assigned_node_ = node_index;
}

void Job::SetNodeArrival(double time) {
    node_arrival_time_ = time;
}

void Job::SetServiceStart(double time) {
    service_start_time_ = time;
}

void Job::SetServiceEnd(double time) {
    service_end_time_ = time;
}

void Job::SetQueueStart(double time) {
    queue_start_time_ = time;
}

void Job::SetQueueEnd(double time) {
    queue_end_time_ = time;
}

void Job::SetOperationTime() {
    operation_time_ = service_end_time_ - system_arrival_time_;
}

double Job::GetSystemArrival() const {
    return system_arrival_time_;
}

double Job::GetNodeArrival() const {
    return node_arrival_time_;
}

double Job::GetServiceStart() const {
    return service_start_time_;
}

double Job::GetServiceEnd() const {
    return service_end_time_;
}

double Job::GetQueueStart() const {
    return queue_start_time_;
}

double Job::GetQueueEnd() const {
    return queue_end_time_;
}

double Job::GetOperationTime() const {
    return operation_time_;
}
