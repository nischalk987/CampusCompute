#define _POSIX_C_SOURCE 200809L

#include "process_manager.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define SIMULATED_FAILURE_EXIT 42

static void set_error(char *error, size_t size, const char *message)
{
    if (error && size) {
        (void)snprintf(error, size, "%s", message);
    }
}

bool process_manager_run(bool simulate_failure,
                         ProcessResult *result,
                         char *error,
                         size_t error_size)
{
    if (result == NULL) {
        set_error(error, error_size, "Result pointer is NULL.");
        return false;
    }

    memset(result, 0, sizeof(*result));

    pid_t pid = fork();

    if (pid < 0) {
        set_error(error, error_size, "fork() failed.");
        return false;
    }

    if (pid == 0) {
        if (simulate_failure) {
            _exit(SIMULATED_FAILURE_EXIT);
        }

        _exit(EXIT_SUCCESS);
    }

    result->child_pid = pid;

    int status;
    pid_t waited;

    do {
        waited = waitpid(pid, &status, 0);
    } while (waited < 0 && errno == EINTR);

    if (waited < 0) {
        set_error(error, error_size, "waitpid() failed.");
        return false;
    }

    if (WIFEXITED(status)) {
        result->exited_normally = true;
        result->exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result->terminated_by_signal = true;
        result->signal_number = WTERMSIG(status);
    }

    set_error(error, error_size, "");
    return true;
}
