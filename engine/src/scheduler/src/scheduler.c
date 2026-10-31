#define _POSIX_C_SOURCE 200809L
#include "scheduler.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_JOBS 10000U

static void set_error(char *out, size_t size, const char *message) {
    if (out != NULL && size > 0) (void)snprintf(out, size, "%s", message);
}

const char *scheduling_policy_name(SchedulingPolicy policy) {
    switch (policy) {
        case SCHED_FCFS: return "FCFS";
        case SCHED_ROUND_ROBIN: return "Round Robin";
        case SCHED_PRIORITY: return "Non-preemptive Priority";
        case SCHED_SJF: return "Shortest Job First (SJF)";
        case SCHED_SRTF: return "Shortest Remaining Time First (SRTF)";
        case SCHED_MULTILEVEL_QUEUE: return "Multilevel Queue";
        default: return "Unknown";
    }
}

void schedule_result_destroy(ScheduleResult *result) {
    if (!result) return;
    free(result->slices);
    free(result->completion_times);
    free(result->turnaround_times);
    free(result->waiting_times);
    memset(result, 0, sizeof(*result));
}

static bool validate_jobs(const JobManager *manager, char *error, size_t error_size) {
    uint64_t total_burst = 0;
    unsigned int latest_arrival = 0;
    if (!manager || !manager->jobs || manager->count == 0) {
        set_error(error, error_size, "Add at least one job before scheduling."); return false;
    }
    if (manager->count > MAX_JOBS) {
        set_error(error, error_size, "Scheduling is limited to 10000 jobs."); return false;
    }
    for (size_t i = 0; i < manager->count; ++i) {
        const Job *job = &manager->jobs[i];
        if (job->id[0] == '\0' || job->burst_time == 0) {
            set_error(error, error_size, "A job has an empty ID or zero CPU burst."); return false;
        }
        if (job->arrival_time > latest_arrival) latest_arrival = job->arrival_time;
        total_burst += job->burst_time;
        for (size_t j = i + 1; j < manager->count; ++j)
            if (strcmp(job->id, manager->jobs[j].id) == 0) {
                set_error(error, error_size, "Job IDs must be unique."); return false;
            }
    }
    if (total_burst + latest_arrival > UINT32_MAX) {
        set_error(error, error_size, "Total scheduling time exceeds the supported 32-bit time range."); return false;
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
    if (!result->completion_times || !result->turnaround_times || !result->waiting_times) {
        set_error(error, error_size, "Unable to allocate scheduling result memory.");
        schedule_result_destroy(result); return false;
    }
    return true;
}

static bool append_slice(ScheduleResult *result, const char *id, unsigned int start,
                         unsigned int end, char *error, size_t error_size) {
    if (start == end) return true;
    if (result->slice_count == SIZE_MAX / sizeof(*result->slices)) {
        set_error(error, error_size, "Scheduling timeline is too large."); return false;
    }
    ScheduleSlice *grown = realloc(result->slices, (result->slice_count + 1) * sizeof(*result->slices));
    if (!grown) { set_error(error, error_size, "Unable to grow scheduling timeline."); return false; }
    result->slices = grown;
    (void)snprintf(result->slices[result->slice_count].job_id,
                   sizeof(result->slices[result->slice_count].job_id), "%s", id);
    result->slices[result->slice_count].start_time = start;
    result->slices[result->slice_count].end_time = end;
    result->slice_count++;
    return true;
}

static void calculate_metrics(const JobManager *manager, ScheduleResult *result) {
    double tat_total = 0.0, wait_total = 0.0;
    for (size_t i = 0; i < manager->count; ++i) {
        unsigned int tat = result->completion_times[i] - manager->jobs[i].arrival_time;
        unsigned int waiting = tat - manager->jobs[i].burst_time;
        result->turnaround_times[i] = tat;
        result->waiting_times[i] = waiting;
        tat_total += tat; wait_total += waiting;
    }
    result->average_turnaround = tat_total / (double)manager->count;
    result->average_waiting = wait_total / (double)manager->count;
}

static void sort_indices_by_arrival(const JobManager *manager, size_t *indices) {
    for (size_t i = 0; i < manager->count; ++i) indices[i] = i;
    for (size_t i = 1; i < manager->count; ++i) {
        size_t key = indices[i], j = i;
        while (j > 0) {
            size_t previous = indices[j - 1];
            if (manager->jobs[previous].arrival_time < manager->jobs[key].arrival_time ||
                (manager->jobs[previous].arrival_time == manager->jobs[key].arrival_time && previous < key)) break;
            indices[j] = indices[j - 1]; --j;
        }
        indices[j] = key;
    }
}

static bool run_fcfs(const JobManager *manager, ScheduleResult *result, char *error, size_t size) {
    size_t *order = malloc(manager->count * sizeof(*order));
    if (!order) { set_error(error, size, "Unable to allocate FCFS order."); return false; }
    sort_indices_by_arrival(manager, order);
    unsigned int clock = 0;
    for (size_t k = 0; k < manager->count; ++k) {
        size_t i = order[k]; const Job *job = &manager->jobs[i];
        if (clock < job->arrival_time) clock = job->arrival_time;
        unsigned int start = clock; clock += job->burst_time;
        if (!append_slice(result, job->id, start, clock, error, size)) { free(order); return false; }
        result->completion_times[i] = clock;
    }
    result->total_time = clock; free(order); return true;
}

static bool run_nonpreemptive_choice(const JobManager *manager, ScheduleResult *result,
                                     SchedulingPolicy policy, char *error, size_t size) {
    bool *done = calloc(manager->count, sizeof(*done));
    if (!done) { set_error(error, size, "Unable to allocate scheduler state."); return false; }
    unsigned int clock = 0; size_t completed = 0;
    while (completed < manager->count) {
        size_t best = manager->count;
        for (size_t i = 0; i < manager->count; ++i) {
            const Job *job = &manager->jobs[i];
            if (done[i] || job->arrival_time > clock) continue;
            bool better = best == manager->count;
            if (!better && policy == SCHED_PRIORITY)
                better = job->priority < manager->jobs[best].priority ||
                    (job->priority == manager->jobs[best].priority &&
                     (job->arrival_time < manager->jobs[best].arrival_time ||
                      (job->arrival_time == manager->jobs[best].arrival_time && i < best)));
            else if (!better && policy == SCHED_SJF)
                better = job->burst_time < manager->jobs[best].burst_time ||
                    (job->burst_time == manager->jobs[best].burst_time &&
                     (job->arrival_time < manager->jobs[best].arrival_time ||
                      (job->arrival_time == manager->jobs[best].arrival_time && i < best)));
            if (better) best = i;
        }
        if (best == manager->count) {
            unsigned int next = UINT32_MAX;
            for (size_t i = 0; i < manager->count; ++i)
                if (!done[i] && manager->jobs[i].arrival_time < next) next = manager->jobs[i].arrival_time;
            if (next == UINT32_MAX) break;
            clock = next; continue;
        }
        const Job *job = &manager->jobs[best];
        unsigned int start = clock; clock += job->burst_time;
        if (!append_slice(result, job->id, start, clock, error, size)) { free(done); return false; }
        result->completion_times[best] = clock; done[best] = true; completed++;
    }
    result->total_time = clock; free(done); return true;
}

static bool run_round_robin_subset(const JobManager *manager, ScheduleResult *result,
                                   unsigned int quantum, int queue_level, bool *done,
                                   unsigned int *remaining, unsigned int *clock,
                                   char *error, size_t size) {
    size_t n = manager->count;
    size_t *order = malloc(n * sizeof(*order));
    size_t *queue = malloc(n * sizeof(*queue));
    bool *enqueued_flag = calloc(n, sizeof(*enqueued_flag));
    if (!order || !queue || !enqueued_flag) {
        free(order); free(queue); free(enqueued_flag);
        set_error(error, size, "Unable to allocate ready queue.");
        return false;
    }
    sort_indices_by_arrival(manager, order);
    size_t head=0, tail=0, count=0, enqueued=0, completed=0, total=0;
    for (size_t i=0;i<n;i++) {
        bool selected=queue_level==0?manager->jobs[i].priority<=0:
                      queue_level==1?manager->jobs[i].priority==1:
                      manager->jobs[i].priority>=2;
        if (selected) total++;
    }
    while (completed<total) {
        while(enqueued<n && manager->jobs[order[enqueued]].arrival_time<=*clock) {
            size_t i=order[enqueued++];
            bool selected=queue_level==0?manager->jobs[i].priority<=0:
                          queue_level==1?manager->jobs[i].priority==1:
                          manager->jobs[i].priority>=2;
            if(selected&&!done[i]&&!enqueued_flag[i]) {
                queue[tail]=i;tail=(tail+1)%n;count++;enqueued_flag[i]=true;
            }
        }
        if(!count) {
            unsigned int next=UINT32_MAX;
            for(size_t i=0;i<n;i++) {
                bool selected=queue_level==0?manager->jobs[i].priority<=0:
                              queue_level==1?manager->jobs[i].priority==1:
                              manager->jobs[i].priority>=2;
                if(selected&&!done[i]&&!enqueued_flag[i]&&manager->jobs[i].arrival_time<next)
                    next=manager->jobs[i].arrival_time;
            }
            if(next==UINT32_MAX)break;
            *clock=next;
            continue;
        }
        size_t i=queue[head];head=(head+1)%n;count--;
        unsigned int run=remaining[i]<quantum?remaining[i]:quantum;
        unsigned int start=*clock;*clock+=run;remaining[i]-=run;
        if(!append_slice(result,manager->jobs[i].id,start,*clock,error,size)) {
            free(order);free(queue);free(enqueued_flag);return false;
        }
        while(enqueued<n&&manager->jobs[order[enqueued]].arrival_time<=*clock) {
            size_t j=order[enqueued++];
            bool selected=queue_level==0?manager->jobs[j].priority<=0:
                          queue_level==1?manager->jobs[j].priority==1:
                          manager->jobs[j].priority>=2;
            if(selected&&!done[j]&&!enqueued_flag[j]) {
                queue[tail]=j;tail=(tail+1)%n;count++;enqueued_flag[j]=true;
            }
        }
        if(remaining[i]){queue[tail]=i;tail=(tail+1)%n;count++;}
        else{done[i]=true;result->completion_times[i]=*clock;completed++;}
    }
    free(order);free(queue);free(enqueued_flag);
    return completed==total;
}
static bool run_srtf(const JobManager *manager, ScheduleResult *result, unsigned int *remaining,
                     char *error, size_t size) {
    size_t completed=0; unsigned int clock=0;
    while(completed<manager->count) {
        size_t best=manager->count;
        for(size_t i=0;i<manager->count;i++) if(remaining[i] && manager->jobs[i].arrival_time<=clock) {
            if(best==manager->count || remaining[i]<remaining[best] ||
               (remaining[i]==remaining[best] && (manager->jobs[i].arrival_time<manager->jobs[best].arrival_time ||
                (manager->jobs[i].arrival_time==manager->jobs[best].arrival_time && i<best)))) best=i;
        }
        if(best==manager->count) {
            unsigned int next=UINT32_MAX;
            for(size_t i=0;i<manager->count;i++) if(remaining[i] && manager->jobs[i].arrival_time<next) next=manager->jobs[i].arrival_time;
            if(next==UINT32_MAX) break; clock=next; continue;
        }
        unsigned int start=clock; clock++;
        remaining[best]--;
        if(!append_slice(result,manager->jobs[best].id,start,clock,error,size)) return false;
        if(remaining[best]==0){result->completion_times[best]=clock;completed++;}
    }
    result->total_time=clock; return true;
}

static bool run_multilevel(const JobManager *manager, ScheduleResult *result, char *error, size_t size) {
    size_t n=manager->count;
    bool *done=calloc(n,sizeof(*done));
    unsigned int *remaining=malloc(n*sizeof(*remaining));
    if(!done||!remaining){free(done);free(remaining);set_error(error,size,"Unable to allocate multilevel state.");return false;}
    for(size_t i=0;i<n;i++) remaining[i]=manager->jobs[i].burst_time;
    unsigned int clock=0;
    bool ok=true;
    /* Strict fixed priority: exhaust each higher queue before entering the next. */
    for(int level=0;level<3&&ok;level++) {
        bool has=false;
        for(size_t i=0;i<n;i++) {
            bool selected=level==0?manager->jobs[i].priority<=0:level==1?manager->jobs[i].priority==1:manager->jobs[i].priority>=2;
            if(selected) has=true;
        }
        if(!has) continue;
        if(level<2) ok=run_round_robin_subset(manager,result,level==0?2U:4U,level,done,remaining,&clock,error,size);
        else {
            for(;;) {
                size_t best=n;
                for(size_t i=0;i<n;i++) if(!done[i]&&manager->jobs[i].priority>=2&&manager->jobs[i].arrival_time<=clock) {
                    if(best==n||manager->jobs[i].arrival_time<manager->jobs[best].arrival_time||
                       (manager->jobs[i].arrival_time==manager->jobs[best].arrival_time&&i<best)) best=i;
                }
                if(best==n) {
                    unsigned int next=UINT32_MAX;
                    for(size_t i=0;i<n;i++) if(!done[i]&&manager->jobs[i].priority>=2&&manager->jobs[i].arrival_time<next)next=manager->jobs[i].arrival_time;
                    if(next==UINT32_MAX)break;clock=next;continue;
                }
                unsigned int start=clock;clock+=remaining[best];
                if(!append_slice(result,manager->jobs[best].id,start,clock,error,size)){ok=false;break;}
                done[best]=true;remaining[best]=0;result->completion_times[best]=clock;
            }
        }
    }
    result->total_time=clock;free(done);free(remaining);return ok;
}

static bool run_round_robin(const JobManager *manager, unsigned int quantum, ScheduleResult *result,
                            char *error, size_t size) {
    if (!quantum) { set_error(error,size,"Round Robin quantum must be greater than zero."); return false; }
    size_t n=manager->count;
    size_t *order=malloc(n*sizeof(*order)),*ready=malloc(n*sizeof(*ready));
    unsigned int *remaining=malloc(n*sizeof(*remaining));
    if(!order||!ready||!remaining){free(order);free(ready);free(remaining);set_error(error,size,"Unable to allocate Round Robin queue.");return false;}
    sort_indices_by_arrival(manager,order);
    for(size_t i=0;i<n;i++)remaining[i]=manager->jobs[i].burst_time;
    size_t head=0,tail=0,count=0,enqueued=0,completed=0;unsigned int clock=0;
    while(completed<n){
        while(enqueued<n&&manager->jobs[order[enqueued]].arrival_time<=clock){ready[tail]=order[enqueued++];tail=(tail+1)%n;count++;}
        if(!count){if(enqueued<n){clock=manager->jobs[order[enqueued]].arrival_time;continue;}break;}
        size_t i=ready[head];head=(head+1)%n;count--;
        unsigned int run=remaining[i]<quantum?remaining[i]:quantum,start=clock;
        clock+=run;remaining[i]-=run;
        if(!append_slice(result,manager->jobs[i].id,start,clock,error,size)){free(order);free(ready);free(remaining);return false;}
        while(enqueued<n&&manager->jobs[order[enqueued]].arrival_time<=clock){ready[tail]=order[enqueued++];tail=(tail+1)%n;count++;}
        if(remaining[i]){ready[tail]=i;tail=(tail+1)%n;count++;}
        else{result->completion_times[i]=clock;completed++;}
    }
    result->total_time=clock;free(order);free(ready);free(remaining);return true;
}

bool scheduler_run(const JobManager *manager, SchedulingPolicy policy, unsigned int quantum,
                   ScheduleResult *result, char *error, size_t error_size) {
    if(!result){set_error(error,error_size,"Schedule result pointer is null.");return false;}
    memset(result,0,sizeof(*result));
    if(!validate_jobs(manager,error,error_size))return false;
    if(policy<SCHED_FCFS||policy>SCHED_MULTILEVEL_QUEUE){set_error(error,error_size,"Unknown scheduling policy.");return false;}
    if(policy==SCHED_ROUND_ROBIN&&!quantum){set_error(error,error_size,"Round Robin quantum must be greater than zero.");return false;}
    if(!allocate_result(manager,result,error,error_size))return false;
    bool ok=false;
    if(policy==SCHED_FCFS) ok=run_fcfs(manager,result,error,error_size);
    else if(policy==SCHED_PRIORITY||policy==SCHED_SJF) ok=run_nonpreemptive_choice(manager,result,policy,error,error_size);
    else if(policy==SCHED_ROUND_ROBIN) ok=run_round_robin(manager,quantum,result,error,error_size);
    else if(policy==SCHED_SRTF) {
        unsigned int *remaining=malloc(manager->count*sizeof(*remaining));
        if(!remaining){set_error(error,error_size,"Unable to allocate SRTF state.");}
        else{for(size_t i=0;i<manager->count;i++)remaining[i]=manager->jobs[i].burst_time;
            ok=run_srtf(manager,result,remaining,error,error_size);free(remaining);}
    } else if(policy==SCHED_MULTILEVEL_QUEUE) ok=run_multilevel(manager,result,error,error_size);
    if(!ok){schedule_result_destroy(result);return false;}
    calculate_metrics(manager,result);set_error(error,error_size,"");return true;
}

void scheduler_print_result(const JobManager *manager, SchedulingPolicy policy,const ScheduleResult *result) {
    if(!manager||!result)return;
    printf("\n=== %s Scheduling ===\n",scheduling_policy_name(policy));
    puts("Timeline:");
    if(!result->slice_count)puts("  (no execution slices)");
    for(size_t i=0;i<result->slice_count;i++)printf("  [%u - %u] %s\n",result->slices[i].start_time,result->slices[i].end_time,result->slices[i].job_id);
    puts("\nPer-job metrics:");
    puts("ID                              CT       TAT      WT");
    puts("------------------------------  -------  -------  -------");
    for(size_t i=0;i<manager->count;i++)printf("%-30s  %7u  %7u  %7u\n",manager->jobs[i].id,result->completion_times[i],result->turnaround_times[i],result->waiting_times[i]);
    printf("Average turnaround time: %.2f\n",result->average_turnaround);
    printf("Average waiting time:    %.2f\n",result->average_waiting);
    printf("Makespan:                %u\n",result->total_time);
}
