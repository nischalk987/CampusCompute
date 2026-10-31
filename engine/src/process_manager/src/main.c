#include "process_manager.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc != 2 || (strcmp(argv[1], "normal") != 0 && strcmp(argv[1], "failure") != 0)) {
        fprintf(stderr, "Usage: %s normal|failure\n", argv[0]);
        return 2;
    }
    ProcessResult result;
    char error[256];
    int simulate_failure = strcmp(argv[1], "failure") == 0;
    puts("=== Worker Failure Demonstration ===");
    printf("Scenario: %s\n", simulate_failure ? "controlled child failure" : "normal child completion");
    if (!process_manager_run(simulate_failure != 0, &result, error, sizeof(error))) {
        fprintf(stderr, "Worker operation failed: %s\n", error);
        return 1;
    }
    printf("Child PID: %ld\n", (long)result.child_pid);
    if (result.exited_normally) {
        printf("Child exit code: %d\n", result.exit_code);
        printf("Parent reaped child: YES\n");
        printf("Outcome: %s\n", result.exit_code == 0 ? "SUCCESS" : "EXPECTED FAILURE OBSERVED");
    } else if (result.terminated_by_signal) {
        printf("Child terminated by signal: %d\n", result.signal_number);
        printf("Parent reaped child: YES\n");
    }
    return 0;
}
