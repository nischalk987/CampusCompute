#include "banker.h"
#include <stdio.h>
#include <string.h>

static void set_error(char *out,size_t size,const char *message) {
    if(out && size) (void)snprintf(out,size,"%s",message);
}

bool banker_validate_and_calculate(BankerState *s,char *error,size_t error_size) {
    if(!s) { set_error(error,error_size,"Banker state pointer is null."); return false; }
    if(s->process_count==0 || s->process_count>BANKER_MAX_PROCESSES) {
        set_error(error,error_size,"Process count must be between 1 and 64."); return false;
    }
    if(s->resource_count==0 || s->resource_count>BANKER_MAX_RESOURCES) {
        set_error(error,error_size,"Resource count must be between 1 and 32."); return false;
    }
    for(size_t p=0;p<s->process_count;++p) {
        for(size_t r=0;r<s->resource_count;++r) {
            if(s->allocation[p][r]>s->maximum[p][r]) {
                (void)snprintf(error,error_size,
                    "Invalid matrix: Allocation[P%zu][R%zu] (%u) exceeds Maximum (%u).",
                    p,r,s->allocation[p][r],s->maximum[p][r]);
                return false;
            }
        }
    }
    for(size_t p=0;p<s->process_count;++p)
        for(size_t r=0;r<s->resource_count;++r)
            s->need[p][r]=s->maximum[p][r]-s->allocation[p][r];
    set_error(error,error_size,"");
    return true;
}

bool banker_check_safety(const BankerState *s,BankerResult *result,
                         char *error,size_t error_size) {
    if(!s || !result) { set_error(error,error_size,"Banker state/result pointer is null."); return false; }
    if(s->process_count==0 || s->process_count>BANKER_MAX_PROCESSES ||
       s->resource_count==0 || s->resource_count>BANKER_MAX_RESOURCES) {
        set_error(error,error_size,"Invalid Banker state dimensions."); return false;
    }
    memset(result,0,sizeof(*result));
    bool finished[BANKER_MAX_PROCESSES]={false};
    uint32_t work[BANKER_MAX_RESOURCES]={0};
    for(size_t r=0;r<s->resource_count;++r) work[r]=s->available[r];

    size_t completed=0;
    while(completed<s->process_count) {
        bool progress=false;
        for(size_t p=0;p<s->process_count;++p) {
            if(finished[p]) continue;
            bool can_finish=true;
            for(size_t r=0;r<s->resource_count;++r)
                if(s->need[p][r]>work[r]) { can_finish=false; break; }
            if(!can_finish) continue;

            for(size_t r=0;r<s->resource_count;++r) {
                /* Work + Allocation must stay within uint32_t. */
                if(UINT32_MAX-work[r]<s->allocation[p][r]) {
                    set_error(error,error_size,"Resource-count overflow while releasing allocation.");
                    return false;
                }
                work[r]+=s->allocation[p][r];
            }
            finished[p]=true;
            result->safe_sequence[completed++]=p;
            progress=true;
        }
        if(!progress) break;
    }
    result->safe=completed==s->process_count;
    result->sequence_length=completed;
    for(size_t r=0;r<s->resource_count;++r) result->final_work[r]=work[r];
    set_error(error,error_size,"");
    return true;
}

void banker_print_state(const BankerState *s) {
    if(!s) return;
    printf("Available: ");
    for(size_t r=0;r<s->resource_count;++r) printf("%s%u",r?" ":"",s->available[r]);
    putchar('\n');
    puts("\nAllocation matrix:");
    printf("%-8s","Process");
    for(size_t r=0;r<s->resource_count;++r) printf(" R%-6zu",r);
    putchar('\n');
    for(size_t p=0;p<s->process_count;++p) {
        printf("P%-7zu",p);
        for(size_t r=0;r<s->resource_count;++r) printf(" %-7u",s->allocation[p][r]);
        putchar('\n');
    }
    puts("\nMaximum matrix:");
    printf("%-8s","Process");
    for(size_t r=0;r<s->resource_count;++r) printf(" R%-6zu",r);
    putchar('\n');
    for(size_t p=0;p<s->process_count;++p) {
        printf("P%-7zu",p);
        for(size_t r=0;r<s->resource_count;++r) printf(" %-7u",s->maximum[p][r]);
        putchar('\n');
    }
    puts("\nNeed matrix (Maximum - Allocation):");
    printf("%-8s","Process");
    for(size_t r=0;r<s->resource_count;++r) printf(" R%-6zu",r);
    putchar('\n');
    for(size_t p=0;p<s->process_count;++p) {
        printf("P%-7zu",p);
        for(size_t r=0;r<s->resource_count;++r) printf(" %-7u",s->need[p][r]);
        putchar('\n');
    }
}

void banker_print_result(const BankerState *s,const BankerResult *r) {
    if(!s || !r) return;
    if(r->safe) {
        printf("\nState: SAFE\nSafe sequence: ");
        for(size_t i=0;i<r->sequence_length;++i)
            printf("%sP%zu",i?" -> ":"",r->safe_sequence[i]);
        putchar('\n');
    } else {
        printf("\nState: UNSAFE\nNo complete safe sequence exists. Processes that could finish first: ");
        if(r->sequence_length==0) puts("none");
        else {
            for(size_t i=0;i<r->sequence_length;++i)
                printf("%sP%zu",i?" -> ":"",r->safe_sequence[i]);
            putchar('\n');
        }
    }
}
