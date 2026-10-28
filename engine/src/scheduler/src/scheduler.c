#define _POSIX_C_SOURCE 200809L
#include "scheduler.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_JOBS 10000U

static void set_error(char *out, size_t size, const char *message) {
    if (out != NULL && size > 0) {
        (void)snprintf(out, size, "%s", message);
    }
}

const char *scheduling_policy_name(SchedulingPolicy policy) {
    switch (policy) {
        case SCHED_FCFS: return "FCFS";
        case SCHED_ROUND_ROBIN: return "Round Robin";
        case SCHED_PRIORITY: return "Non-preemptive Priority";
        default: return "Unknown";
    }
}

void schedule_result_destroy(ScheduleResult *result) {
    if (result == NULL) return;
    free(result->slices);
    free(result->completion_times);
    free(result->turnaround_times);
    free(result->waiting_times);
    memset(result, 0, sizeof(*result));
}

static bool validate_jobs(const JobManager *manager, char *error, size_t error_size) {
    uint64_t total_burst = 0;
    unsigned int latest_arrival = 0;
    if (manager == NULL || manager->jobs == NULL || manager->count == 0) {
        set_error(error, error_size, "Add at least one job before scheduling.");
        return false;
    }
    if (manager->count > MAX_JOBS) {
        set_error(error, error_size, "Scheduling is limited to 10000 jobs.");
        return false;
    }
    for (size_t i = 0; i < manager->count; ++i) {
        const Job *job = &manager->jobs[i];
        if (job->id[0] == '\0' || job->burst_time == 0) {
            set_error(error, error_size, "A job has an empty ID or zero CPU burst.");
            return false;
        }
        if (job->arrival_time > latest_arrival) latest_arrival = job->arrival_time;
        total_burst += job->burst_time;
        for (size_t j = i + 1; j < manager->count; ++j) {
            if (strcmp(job->id, manager->jobs[j].id) == 0) {
                set_error(error, error_size, "Job IDs must be unique.");
                return false;
            }
        }
    }
    if (total_burst + latest_arrival > UINT32_MAX) {
        set_error(error, error_size, "Total scheduling time exceeds the supported 32-bit time range.");
        return false;
    }
    return true;
}

static bool allocate_result(const JobManager *manager, ScheduleResult *result,
                            char *error, size_t error_size) {
    memset(result, 0, sizeof(*result));
    result->job_count = manager->count;
    result->completion_times = calloc(manager->count, sizeof(*result->completion_times));
    result->turnaround_times = calloc(manager->count, sizeof(*result->turnaround_times));
    result->waiting_times = calloc(manager->count, sizeof(*result->waiting_times));
    if (result->completion_times == NULL || result->turnaround_times == NULL ||
        result->waiting_times == NULL) {
        set_error(error, error_size, "Unable to allocate scheduling result memory.");
        schedule_result_destroy(result);
        return false;
    }
    return true;
}

static bool append_slice(ScheduleResult *result, const char *id,
                         unsigned int start, unsigned int end,
                         char *error, size_t error_size) {
    if (start == end) return true;
    if (result->slice_count == SIZE_MAX / sizeof(*result->slices)) {
        set_error(error, error_size, "Scheduling timeline is too large.");
        return false;
    }
    ScheduleSlice *grown = realloc(result->slices,
                                   (result->slice_count + 1) * sizeof(*result->slices));
    if (grown == NULL) {
        set_error(error, error_size, "Unable to grow scheduling timeline.");
        return false;
    }
    result->slices = grown;
    (void)snprintf(result->slices[result->slice_count].job_id,
                   sizeof(result->slices[result->slice_count].job_id), "%s", id);
    result->slices[result->slice_count].start_time = start;
    result->slices[result->slice_count].end_time = end;
    result->slice_count++;
    return true;
}

static void calculate_metrics(const JobManager *manager, ScheduleResult *result) {
    double turnaround_total = 0.0;
    double waiting_total = 0.0;
    for (size_t i = 0; i < manager->count; ++i) {
        unsigned int turnaround = result->completion_times[i] - manager->jobs[i].arrival_time;
        unsigned int waiting = turnaround - manager->jobs[i].burst_time;
        result->turnaround_times[i] = turnaround;
        result->waiting_times[i] = waiting;
        turnaround_total += turnaround;
        waiting_total += waiting;
    }
    result->average_turnaround = turnaround_total / (double)manager->count;
    result->average_waiting = waiting_total / (double)manager->count;
}

