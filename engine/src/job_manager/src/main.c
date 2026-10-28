#include "job_manager.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INPUT_SIZE 256
static bool read_line(const char *prompt,char *out,size_t size) {
    fputs(prompt,stdout); fflush(stdout);
    if (!fgets(out,(int)size,stdin)) return false;
    size_t n=strlen(out);
    if(n && out[n-1]=='\n') { out[n-1]='\0'; return true; }
    int c; while((c=getchar())!='\n' && c!=EOF) {}
    fprintf(stderr,"Input too long; please try again.\n"); out[0]='\0'; return true;
}
static bool parse_long_value(const char *s,long *out) {
    char *end=NULL; errno=0; long v=strtol(s,&end,10);
    if(errno || end==s) return false;
    while(*end==' '||*end=='\t'||*end=='\r') ++end;
    if(*end) return false; *out=v; return true;
}
static bool read_number(const char *prompt,long *out) {
    char buf[INPUT_SIZE]; if(!read_line(prompt,buf,sizeof(buf))) return false;
    if(!parse_long_value(buf,out)) { fprintf(stderr,"Invalid number. Enter a whole number without extra characters.\n"); return false; }
    return true;
}
static void add_job(JobManager *m) {
    char id[INPUT_SIZE],err[256]; long a,b,p;
    if(!read_line("Job ID (letters/digits/_/-): ",id,sizeof(id))) return;
    if(!read_number("Arrival time (>= 0): ",&a)) return;
    if(!read_number("CPU burst (> 0): ",&b)) return;
    if(!read_number("Priority (-1000000..1000000): ",&p)) return;
    if(!job_manager_add(m,id,a,b,p,err,sizeof(err))) { fprintf(stderr,"Cannot add job: %s\n",err); return; }
    printf("Job '%s' added with status CREATED.\n",id);
}
static void change_status(JobManager *m,int action) {
    char id[INPUT_SIZE],err[256]; if(!read_line("Job ID: ",id,sizeof(id))) return;
    bool ok=false;
    if(action==1) ok=job_manager_mark_ready(m,id,err,sizeof(err));
    else if(action==2) ok=job_manager_mark_running(m,id,err,sizeof(err));
    else {
        long done;
        if(!read_number("Completion time: ",&done)) return;
        if(done<0 || (unsigned long long)done>UINT_MAX) { fprintf(stderr,"Completion time must be between 0 and %u.\n",UINT_MAX); return; }
        ok=job_manager_mark_completed(m,id,(unsigned int)done,err,sizeof(err));
    }
    if(!ok) fprintf(stderr,"State update failed: %s\n",err);
    else printf("Job '%s' is now %s.\n",id,job_status_name(job_manager_find(m,id)->status));
}
static void usage(const char *p) {
    printf("CampusCompute Job Manager (FR1)\nUsage: %s [--help]\n",p);
    puts("Interactive menu: add/list jobs and change lifecycle states.");
}
int main(int argc,char **argv) {
    if(argc>2 || (argc==2 && strcmp(argv[1],"--help") && strcmp(argv[1],"-h"))) { usage(argv[0]); return EXIT_FAILURE; }
    if(argc==2) { usage(argv[0]); return EXIT_SUCCESS; }
    JobManager m; job_manager_init(&m); char choice[INPUT_SIZE];
    for(;;) {
        puts("\n=== CampusCompute | Job Manager (FR1) ===");
        puts("1. Add job\n2. List jobs\n3. Mark job READY\n4. Mark job RUNNING\n5. Mark job COMPLETED\n0. Exit");
        if(!read_line("Choice: ",choice,sizeof(choice))) break;
        if(!strcmp(choice,"0")) break;
        if(!strcmp(choice,"1")) add_job(&m);
        else if(!strcmp(choice,"2")) job_manager_list(&m);
        else if(!strcmp(choice,"3")) change_status(&m,1);
        else if(!strcmp(choice,"4")) change_status(&m,2);
        else if(!strcmp(choice,"5")) change_status(&m,3);
        else fprintf(stderr,"Unknown menu choice. Enter 0, 1, 2, 3, 4, or 5.\n");
    }
    job_manager_destroy(&m); puts("Job manager exited cleanly."); return EXIT_SUCCESS;
}
