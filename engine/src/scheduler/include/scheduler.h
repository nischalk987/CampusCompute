#ifndef CAMPUSCOMPUTE_SCHEDULER_H
#define CAMPUSCOMPUTE_SCHEDULER_H

#include "job_manager.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SCHED_FCFS = 0,
    SCHED_ROUND_ROBIN,
    SCHED_PRIORITY
} SchedulingPolicy;

typedef struct {
    char job_id[JOB_ID_MAX + 1];
    unsigned int start_time;
    unsigned int end_time;
} ScheduleSlice;

typedef struct {
    ScheduleSlice *slices;
    size_t slice_count;
    unsigned int *completion_times;
    unsigned int *turnaround_times;
    unsigned int *waiting_times;
    size_t job_count;
    double average_turnaround;
    double average_waiting;
    unsigned int total_time;
} ScheduleResult;

const char *scheduling_policy_name(SchedulingPolicy policy);
void schedule_result_destroy(ScheduleResult *result);
/* Results' metric arrays follow input insertion order. FCFS ties: insertion order.
 * Priority: lower number wins, then earlier arrival, then insertion order.
 * Round Robin: FIFO; arrivals during a slice are enqueued before re-queueing
 * the unfinished running job.
 */
bool scheduler_run(const JobManager *manager, SchedulingPolicy policy,
                   unsigned int quantum, ScheduleResult *result,
                   char *error, size_t error_size);
void scheduler_print_result(const JobManager *manager, SchedulingPolicy policy,
                            const ScheduleResult *result);
#endif
