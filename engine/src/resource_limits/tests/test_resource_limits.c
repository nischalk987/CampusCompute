#include "resource_limits.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/resource.h>

static void test_resource_limit_demo(void) {
    struct rlimit before, after;
    ResourceLimitResult result;
    char error[256];

    assert(getrlimit(RLIMIT_NOFILE, &before) == 0);

    assert(resource_limits_run_demo(
        &result, error, sizeof(error)));

    assert(getrlimit(RLIMIT_NOFILE, &after) == 0);

    assert(result.limit_applied);
    assert(result.applied_soft_limit <= 32);
    assert(result.exhaustion_error == EMFILE);

    assert(result.descriptors_opened > 0);
    assert(result.descriptors_closed == result.descriptors_opened);
    assert(result.cleanup_successful);

    /* The parent's resource limit must remain unchanged. */
    assert(before.rlim_cur == after.rlim_cur);
    assert(before.rlim_max == after.rlim_max);

    puts("PASS: Resource limit applied in child.");
    puts("PASS: File descriptor exhaustion detected.");
    puts("PASS: All opened descriptors cleaned up.");
    puts("PASS: Parent resource limits unchanged.");
}

static void test_null_result(void) {
    char error[256];

    assert(!resource_limits_run_demo(NULL, error, sizeof(error)));
    puts("PASS: Null result pointer handled.");
}

int main(void) {
    test_resource_limit_demo();
    test_null_result();

    puts("\nFR7 resource-limit tests passed.");
    return 0;
}
