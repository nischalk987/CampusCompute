#define _POSIX_C_SOURCE 200809L

#include "resource_limits.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEMO_FD_LIMIT 32
#define MAX_OPENED_FDS 64

typedef struct {
    ResourceLimitResult result;
    int error_number;
} ChildReport;

/* Write the complete child report, handling interrupted writes. */
static bool write_report(int fd, const ChildReport *report) {
    const char *data = (const char *)report;
    size_t remaining = sizeof(*report);

    while (remaining > 0) {
        ssize_t written = write(fd, data, remaining);

        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }

        if (written == 0) {
            return false;
        }

        data += written;
        remaining -= (size_t)written;
    }

    return true;
}

/* Close a descriptor and record whether cleanup succeeded. */
static bool close_descriptor(int fd) {
    if (close(fd) == 0) {
        return true;
    }

    return false;
}

/* Runs only in the isolated child process. */
static void run_child(int report_fd) {
    ChildReport report = {0};
    int descriptors[MAX_OPENED_FDS];
    size_t opened = 0;
    struct rlimit limits;

    if (getrlimit(RLIMIT_NOFILE, &limits) == -1) {
        report.error_number = errno;
        (void)write_report(report_fd, &report);
        (void)close_descriptor(report_fd);
        _exit(EXIT_FAILURE);
    }

    report.result.original_soft_limit = limits.rlim_cur;
    report.result.original_hard_limit = limits.rlim_max;

    rlim_t target = DEMO_FD_LIMIT;

    if (target > limits.rlim_max) {
        target = limits.rlim_max;
    }

    if (target < 4) {
        report.error_number = EINVAL;
        (void)write_report(report_fd, &report);
        (void)close_descriptor(report_fd);
        _exit(EXIT_FAILURE);
    }

    limits.rlim_cur = target;

    if (setrlimit(RLIMIT_NOFILE, &limits) == -1) {
        report.error_number = errno;
        (void)write_report(report_fd, &report);
        (void)close_descriptor(report_fd);
        _exit(EXIT_FAILURE);
    }

    report.result.limit_applied = true;
    report.result.applied_soft_limit = target;

    /* Open files until the configured descriptor limit is reached. */
    int exhaustion_error = 0;

    while (opened < MAX_OPENED_FDS) {
        int fd = open("/dev/null", O_RDONLY);

        if (fd == -1) {
            exhaustion_error = errno;
            break;
        }

        descriptors[opened++] = fd;
    }

    report.result.descriptors_opened = opened;
    report.result.exhaustion_error = exhaustion_error;

    /* Close every descriptor opened by this demonstration. */
    bool cleanup_ok = true;

    while (opened > 0) {
        int fd = descriptors[--opened];

        if (close_descriptor(fd)) {
            report.result.descriptors_closed++;
        } else {
            cleanup_ok = false;
        }
    }

    report.result.cleanup_successful =
        cleanup_ok &&
        report.result.descriptors_closed ==
            report.result.descriptors_opened;

    if (exhaustion_error != EMFILE) {
        report.error_number =
            exhaustion_error != 0 ? exhaustion_error : EIO;
    } else if (!report.result.cleanup_successful) {
        report.error_number = EIO;
    }

    bool report_ok = write_report(report_fd, &report);
    bool close_ok = close_descriptor(report_fd);

    if (!report_ok || !close_ok) {
        _exit(EXIT_FAILURE);
    }

    _exit(report.error_number == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* Parent process: creates the child, receives its report and reaps it. */
bool resource_limits_run_demo(
    ResourceLimitResult *result,
    char *error,
    size_t error_size
) {
    if (result == NULL) {
        if (error != NULL && error_size > 0) {
            (void)snprintf(error, error_size, "Result pointer is null.");
        }
        return false;
    }

    memset(result, 0, sizeof(*result));

    if (error != NULL && error_size > 0) {
        error[0] = '\0';
    }

    int pipe_fd[2];

    if (pipe(pipe_fd) == -1) {
        int saved_errno = errno;

        if (error != NULL && error_size > 0) {
            (void)snprintf(error, error_size,
                           "pipe failed: %s", strerror(saved_errno));
        }

        return false;
    }

    pid_t pid = fork();

    if (pid == -1) {
        int saved_errno = errno;
        bool read_close_ok = close_descriptor(pipe_fd[0]);
        bool write_close_ok = close_descriptor(pipe_fd[1]);

        if (error != NULL && error_size > 0) {
            (void)snprintf(
                error, error_size,
                "fork failed: %s; pipe cleanup: %s",
                strerror(saved_errno),
                read_close_ok && write_close_ok ? "successful" : "failed"
            );
        }

        return false;
    }

    if (pid == 0) {
        bool close_ok = close_descriptor(pipe_fd[0]);

        if (!close_ok) {
            _exit(EXIT_FAILURE);
        }

        run_child(pipe_fd[1]);
    }

    /* Parent must close its copy of the child's write end. */
    bool write_close_ok = close_descriptor(pipe_fd[1]);

    ChildReport report = {0};
    char *data = (char *)&report;
    size_t remaining = sizeof(report);
    bool read_ok = true;
    int read_errno = 0;

    while (remaining > 0) {
        ssize_t received = read(pipe_fd[0], data, remaining);

        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }

            read_errno = errno;
            read_ok = false;
            break;
        }

        if (received == 0) {
            read_ok = false;
            read_errno = EIO;
            break;
        }

        data += received;
        remaining -= (size_t)received;
    }

    bool read_close_ok = close_descriptor(pipe_fd[0]);

    /* Always attempt to reap the child, even if reading failed. */
    int status = 0;
    pid_t waited;

    do {
        waited = waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited == -1) {
        int saved_errno = errno;

        if (error != NULL && error_size > 0) {
            (void)snprintf(error, error_size,
                           "waitpid failed: %s", strerror(saved_errno));
        }

        return false;
    }

    if (!write_close_ok || !read_close_ok) {
        if (error != NULL && error_size > 0) {
            (void)snprintf(error, error_size,
                           "Parent pipe descriptor cleanup failed.");
        }

        return false;
    }

    if (!read_ok) {
        if (error != NULL && error_size > 0) {
            (void)snprintf(error, error_size,
                           "Child report read failed: %s",
                           strerror(read_errno));
        }

        return false;
    }

    *result = report.result;

    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS ||
        report.error_number != 0) {
        int failure = report.error_number != 0
                          ? report.error_number
                          : ECHILD;

        if (error != NULL && error_size > 0) {
            (void)snprintf(error, error_size,
                           "Resource-limit demo failed: %s",
                           strerror(failure));
        }

        return false;
    }

    return true;
}
