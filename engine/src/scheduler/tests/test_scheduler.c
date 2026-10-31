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

static void test_sjf(void) {
    JobManager m; job_manager_init(&m);
    add(&m, "P1", 0, 7, 0); add(&m, "P2", 2, 4, 0); add(&m, "P3", 4, 1, 0);
    ScheduleResult r; char e[256];
    assert(scheduler_run(&m, SCHED_SJF, 1, &r, e, sizeof(e)));
    assert(strcmp(r.slices[0].job_id, "P1") == 0);
    assert(strcmp(r.slices[1].job_id, "P3") == 0);
    assert(strcmp(r.slices[2].job_id, "P2") == 0);
    assert(r.completion_times[0] == 7 && r.completion_times[2] == 8 && r.completion_times[1] == 12);
    assert(r.waiting_times[1] == 6);
    schedule_result_destroy(&r); job_manager_destroy(&m);
}

static void test_srtf(void) {
    JobManager m; job_manager_init(&m);
    add(&m, "P1", 0, 8, 0); add(&m, "P2", 1, 4, 0); add(&m, "P3", 2, 2, 0);
    ScheduleResult r; char e[256];
    assert(scheduler_run(&m, SCHED_SRTF, 1, &r, e, sizeof(e)));
    assert(r.completion_times[2] == 4);
    assert(r.completion_times[1] == 7);
    assert(r.completion_times[0] == 14);
    assert(r.slice_count >= 4);
    schedule_result_destroy(&r); job_manager_destroy(&m);
}

static void test_multilevel(void) {
    JobManager m; job_manager_init(&m);
    add(&m, "FG", 0, 5, 0); add(&m, "INT", 0, 3, 1); add(&m, "BATCH", 0, 2, 2);
    ScheduleResult r; char e[256];
    assert(scheduler_run(&m, SCHED_MULTILEVEL_QUEUE, 1, &r, e, sizeof(e)));
    assert(r.completion_times[0] == 5);
    assert(r.completion_times[1] == 8);
    assert(r.completion_times[2] == 10);
    assert(r.total_time == 10);
    schedule_result_destroy(&r); job_manager_destroy(&m);
}

static void test_srtf_arrivals(void) {
    JobManager m; job_manager_init(&m);
    add(&m,"P1",0,8,0); add(&m,"P2",1,4,0); add(&m,"P3",2,2,0);
    ScheduleResult r; char e[256];
    assert(scheduler_run(&m,SCHED_SRTF,1,&r,e,sizeof(e)));
    assert(r.completion_times[2]==4);
    assert(r.completion_times[1]==7);
    assert(r.completion_times[0]==14);
    schedule_result_destroy(&r); job_manager_destroy(&m);
}
static void test_mlq_arrivals(void) {
    JobManager m; job_manager_init(&m);
    add(&m,"FG1",0,5,0); add(&m,"FG2",1,3,0);
    add(&m,"INT",0,2,1); add(&m,"BATCH",0,1,2);
    ScheduleResult r; char e[256];
    assert(scheduler_run(&m,SCHED_MULTILEVEL_QUEUE,1,&r,e,sizeof(e)));
    assert(r.completion_times[0]==8);
    assert(r.completion_times[1]==7);
    assert(r.completion_times[2]==10);
    assert(r.completion_times[3]==11);
    schedule_result_destroy(&r); job_manager_destroy(&m);
}

int main(void) {
    test_sjf();
    test_srtf();
    test_multilevel();
    test_srtf_arrivals();
    test_mlq_arrivals();
    puts("FR2 scheduling tests passed.");
    return 0;
}