static void sort_indices_by_arrival(const JobManager *manager, size_t *indices) {
    for (size_t i = 0; i < manager->count; ++i) indices[i] = i;
    for (size_t i = 1; i < manager->count; ++i) {
        size_t key = indices[i];
        size_t j = i;
        while (j > 0) {
            size_t previous = indices[j - 1];
            if (manager->jobs[previous].arrival_time < manager->jobs[key].arrival_time) break;
            if (manager->jobs[previous].arrival_time == manager->jobs[key].arrival_time &&
                previous < key) break;
            indices[j] = indices[j - 1];
            --j;
        }
        indices[j] = key;
    }
}

static bool run_fcfs(const JobManager *manager, ScheduleResult *result,
                     char *error, size_t error_size) {
    size_t *order = malloc(manager->count * sizeof(*order));
    if (order == NULL) {
        set_error(error, error_size, "Unable to allocate FCFS order.");
        return false;
    }
    sort_indices_by_arrival(manager, order);
    unsigned int clock = 0;
    for (size_t k = 0; k < manager->count; ++k) {
        size_t i = order[k];
        const Job *job = &manager->jobs[i];
        if (clock < job->arrival_time) clock = job->arrival_time;
        unsigned int start = clock;
        clock += job->burst_time;
        if (!append_slice(result, job->id, start, clock, error, error_size)) {
            free(order);
            return false;
        }
        result->completion_times[i] = clock;
    }
    result->total_time = clock;
    free(order);
    return true;
}

static bool run_priority(const JobManager *manager, ScheduleResult *result,
                         char *error, size_t error_size) {
    bool *done = calloc(manager->count, sizeof(*done));
    if (done == NULL) {
        set_error(error, error_size, "Unable to allocate priority scheduler state.");
        return false;
    }
    unsigned int clock = 0;
    size_t completed = 0;
    while (completed < manager->count) {
        size_t best = manager->count;
        for (size_t i = 0; i < manager->count; ++i) {
            const Job *job = &manager->jobs[i];
            if (done[i] || job->arrival_time > clock) continue;
            if (best == manager->count ||
                job->priority < manager->jobs[best].priority ||
                (job->priority == manager->jobs[best].priority &&
                 (job->arrival_time < manager->jobs[best].arrival_time ||
                  (job->arrival_time == manager->jobs[best].arrival_time && i < best)))) {
                best = i;
            }
        }
        if (best == manager->count) {
            unsigned int next_arrival = UINT32_MAX;
            for (size_t i = 0; i < manager->count; ++i) {
                if (!done[i] && manager->jobs[i].arrival_time < next_arrival) {
                    next_arrival = manager->jobs[i].arrival_time;
                }
            }
            if (next_arrival == UINT32_MAX) break;
            clock = next_arrival;
            continue;
        }
        const Job *job = &manager->jobs[best];
        unsigned int start = clock;
        clock += job->burst_time;
        if (!append_slice(result, job->id, start, clock, error, error_size)) {
            free(done);
            return false;
        }
        result->completion_times[best] = clock;
        done[best] = true;
        completed++;
    }
    result->total_time = clock;
    free(done);
    return true;
}

