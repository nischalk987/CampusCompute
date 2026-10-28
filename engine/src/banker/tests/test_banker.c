#include "banker.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static BankerState safe_case(void) {
    BankerState s={0}; s.process_count=5; s.resource_count=3;
    s.available[0]=3; s.available[1]=3; s.available[2]=2;
    const uint32_t allocation[5][3]={{0,1,0},{2,0,0},{3,0,2},{2,1,1},{0,0,2}};
    const uint32_t maximum[5][3]={{7,5,3},{3,2,2},{9,0,2},{2,2,2},{4,3,3}};
    memcpy(s.allocation,allocation,sizeof(allocation));
    memcpy(s.maximum,maximum,sizeof(maximum));
    return s;
}
static void test_safe_state(void) {
    BankerState s=safe_case(); BankerResult r; char e[256];
    assert(banker_validate_and_calculate(&s,e,sizeof(e)));
    assert(s.need[0][0]==7 && s.need[0][1]==4 && s.need[0][2]==3);
    assert(s.need[1][0]==1 && s.need[1][1]==2 && s.need[1][2]==2);
    assert(banker_check_safety(&s,&r,e,sizeof(e)));
    assert(r.safe && r.sequence_length==5);
    /* The classic example admits this safe sequence. */
    const size_t expected[]={1,3,4,0,2};
    assert(memcmp(r.safe_sequence,expected,sizeof(expected))==0);
}
static void test_unsafe_state(void) {
    BankerState s={0}; s.process_count=2; s.resource_count=1;
    s.available[0]=0;
    s.allocation[0][0]=1; s.maximum[0][0]=2;
    s.allocation[1][0]=1; s.maximum[1][0]=2;
    char e[256]; BankerResult r;
    assert(banker_validate_and_calculate(&s,e,sizeof(e)));
    assert(banker_check_safety(&s,&r,e,sizeof(e)));
    assert(!r.safe && r.sequence_length==0);
}
static void test_invalid_matrix(void) {
    BankerState s={0}; s.process_count=1; s.resource_count=1;
    s.allocation[0][0]=3; s.maximum[0][0]=2;
    char e[256];
    assert(!banker_validate_and_calculate(&s,e,sizeof(e)));
    assert(strstr(e,"exceeds Maximum")!=NULL);
}
static void test_dimensions(void) {
    BankerState s={0}; char e[256];
    s.process_count=BANKER_MAX_PROCESSES+1; s.resource_count=1;
    assert(!banker_validate_and_calculate(&s,e,sizeof(e)));
    s.process_count=1; s.resource_count=BANKER_MAX_RESOURCES+1;
    assert(!banker_validate_and_calculate(&s,e,sizeof(e)));
}
int main(void) {
    test_safe_state(); test_unsafe_state(); test_invalid_matrix(); test_dimensions();
    puts("FR5 Banker's Algorithm tests passed."); return 0;
}
