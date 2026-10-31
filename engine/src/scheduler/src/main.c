#include "job_manager.h"
#include "scheduler.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INPUT_SIZE 256

static bool read_line(const char *prompt,char *out,size_t size){fputs(prompt,stdout);fflush(stdout);if(!fgets(out,(int)size,stdin))return false;size_t n=strlen(out);if(n&&out[n-1]=='\n'){out[n-1]='\0';return true;}int ch;while((ch=getchar())!='\n'&&ch!=EOF){}fprintf(stderr,"Input too long; try again.\n");out[0]='\0';return true;}
static bool read_long(const char *prompt,long *out){char b[INPUT_SIZE];for(;;){if(!read_line(prompt,b,sizeof(b)))return false;char *end=NULL;errno=0;long v=strtol(b,&end,10);while(*end==' '||*end=='\t'||*end=='\r')end++;if(errno==0&&end!=b&&*end=='\0'){*out=v;return true;}puts("Enter a whole number.");}}
static bool load_jobs(JobManager *m){long n;if(!read_long("Number of jobs (1-10000): ",&n)||n<1||n>10000)return false;char error[256];for(long i=0;i<n;i++){char id[INPUT_SIZE];long a,b,p;printf("\nJob %ld of %ld\n",i+1,n);if(!read_line("ID: ",id,sizeof(id))||!read_long("Arrival time (>= 0): ",&a)||!read_long("CPU burst (> 0): ",&b)||!read_long("Priority / queue class (0=foreground, 1=interactive, 2+=batch): ",&p))return false;if(!job_manager_add(m,id,a,b,p,error,sizeof(error))){fprintf(stderr,"Cannot add job: %s\n",error);return false;}}return true;}
static bool run_one(const JobManager *m,SchedulingPolicy p,unsigned int q){ScheduleResult r;char e[256];if(!scheduler_run(m,p,q,&r,e,sizeof(e))){fprintf(stderr,"Scheduling failed: %s\n",e);return false;}scheduler_print_result(m,p,&r);schedule_result_destroy(&r);return true;}
int main(void){JobManager m;job_manager_init(&m);bool loaded=false;char choice[INPUT_SIZE];for(;;){puts("\n=== CampusCompute | CPU Scheduler (FR2) ===");puts("1. Enter/replace jobs\n2. Run FCFS\n3. Run Round Robin\n4. Run non-preemptive Priority\n5. Run SJF (non-preemptive)\n6. Run SRTF (preemptive)\n7. Run Multilevel Queue\n8. Compare all six policies\n9. List current jobs\n0. Exit");if(!read_line("Choice: ",choice,sizeof(choice)))break;if(!strcmp(choice,"0"))break;if(!strcmp(choice,"1")){job_manager_destroy(&m);job_manager_init(&m);loaded=load_jobs(&m);if(!loaded){job_manager_destroy(&m);job_manager_init(&m);}continue;}if(!loaded){puts("Enter a valid job set first.");continue;}if(!strcmp(choice,"9")){job_manager_list(&m);continue;}
SchedulingPolicy policies[]={SCHED_FCFS,SCHED_ROUND_ROBIN,SCHED_PRIORITY,SCHED_SJF,SCHED_SRTF,SCHED_MULTILEVEL_QUEUE};
if(!strcmp(choice,"2"))run_one(&m,policies[0],1);
else if(!strcmp(choice,"3")){long q;if(read_long("Round Robin quantum: ",&q)&&q>0&&q<=UINT_MAX)run_one(&m,policies[1],(unsigned int)q);else puts("Quantum must be a positive integer.");}
else if(!strcmp(choice,"4"))run_one(&m,policies[2],1);
else if(!strcmp(choice,"5"))run_one(&m,policies[3],1);
else if(!strcmp(choice,"6"))run_one(&m,policies[4],1);
else if(!strcmp(choice,"7"))run_one(&m,policies[5],1);
else if(!strcmp(choice,"8")){long q;if(!read_long("Round Robin quantum for comparison: ",&q)||q<=0||q>UINT_MAX){puts("Quantum must be a positive integer.");continue;}for(size_t i=0;i<6;i++)run_one(&m,policies[i],i==1?(unsigned int)q:1);}
else puts("Choose an option from 0 to 9.");}job_manager_destroy(&m);puts("Scheduler closed.");return EXIT_SUCCESS;}
