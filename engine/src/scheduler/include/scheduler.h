#ifndef CAMPUSCOMPUTE_SCHEDULER_H
#define CAMPUSCOMPUTE_SCHEDULER_H

#include "job_manager.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SCHED_FCFS = 0,
    SCHED_ROUND_ROBIN,
    SCHED_PRIORITY,
    SCHED_SJF,
    SCHED_SRTF,
    SCHED_MULTILEVEL_QUEUE
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
/* Lower numeric priority wins. Multilevel Queue uses three fixed queues:
 * queue 0 (priority <= 0): foreground, RR quantum 2;
 * queue 1 (priority == 1): interactive, RR quantum 4;
 * queue 2 (priority >= 2): batch, FCFS.
 * Strict queue priority applies; lower queues run only when higher queues are empty.
 * SJF is non-preemptive; SRTF is preemptive. Equal remaining/burst times use
 * earlier arrival, then input insertion order.
 */
bool scheduler_run(const JobManager *manager, SchedulingPolicy policy,
                   unsigned int quantum, ScheduleResult *result,
                   char *error, size_t error_size);
void scheduler_print_result(const JobManager *manager, SchedulingPolicy policy,
                            const ScheduleResult *result);
#endif
