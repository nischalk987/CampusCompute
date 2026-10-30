#include "process_manager.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    ProcessResult result;
    char error[256];

    /* Test 1: Normal child completion */
    assert(process_manager_run(false, &result, error, sizeof(error)));
    assert(result.child_pid > 0);
    assert(result.exited_normally);
    assert(result.exit_code == 0);
    assert(!result.terminated_by_signal);

    puts("Test 1 passed: Normal child completion.");

    /* Test 2: Controlled child failure */
    assert(process_manager_run(true, &result, error, sizeof(error)));
    assert(result.child_pid > 0);
    assert(result.exited_normally);
    assert(result.exit_code == 42);
    assert(!result.terminated_by_signal);

    puts("Test 2 passed: Controlled child failure.");

    /* Test 3: Invalid result pointer */
    assert(!process_manager_run(false, NULL, error, sizeof(error)));
    assert(strcmp(error, "Result pointer is NULL.") == 0);

    puts("Test 3 passed: Invalid argument handling.");

    puts("\nAll FR6 process-manager tests passed.");

    return 0;
}
