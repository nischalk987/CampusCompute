#include "resource_limits.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        puts("CampusCompute Resource Limits Demo");
        puts("Usage: ./resource_limits_demo");
        puts("Demonstrates RLIMIT_NOFILE and safe descriptor cleanup.");
        return 0;
    }

    if (argc != 1) {
        fprintf(stderr, "Usage: %s [--help]\n", argv[0]);
        return 2;
    }

    ResourceLimitResult result;
    char error[256];

    puts("=== Resource Limits Demonstration ===");

    if (!resource_limits_run_demo(&result, error, sizeof(error))) {
        fprintf(stderr, "Demo failed: %s\n", error);
        return 1;
    }

    printf("Original soft limit : %llu\n",
           (unsigned long long)result.original_soft_limit);
    printf("Original hard limit : %llu\n",
           (unsigned long long)result.original_hard_limit);
    printf("Applied soft limit  : %llu\n",
           (unsigned long long)result.applied_soft_limit);

    printf("Descriptors opened  : %zu\n", result.descriptors_opened);
    printf("Descriptors closed  : %zu\n", result.descriptors_closed);

    printf("Exhaustion detected : %s\n",
           result.exhaustion_error == EMFILE ? "YES (EMFILE)" : "NO");

    printf("Cleanup successful : %s\n",
           result.cleanup_successful ? "YES" : "NO");

    return 0;
}
