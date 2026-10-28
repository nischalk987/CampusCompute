#include "banker.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 256
static bool read_line(const char *prompt,char *out,size_t size) {
    fputs(prompt,stdout); fflush(stdout);
    if(!fgets(out,(int)size,stdin)) return false;
    size_t n=strlen(out);
    if(n && out[n-1]=='\n') { out[n-1]='\0'; return true; }
    int c; while((c=getchar())!='\n' && c!=EOF) {}
    fprintf(stderr,"Input too long; try again.\n"); out[0]='\0'; return true;
}
static bool parse_u32(const char *text,uint32_t *value) {
    if(!text || !*text || *text=='-' || *text=='+') return false;
    errno=0; char *end=NULL; unsigned long long n=strtoull(text,&end,10);
    if(errno || end==text || *end!='\0' || n>UINT32_MAX) return false;
    *value=(uint32_t)n; return true;
}
static bool read_u32(const char *prompt,uint32_t *value) {
    char line[LINE_SIZE];
    if(!read_line(prompt,line,sizeof(line))) return false;
    if(!parse_u32(line,value)) { fprintf(stderr,"Invalid value: enter an unsigned integer from 0 to %u.\n",UINT32_MAX); return false; }
    return true;
}
static bool read_dimension(const char *prompt,size_t maximum,size_t *value) {
    uint32_t n;
    if(!read_u32(prompt,&n)) return false;
    if(n<1 || n>maximum) { fprintf(stderr,"Value must be between 1 and %zu.\n",maximum); return false; }
    *value=(size_t)n; return true;
}
static bool read_state(BankerState *s) {
    memset(s,0,sizeof(*s));
    if(!read_dimension("Number of processes (1-64): ",BANKER_MAX_PROCESSES,&s->process_count) ||
       !read_dimension("Number of resource types (1-32): ",BANKER_MAX_RESOURCES,&s->resource_count))
        return false;
    puts("\nEnter Available vector:");
    for(size_t r=0;r<s->resource_count;++r) {
        char prompt[80]; (void)snprintf(prompt,sizeof(prompt),"Available R%zu: ",r);
        if(!read_u32(prompt,&s->available[r])) return false;
    }
    puts("\nEnter Allocation matrix row by row:");
    for(size_t p=0;p<s->process_count;++p) for(size_t r=0;r<s->resource_count;++r) {
        char prompt[80]; (void)snprintf(prompt,sizeof(prompt),"Allocation P%zu R%zu: ",p,r);
        if(!read_u32(prompt,&s->allocation[p][r])) return false;
    }
    puts("\nEnter Maximum matrix row by row:");
    for(size_t p=0;p<s->process_count;++p) for(size_t r=0;r<s->resource_count;++r) {
        char prompt[80]; (void)snprintf(prompt,sizeof(prompt),"Maximum P%zu R%zu: ",p,r);
        if(!read_u32(prompt,&s->maximum[p][r])) return false;
    }
    return true;
}
static void print_help(const char *p) {
    printf("CampusCompute Banker's Algorithm (FR5)\nUsage: %s [--help]\n",p);
    puts("Interactive mode accepts Available, Allocation and Maximum, derives Need, then checks for a safe sequence.");
}
int main(int argc,char **argv) {
    if(argc>2 || (argc==2 && strcmp(argv[1],"--help") && strcmp(argv[1],"-h"))) {
        print_help(argv[0]); return EXIT_FAILURE;
    }
    if(argc==2) { print_help(argv[0]); return EXIT_SUCCESS; }
    for(;;) {
        puts("\n=== CampusCompute | Banker's Algorithm (FR5) ===");
        puts("1. Check a resource state\n0. Exit");
        char choice[LINE_SIZE];
        if(!read_line("Choice: ",choice,sizeof(choice))) break;
        if(!strcmp(choice,"0")) break;
        if(strcmp(choice,"1")) { fprintf(stderr,"Invalid choice. Enter 1 or 0.\n"); continue; }
        BankerState state; BankerResult result; char error[256];
        if(!read_state(&state)) { fprintf(stderr,"Input cancelled or invalid; no result was produced.\n"); continue; }
        if(!banker_validate_and_calculate(&state,error,sizeof(error))) {
            fprintf(stderr,"Invalid resource state: %s\n",error); continue;
        }
        if(!banker_check_safety(&state,&result,error,sizeof(error))) {
            fprintf(stderr,"Safety check failed: %s\n",error); continue;
        }
        banker_print_state(&state);
        banker_print_result(&state,&result);
    }
    puts("Banker's algorithm exited cleanly.");
    return EXIT_SUCCESS;
}
