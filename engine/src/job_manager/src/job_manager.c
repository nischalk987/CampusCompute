#define _POSIX_C_SOURCE 200809L
#include "job_manager.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INITIAL_CAPACITY 4
#define MAX_JOB_COUNT 10000U
#define MAX_TIME_VALUE 1000000000L
#define PRIORITY_MIN (-1000000L)
#define PRIORITY_MAX 1000000L

static void set_error(char *out, size_t size, const char *message) {
    if (out && size) (void)snprintf(out, size, "%s", message);
}
static bool valid_id(const char *id) {
    if (!id) return false;
    size_t n = strlen(id);
    if (n == 0 || n > JOB_ID_MAX) return false;
    for (size_t i=0; i<n; ++i) {
        unsigned char c=(unsigned char)id[i];
        if (!(isalnum(c) || c=='_' || c=='-')) return false;
    }
    return true;
}
static bool reserve_one(JobManager *m) {
    if (m->count < m->capacity) return true;
    if (m->capacity >= MAX_JOB_COUNT) return false;
    size_t cap = m->capacity ? m->capacity * 2 : INITIAL_CAPACITY;
    if (cap < m->capacity || cap > MAX_JOB_COUNT) cap = MAX_JOB_COUNT;
    if (cap > SIZE_MAX / sizeof(*m->jobs)) return false;
    Job *p = realloc(m->jobs, cap * sizeof(*m->jobs));
    if (!p) return false;
    m->jobs=p; m->capacity=cap; return true;
}
void job_manager_init(JobManager *m) {
    if (!m) return; m->jobs=NULL; m->count=0; m->capacity=0;
}
void job_manager_destroy(JobManager *m) {
    if (!m) return; free(m->jobs); m->jobs=NULL; m->count=0; m->capacity=0;
}
Job *job_manager_find(JobManager *m, const char *id) {
    if (!m || !id) return NULL;
    for (size_t i=0;i<m->count;++i) if (!strcmp(m->jobs[i].id,id)) return &m->jobs[i];
    return NULL;
}
const Job *job_manager_find_const(const JobManager *m, const char *id) {
    if (!m || !id) return NULL;
    for (size_t i=0;i<m->count;++i) if (!strcmp(m->jobs[i].id,id)) return &m->jobs[i];
    return NULL;
}
bool job_manager_add(JobManager *m,const char *id,long arrival,long burst,long priority,
                     char *error,size_t error_size) {
    if (!m) { set_error(error,error_size,"Job manager is not initialized."); return false; }
    if (!valid_id(id)) { set_error(error,error_size,"Job ID must be 1-31 characters: letters, digits, '_' or '-'."); return false; }
    if (job_manager_find_const(m,id)) { set_error(error,error_size,"A job with this ID already exists."); return false; }
    if (arrival<0 || arrival>MAX_TIME_VALUE) { set_error(error,error_size,"Arrival time must be between 0 and 1000000000."); return false; }
    if (burst<=0 || burst>MAX_TIME_VALUE) { set_error(error,error_size,"CPU burst must be between 1 and 1000000000."); return false; }
    if (priority<PRIORITY_MIN || priority>PRIORITY_MAX) { set_error(error,error_size,"Priority must be between -1000000 and 1000000."); return false; }
    if (m->count>=MAX_JOB_COUNT) { set_error(error,error_size,"Job limit reached (10000 jobs)."); return false; }
    if (!reserve_one(m)) { set_error(error,error_size,"Unable to allocate memory for another job."); return false; }
    Job j={0};
    (void)snprintf(j.id,sizeof(j.id),"%s",id);
    j.arrival_time=(unsigned int)arrival; j.burst_time=(unsigned int)burst;
    j.remaining_time=j.burst_time; j.priority=(int)priority; j.status=JOB_CREATED;
    m->jobs[m->count++]=j; set_error(error,error_size,""); return true;
}
static bool transition(JobManager *m,const char *id,JobStatus target,char *error,size_t error_size) {
    Job *j=job_manager_find(m,id);
    if (!j) { set_error(error,error_size,"Job ID was not found."); return false; }
    bool allowed=(target==JOB_READY && j->status==JOB_CREATED) ||
                 (target==JOB_RUNNING && j->status==JOB_READY) ||
                 (target==JOB_COMPLETED && j->status==JOB_RUNNING);
    if (!allowed) { set_error(error,error_size,"Invalid job state transition."); return false; }
    j->status=target;
    if (target==JOB_RUNNING) j->remaining_time=j->burst_time;
    if (target==JOB_COMPLETED) j->remaining_time=0;
    set_error(error,error_size,""); return true;
}
bool job_manager_mark_ready(JobManager *m,const char *id,char *e,size_t n) {
    return transition(m,id,JOB_READY,e,n);
}
bool job_manager_mark_running(JobManager *m,const char *id,char *e,size_t n) {
    return transition(m,id,JOB_RUNNING,e,n);
}
bool job_manager_mark_completed(JobManager *m,const char *id,unsigned int completion,char *e,size_t n) {
    Job *j=job_manager_find(m,id);
    if (!j) { set_error(e,n,"Job ID was not found."); return false; }
    if (j->status!=JOB_RUNNING) { set_error(e,n,"Only a running job can be completed."); return false; }
    if ((unsigned long long)completion < (unsigned long long)j->arrival_time+j->burst_time) {
        set_error(e,n,"Completion time cannot be earlier than arrival time plus CPU burst."); return false;
    }
    j->completion_time=completion;
    return transition(m,id,JOB_COMPLETED,e,n);
}
const char *job_status_name(JobStatus s) {
    switch(s) { case JOB_CREATED:return "CREATED"; case JOB_READY:return "READY";
        case JOB_RUNNING:return "RUNNING"; case JOB_COMPLETED:return "COMPLETED"; default:return "UNKNOWN"; }
}
void job_manager_list(const JobManager *m) {
    if (!m || !m->count) { puts("No jobs registered."); return; }
    puts("ID                              ARRIVAL  BURST  PRIORITY  REMAINING  COMPLETION  STATUS");
    puts("------------------------------  -------  -----  --------  ---------  ----------  --------");
    for(size_t i=0;i<m->count;++i) {
        const Job *j=&m->jobs[i];
        printf("%-30s  %7u  %5u  %8d  %9u  %10u  %s\n",j->id,j->arrival_time,j->burst_time,j->priority,j->remaining_time,
               j->status==JOB_COMPLETED?j->completion_time:0U,job_status_name(j->status));
    }
}
