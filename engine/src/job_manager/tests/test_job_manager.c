#include "job_manager.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void bad_add(JobManager *m,const char *id,long a,long b,long p) {
    char error[256]; size_t old=m->count;
    assert(!job_manager_add(m,id,a,b,p,error,sizeof(error)));
    assert(error[0] && m->count==old);
}
int main(void) {
    JobManager m; char err[256]; job_manager_init(&m);
    assert(job_manager_add(&m,"job_1",0,5,2,err,sizeof(err)));
    assert(job_manager_add(&m,"J-2",3,4,1,err,sizeof(err)));
    assert(m.count==2 && job_manager_find(&m,"job_1"));
    bad_add(&m,"job_1",0,5,2); bad_add(&m,"",0,1,0); bad_add(&m,"bad id",0,1,0);
    bad_add(&m,"negative",-1,1,0); bad_add(&m,"zero-burst",0,0,0);
    bad_add(&m,"negative-burst",0,-2,0); bad_add(&m,"priority-high",0,1,1000001);
    assert(job_manager_mark_ready(&m,"job_1",err,sizeof(err)));
    assert(job_manager_find(&m,"job_1")->status==JOB_READY);
    assert(!job_manager_mark_running(&m,"J-2",err,sizeof(err)));
    assert(job_manager_mark_running(&m,"job_1",err,sizeof(err)));
    assert(!job_manager_mark_completed(&m,"job_1",4,err,sizeof(err)));
    assert(job_manager_find(&m,"job_1")->status==JOB_RUNNING);
    assert(job_manager_mark_completed(&m,"job_1",5,err,sizeof(err)));
    const Job *j=job_manager_find_const(&m,"job_1");
    assert(j && j->status==JOB_COMPLETED && j->remaining_time==0 && j->completion_time==5);
    assert(!job_manager_mark_ready(&m,"job_1",err,sizeof(err)));
    job_manager_destroy(&m); assert(!m.jobs && !m.count && !m.capacity);
    puts("FR1 job-manager tests passed."); return 0;
}
