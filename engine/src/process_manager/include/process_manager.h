#ifndef CAMPUSCOMPUTE_PROCESS_MANAGER_H
#define CAMPUSCOMPUTE_PROCESS_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

typedef struct {
    pid_t child_pid;
    bool exited_normally;
    int exit_code;
    bool terminated_by_signal;
    int signal_number;
} ProcessResult;

bool process_manager_run(bool simulate_failure,
                         ProcessResult *result,
                         char *error,
                         size_t error_size);

#endif