static bool run_round_robin(const JobManager *manager, unsigned int quantum,
                            ScheduleResult *result, char *error, size_t error_size) {
    if (quantum == 0) {
        set_error(error, error_size, "Round Robin quantum must be greater than zero.");
        return false;
    }
    size_t *arrival_order = malloc(manager->count * sizeof(*arrival_order));
    size_t *ready = malloc(manager->count * sizeof(*ready));
    unsigned int *remaining = malloc(manager->count * sizeof(*remaining));
    if (arrival_order == NULL || ready == NULL || remaining == NULL) {
        free(arrival_order);
        free(ready);
        free(remaining);
        set_error(error, error_size, "Unable to allocate Round Robin queue.");
        return false;
    }
    sort_indices_by_arrival(manager, arrival_order);
    for (size_t i = 0; i < manager->count; ++i) remaining[i] = manager->jobs[i].burst_time;

    size_t head = 0;
    size_t tail = 0;
    size_t queue_size = 0;
    size_t enqueued = 0;
    size_t completed = 0;
    unsigned int clock = 0;

    while (completed < manager->count) {
        while (enqueued < manager->count &&
               manager->jobs[arrival_order[enqueued]].arrival_time <= clock) {
            ready[tail] = arrival_order[enqueued++];
            tail = (tail + 1) % manager->count;
            queue_size++;
        }
        if (queue_size == 0) {
            if (enqueued < manager->count) {
                clock = manager->jobs[arrival_order[enqueued]].arrival_time;
                continue;
            }
            break;
        }

        size_t i = ready[head];
        head = (head + 1) % manager->count;
        queue_size--;
        unsigned int run_for = remaining[i] < quantum ? remaining[i] : quantum;
        unsigned int start = clock;
        clock += run_for;
        remaining[i] -= run_for;

        if (!append_slice(result, manager->jobs[i].id, start, clock, error, error_size)) {
            free(arrival_order);
            free(ready);
            free(remaining);
            return false;
        }

        /* First queue arrivals that appeared during this slice; then requeue the running job. */
        while (enqueued < manager->count &&
               manager->jobs[arrival_order[enqueued]].arrival_time <= clock) {
            ready[tail] = arrival_order[enqueued++];
            tail = (tail + 1) % manager->count;
            queue_size++;
        }
        if (remaining[i] > 0) {
            ready[tail] = i;
            tail = (tail + 1) % manager->count;
            queue_size++;
        } else {
            result->completion_times[i] = clock;
            completed++;
        }
    }

    result->total_time = clock;
    free(arrival_order);
    free(ready);
    free(remaining);
    return true;
}

bool scheduler_run(const JobManager *manager, SchedulingPolicy policy,
                   unsigned int quantum, ScheduleResult *result,
                   char *error, size_t error_size) {
    if (result == NULL) {
        set_error(error, error_size, "Schedule result pointer is null.");
        return false;
    }
    memset(result, 0, sizeof(*result));
    if (!validate_jobs(manager, error, error_size)) return false;
    if (policy != SCHED_FCFS && policy != SCHED_ROUND_ROBIN && policy != SCHED_PRIORITY) {
        set_error(error, error_size, "Unknown scheduling policy.");
        return false;
    }
    if (policy == SCHED_ROUND_ROBIN && quantum == 0) {
        set_error(error, error_size, "Round Robin quantum must be greater than zero.");
        return false;
    }
    if (!allocate_result(manager, result, error, error_size)) return false;

    bool ok;
    if (policy == SCHED_FCFS) ok = run_fcfs(manager, result, error, error_size);
    else if (policy == SCHED_PRIORITY) ok = run_priority(manager, result, error, error_size);
    else ok = run_round_robin(manager, quantum, result, error, error_size);
    if (!ok) {
        schedule_result_destroy(result);
        return false;
    }
    calculate_metrics(manager, result);
    set_error(error, error_size, "");
    return true;
}

void scheduler_print_result(const JobManager *manager, SchedulingPolicy policy,
                            const ScheduleResult *result) {
    if (manager == NULL || result == NULL) return;
    printf("\n=== %s Scheduling ===\n", scheduling_policy_name(policy));
    puts("Timeline:");
    if (result->slice_count == 0) puts("  (no execution slices)");
    for (size_t i = 0; i < result->slice_count; ++i) {
        printf("  [%u - %u] %s\n", result->slices[i].start_time,
               result->slices[i].end_time, result->slices[i].job_id);
    }
    puts("\nPer-job metrics:");
    puts("ID                              CT       TAT      WT");
    puts("------------------------------  -------  -------  -------");
    for (size_t i = 0; i < manager->count; ++i) {
        printf("%-30s  %7u  %7u  %7u\n", manager->jobs[i].id,
               result->completion_times[i], result->turnaround_times[i],
               result->waiting_times[i]);
    }
    printf("Average turnaround time: %.2f\n", result->average_turnaround);
    printf("Average waiting time:    %.2f\n", result->average_waiting);
    printf("Makespan:                %u\n", result->total_time);
}
