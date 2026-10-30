#ifndef RESOURCE_LIMITS_H
#define RESOURCE_LIMITS_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/resource.h>

typedef struct {
    rlim_t original_soft_limit;
    rlim_t original_hard_limit;
    rlim_t applied_soft_limit;

    size_t descriptors_opened;
    size_t descriptors_closed;

    int exhaustion_error;
    bool limit_applied;
    bool cleanup_successful;
} ResourceLimitResult;

/*
 * Demonstrates RLIMIT_NOFILE in an isolated child process.
 * The parent process's resource limits remain unchanged.
 */
bool resource_limits_run_demo(
    ResourceLimitResult *result,
    char *error,
    size_t error_size
);

#endif
