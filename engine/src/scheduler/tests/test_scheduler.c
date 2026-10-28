#include "job_manager.h"
#include "scheduler.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void add(JobManager *manager, const char *id, long arrival, long burst, long priority) {
    char error[256];
    assert(job_manager_add(manager, id, arrival, burst, priority, error, sizeof(error)));
}

static void test_fcfs(void) {
    JobManager manager;
    job_manager_init(&manager);
    add(&manager, "P1", 0, 5, 2);
    add(&manager, "P2", 1, 3, 1);
    add(&manager, "P3", 8, 2, 3);
    ScheduleResult result;
    char error[256];
    assert(scheduler_run(&manager, SCHED_FCFS, 1, &result, error, sizeof(error)));
    assert(result.completion_times[0] == 5);
    assert(result.completion_times[1] == 8);
    assert(result.completion_times[2] == 10);
    assert(result.turnaround_times[0] == 5 && result.waiting_times[0] == 0);
    assert(result.turnaround_times[1] == 7 && result.waiting_times[1] == 4);
    assert(result.turnaround_times[2] == 2 && result.waiting_times[2] == 0);
    assert(fabs(result.average_turnaround - 14.0 / 3.0) < 0.001);
    assert(fabs(result.average_waiting - 4.0 / 3.0) < 0.001);
    assert(result.slice_count == 3 && result.slices[2].start_time == 8);
    schedule_result_destroy(&result);
    job_manager_destroy(&manager);
}

static void test_priority_ties_and_idle(void) {
    JobManager manager;
    job_manager_init(&manager);
    add(&manager, "late", 5, 1, 1);
    add(&manager, "b", 0, 2, 3);
    add(&manager, "a", 0, 1, 3);
    ScheduleResult result;
    char error[256];
    assert(scheduler_run(&manager, SCHED_PRIORITY, 1, &result, error, sizeof(error)));
    assert(strcmp(result.slices[0].job_id, "b") == 0);
    assert(strcmp(result.slices[1].job_id, "a") == 0);
    assert(result.slices[0].start_time == 0 && result.slices[2].start_time == 5);
    assert(result.completion_times[0] == 6);
    schedule_result_destroy(&result);
    job_manager_destroy(&manager);
}

static void test_round_robin(void) {
    JobManager manager;
    job_manager_init(&manager);
    add(&manager, "P1", 0, 5, 0);
    add(&manager, "P2", 1, 3, 0);
    add(&manager, "P3", 2, 1, 0);
    ScheduleResult result;
    char error[256];
    assert(scheduler_run(&manager, SCHED_ROUND_ROBIN, 2, &result, error, sizeof(error)));
    assert(result.completion_times[0] == 9);
    assert(result.completion_times[1] == 8);
    assert(result.completion_times[2] == 5);
    assert(result.waiting_times[0] == 4);
    assert(result.waiting_times[1] == 4);
    assert(result.waiting_times[2] == 2);
    assert(result.slice_count == 6);
    schedule_result_destroy(&result);
    job_manager_destroy(&manager);
}

static void test_bad_quantum_and_empty(void) {
    JobManager manager;
    ScheduleResult result;
    char error[256];
    job_manager_init(&manager);
    assert(!scheduler_run(&manager, SCHED_FCFS, 1, &result, error, sizeof(error)));
    assert(error[0] != '\0');
    add(&manager, "P", 0, 1, 0);
    assert(!scheduler_run(&manager, SCHED_ROUND_ROBIN, 0, &result, error, sizeof(error)));
    assert(strstr(error, "quantum") != NULL);
    schedule_result_destroy(&result);
    job_manager_destroy(&manager);
}

int main(void) {
    test_fcfs();
    test_priority_ties_and_idle();
    test_round_robin();
    test_bad_quantum_and_empty();
    puts("FR2 scheduling tests passed.");
    return 0;
}
