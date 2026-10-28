#ifndef CAMPUSCOMPUTE_JOB_MANAGER_H
#define CAMPUSCOMPUTE_JOB_MANAGER_H
#include <stdbool.h>
#include <stddef.h>
#define JOB_ID_MAX 31
typedef enum { JOB_CREATED=0, JOB_READY, JOB_RUNNING, JOB_COMPLETED } JobStatus;
typedef struct {
    char id[JOB_ID_MAX + 1];
    unsigned int arrival_time, burst_time, remaining_time, completion_time;
    int priority;
    JobStatus status;
} Job;
typedef struct { Job *jobs; size_t count, capacity; } JobManager;
void job_manager_init(JobManager *manager);
void job_manager_destroy(JobManager *manager);
bool job_manager_add(JobManager *manager, const char *id, long arrival_time,
                     long burst_time, long priority, char *error, size_t error_size);
Job *job_manager_find(JobManager *manager, const char *id);
const Job *job_manager_find_const(const JobManager *manager, const char *id);
bool job_manager_mark_ready(JobManager *manager, const char *id, char *error, size_t error_size);
bool job_manager_mark_running(JobManager *manager, const char *id, char *error, size_t error_size);
bool job_manager_mark_completed(JobManager *manager, const char *id, unsigned int completion_time,
                                char *error, size_t error_size);
const char *job_status_name(JobStatus status);
void job_manager_list(const JobManager *manager);
#endif
