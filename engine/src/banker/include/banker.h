#ifndef CAMPUSCOMPUTE_BANKER_H
#define CAMPUSCOMPUTE_BANKER_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BANKER_MAX_PROCESSES 64
#define BANKER_MAX_RESOURCES 32

typedef struct {
    size_t process_count;
    size_t resource_count;
    uint32_t available[BANKER_MAX_RESOURCES];
    uint32_t allocation[BANKER_MAX_PROCESSES][BANKER_MAX_RESOURCES];
    uint32_t maximum[BANKER_MAX_PROCESSES][BANKER_MAX_RESOURCES];
    uint32_t need[BANKER_MAX_PROCESSES][BANKER_MAX_RESOURCES];
} BankerState;

typedef struct {
    bool safe;
    size_t sequence_length;
    size_t safe_sequence[BANKER_MAX_PROCESSES];
    uint32_t final_work[BANKER_MAX_RESOURCES];
} BankerResult;

/* Validates dimensions and Allocation <= Maximum, then derives Need. */
bool banker_validate_and_calculate(BankerState *state, char *error, size_t error_size);
/* Computes the safety sequence; the input state is not changed. */
bool banker_check_safety(const BankerState *state, BankerResult *result,
                         char *error, size_t error_size);
void banker_print_state(const BankerState *state);
void banker_print_result(const BankerState *state, const BankerResult *result);
void banker_print_resource_graph(const BankerState *state);
#endif
