#define _POSIX_C_SOURCE 200809L

#include "job_manager.h"
#include "scheduler.h"
#include "banker.h"
#include "process_manager.h"
#include "resource_limits.h"

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <termios.h>

#define INPUT_SIZE 256

static volatile sig_atomic_t interrupted = 0;

static bool read_line(const char *prompt, char *buffer, size_t size);


static bool authenticate_student(char *student_name, size_t name_size,
                                 char *student_srn, size_t srn_size)
{
    char srn[INPUT_SIZE];
    char password[INPUT_SIZE];
    int password_pipe[2];

    puts("\n========== STUDENT LOGIN ==========");
    if (!read_line("Enter SRN      : ", srn, sizeof(srn))) return false;
    if (srn[0] == '\0' || strlen(srn) >= srn_size) {
        puts("Invalid SRN.");
        return false;
    }

    printf("Enter Password : ");
    fflush(stdout);
    struct termios old_term, new_term;
    bool hidden = (tcgetattr(STDIN_FILENO, &old_term) == 0);
    if (hidden) {
        new_term = old_term;
        new_term.c_lflag &= (tcflag_t)~ECHO;
        (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_term);
    }
    bool read_ok = fgets(password, sizeof(password), stdin) != NULL;
    if (hidden) {
        (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_term);
        puts("");
    }
    if (!read_ok) return false;
    password[strcspn(password, "\r\n")] = '\0';
    if (password[0] == '\0') {
        puts("Password cannot be empty.");
        return false;
    }

    if (pipe(password_pipe) < 0) {
        perror("pipe");
        return false;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(password_pipe[0]); close(password_pipe[1]);
        return false;
    }
    if (pid == 0) {
        close(password_pipe[1]);
        if (dup2(password_pipe[0], STDIN_FILENO) < 0) _exit(126);
        close(password_pipe[0]);
        execlp("python3", "python3", "engine/authenticate_cli.py", srn, (char *)NULL);
        perror("python3");
        _exit(127);
    }

    close(password_pipe[0]);
    size_t length = strlen(password);
    bool sent = write(password_pipe[1], password, length) == (ssize_t)length &&
                write(password_pipe[1], "\n", 1) == 1;
    close(password_pipe[1]);
    memset(password, 0, sizeof(password));

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        perror("waitpid");
        return false;
    }
    if (!sent || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        puts("Authentication failed. Check your SRN/password and register through the web app if you do not have an account.");
        return false;
    }

    (void)snprintf(student_name, name_size, "%s", srn);
    (void)snprintf(student_srn, srn_size, "%s", srn);
    return true;
}

static void handle_signal(int signal_number)
{
    (void)signal_number;
    interrupted = 1;
}

static bool read_line(const char *prompt, char *buffer, size_t size)
{
    if (interrupted) return false;

    printf("%s", prompt);
    fflush(stdout);

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return false;
    }

    size_t length = strlen(buffer);

    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
        return true;
    }

    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) { }

    fprintf(stderr, "Input too long. Please try again.\n");
    buffer[0] = '\0';
    return true;
}

static bool read_number(const char *prompt, long *value,
                        long minimum, long maximum)
{
    char buffer[INPUT_SIZE];

    for (;;) {
        if (!read_line(prompt, buffer, sizeof(buffer))) {
            return false;
        }

        char *end = NULL;
        errno = 0;
        long parsed = strtol(buffer, &end, 10);

        while (*end == ' ' || *end == '\t' || *end == '\r') {
            end++;
        }

        if (errno == 0 && end != buffer && *end == '\0' &&
            parsed >= minimum && parsed <= maximum) {
            *value = parsed;
            return true;
        }

        fprintf(stderr, "Enter a whole number between %ld and %ld.\n",
                minimum, maximum);
    }
}

static bool read_unsigned(const char *prompt, uint32_t *value)
{
    char buffer[INPUT_SIZE];

    for (;;) {
        if (!read_line(prompt, buffer, sizeof(buffer))) {
            return false;
        }

        if (buffer[0] == '-' || buffer[0] == '+') {
            fprintf(stderr, "Enter an unsigned integer.\n");
            continue;
        }

        char *end = NULL;
        errno = 0;
        unsigned long long parsed = strtoull(buffer, &end, 10);

        if (errno == 0 && end != buffer && *end == '\0' &&
            parsed <= UINT32_MAX) {
            *value = (uint32_t)parsed;
            return true;
        }

        fprintf(stderr, "Invalid unsigned integer. Try again.\n");
    }
}

static bool read_positive_unsigned(const char *prompt,
                                   unsigned int *value)
{
    long number;

    if (!read_number(prompt, &number, 1, UINT_MAX)) {
        return false;
    }

    *value = (unsigned int)number;
    return true;
}

/* Execute a fixed program without invoking a shell. */
static bool run_child(char *const arguments[])
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return false;
    }

    if (pid == 0) {
        execv(arguments[0], arguments);
        perror("execv");
        _exit(127);
    }

    int status = 0;
    pid_t result;

    do {
        result = waitpid(pid, &status, 0);

        if (interrupted && result < 0 && errno == EINTR) {
            kill(pid, SIGTERM);
        }
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        perror("waitpid");
        return false;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return true;
    }

    if (WIFEXITED(status)) {
        fprintf(stderr, "Child process exited with code %d.\n",
                WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "Child process terminated by signal %d.\n",
                WTERMSIG(status));
    }

    return false;
}

static void job_manager_menu(void)
{
    JobManager manager;
    job_manager_init(&manager);

    for (;;) {
        puts("\n--- FR1: Job Manager ---");
        puts("1. Add job");
        puts("2. List jobs");
        puts("3. Mark READY");
        puts("4. Mark RUNNING");
        puts("5. Mark COMPLETED");
        puts("0. Back");

        long choice;
        if (!read_number("Choice: ", &choice, 0, 5)) break;
        if (choice == 0) break;

        char id[INPUT_SIZE];
        char error[256];
        bool success = false;

        if (choice == 1) {
            long arrival, burst, priority;

            if (!read_line("Job ID: ", id, sizeof(id))) break;
            if (!read_number("Arrival time: ", &arrival, 0, UINT_MAX)) continue;
            if (!read_number("CPU burst: ", &burst, 1, UINT_MAX)) continue;
            if (!read_number("Priority (-1000000 to 1000000): ",
                             &priority, -1000000, 1000000)) continue;

            success = job_manager_add(&manager, id, arrival, burst,
                                      priority, error, sizeof(error));

            if (success) puts("Job added successfully.");
        } else if (choice == 2) {
            job_manager_list(&manager);
            continue;
        } else {
            if (!read_line("Job ID: ", id, sizeof(id))) break;

            if (choice == 3) {
                success = job_manager_mark_ready(
                    &manager, id, error, sizeof(error));
            } else if (choice == 4) {
                success = job_manager_mark_running(
                    &manager, id, error, sizeof(error));
            } else {
                long completion;

                if (!read_number("Completion time: ", &completion,
                                 0, UINT_MAX)) continue;

                success = job_manager_mark_completed(
                    &manager, id, (unsigned int)completion,
                    error, sizeof(error));
            }

            if (success) puts("Job state updated.");
        }

        if (!success) {
            fprintf(stderr, "Operation failed: %s\n", error);
        }
    }

    job_manager_destroy(&manager);
    puts("Job manager closed.");
}

static bool enter_jobs(JobManager *manager)
{
    long count;

    if (!read_number("Number of jobs (1-10000): ",
                     &count, 1, 10000)) {
        return false;
    }

    for (long i = 0; i < count; i++) {
        char id[INPUT_SIZE];
        char error[256];
        long arrival, burst, priority;

        printf("\nJob %ld of %ld\n", i + 1, count);

        if (!read_line("Job ID: ", id, sizeof(id)) ||
            !read_number("Arrival time: ", &arrival, 0, UINT_MAX) ||
            !read_number("CPU burst: ", &burst, 1, UINT_MAX) ||
            !read_number("Priority: ", &priority,
                         -1000000, 1000000)) {
            return false;
        }

        if (!job_manager_add(manager, id, arrival, burst,
                             priority, error, sizeof(error))) {
            fprintf(stderr, "Job rejected: %s\n", error);
            return false;
        }
    }

    return true;
}

static void scheduler_menu(void)
{
    JobManager manager;
    job_manager_init(&manager);

    bool loaded = false;

    for (;;) {
        puts("\n--- FR2: CPU Scheduler ---");
        puts("1. Enter/replace jobs");
        puts("2. Run FCFS");
        puts("3. Run Round Robin");
        puts("4. Run Priority");
        puts("5. Compare all policies");
        puts("6. List jobs");
        puts("0. Back");

        long choice;
        if (!read_number("Choice: ", &choice, 0, 6)) break;
        if (choice == 0) break;

        if (choice == 1) {
            job_manager_destroy(&manager);
            job_manager_init(&manager);

            loaded = enter_jobs(&manager);

            if (!loaded) {
                job_manager_destroy(&manager);
                job_manager_init(&manager);
                puts("Job entry cancelled. No partial job set retained.");
            }
            continue;
        }

        if (choice == 6) {
            job_manager_list(&manager);
            continue;
        }

        if (!loaded || manager.count == 0) {
            puts("Enter a valid job set first.");
            continue;
        }

        unsigned int quantum = 1;

        if (choice == 3 || choice == 5) {
            if (!read_positive_unsigned(
                    "Round Robin quantum: ", &quantum)) {
                break;
            }
        }

        SchedulingPolicy policies[] = {
            SCHED_FCFS,
            SCHED_ROUND_ROBIN,
            SCHED_PRIORITY
        };

        int first = 0;
        int last = 0;

        if (choice == 2) first = last = 0;
        else if (choice == 3) first = last = 1;
        else if (choice == 4) first = last = 2;
        else if (choice == 5) {
            first = 0;
            last = 2;
        } else continue;

        for (int i = first; i <= last; i++) {
            ScheduleResult result;
            char error[256];

            unsigned int q = policies[i] == SCHED_ROUND_ROBIN
                                 ? quantum : 1;

            if (!scheduler_run(&manager, policies[i], q,
                               &result, error, sizeof(error))) {
                fprintf(stderr, "Scheduling failed: %s\n", error);
                continue;
            }

            scheduler_print_result(&manager, policies[i], &result);
            schedule_result_destroy(&result);
        }
    }

    job_manager_destroy(&manager);
    puts("Scheduler closed.");
}

static void producer_consumer_menu(void)
{
    long count;

    if (!read_number("Number of events (1-100000): ",
                     &count, 1, 100000)) return;

    char count_text[32];
    snprintf(count_text, sizeof(count_text), "%ld", count);

    char *arguments[] = {
        "engine/src/producer_consumer/producer_consumer",
        count_text,
        NULL
    };

    puts("Starting producer-consumer demonstration...");

    (void)run_child(arguments);
}

static void shared_memory_menu(void)
{
    char message[INPUT_SIZE];

    if (!read_line("Message to send: ", message, sizeof(message))) {
        return;
    }

    if (message[0] == '\0') {
        puts("Message cannot be empty.");
        return;
    }

    char *reader_args[] = {
        "engine/src/shared_memory/shared_memory",
        "reader",
        NULL
    };

    char *writer_args[] = {
        "engine/src/shared_memory/shared_memory",
        "writer",
        message,
        NULL
    };

    puts("Starting shared-memory reader...");

    pid_t reader = fork();

    if (reader < 0) {
        perror("fork reader");
        return;
    }

    if (reader == 0) {
        execv(reader_args[0], reader_args);
        perror("exec reader");
        _exit(127);
    }

    pid_t writer = fork();

    if (writer < 0) {
        perror("fork writer");
        kill(reader, SIGTERM);
        waitpid(reader, NULL, 0);
        perror("fork writer");
        return;
    }

    if (writer == 0) {
        execv(writer_args[0], writer_args);
        perror("exec writer");
        _exit(127);
    }

    int reader_status = 0;
    int writer_status = 0;

    while (waitpid(reader, &reader_status, 0) < 0 && errno == EINTR) {
        if (interrupted) kill(reader, SIGTERM);
    }

    while (waitpid(writer, &writer_status, 0) < 0 && errno == EINTR) {
        if (interrupted) kill(writer, SIGTERM);
    }

    if (WIFEXITED(reader_status) && WEXITSTATUS(reader_status) == 0 &&
        WIFEXITED(writer_status) && WEXITSTATUS(writer_status) == 0) {
        puts("Shared-memory exchange completed successfully.");
    } else {
        fprintf(stderr, "Shared-memory exchange failed. Check child output and IPC cleanup.\n");
    }
}

static void banker_menu(void)
{
    for (;;) {
        puts("\n--- FR5: Banker's Algorithm ---");
        puts("1. Check resource state");
        puts("0. Back");

        long choice;
        if (!read_number("Choice: ", &choice, 0, 1)) break;
        if (choice == 0) break;

        BankerState state = {0};
        BankerResult result;
        char error[256];

        long processes, resources;

        if (!read_number("Processes (1-64): ",
                         &processes, 1, BANKER_MAX_PROCESSES) ||
            !read_number("Resource types (1-32): ",
                         &resources, 1, BANKER_MAX_RESOURCES)) {
            break;
        }

        state.process_count = (size_t)processes;
        state.resource_count = (size_t)resources;

        bool input_ok = true;
        char prompt[100];

        for (size_t r = 0; r < state.resource_count; r++) {
            snprintf(prompt, sizeof(prompt), "Available R%zu: ", r);
            if (!read_unsigned(prompt, &state.available[r])) {
                input_ok = false;
                break;
            }
        }

        for (size_t p = 0; input_ok && p < state.process_count; p++) {
            for (size_t r = 0; r < state.resource_count; r++) {
                snprintf(prompt, sizeof(prompt),
                         "Allocation P%zu R%zu: ", p, r);

                if (!read_unsigned(prompt, &state.allocation[p][r])) {
                    input_ok = false;
                    break;
                }
            }
        }

        for (size_t p = 0; input_ok && p < state.process_count; p++) {
            for (size_t r = 0; r < state.resource_count; r++) {
                snprintf(prompt, sizeof(prompt),
                         "Maximum P%zu R%zu: ", p, r);

                if (!read_unsigned(prompt, &state.maximum[p][r])) {
                    input_ok = false;
                    break;
                }
            }
        }

        if (!input_ok) break;

        if (!banker_validate_and_calculate(
                &state, error, sizeof(error))) {
            fprintf(stderr, "Invalid state: %s\n", error);
            continue;
        }

        if (!banker_check_safety(
                &state, &result, error, sizeof(error))) {
            fprintf(stderr, "Safety check failed: %s\n", error);
            continue;
        }

        banker_print_state(&state);
        banker_print_result(&state, &result);
    }
}

static void process_manager_menu(void)
{
    puts("\n--- FR6: Process Manager ---");
    puts("1. Normal child process");
    puts("2. Simulated child failure");

    long choice;

    if (!read_number("Choice (1-2): ", &choice, 1, 2)) return;

    ProcessResult result;
    char error[256];

    if (!process_manager_run(choice == 2, &result,
                             error, sizeof(error))) {
        fprintf(stderr, "Process operation failed: %s\n", error);
        return;
    }

    printf("Child PID: %ld\n", (long)result.child_pid);

    if (result.exited_normally) {
        printf("Child exit code: %d\n", result.exit_code);
    } else if (result.terminated_by_signal) {
        printf("Child terminated by signal: %d\n",
               result.signal_number);
    }
}

static void resource_limits_menu(void)
{
    ResourceLimitResult result;
    char error[256];

    puts("\n--- FR7: Resource Limits ---");

    if (!resource_limits_run_demo(&result, error, sizeof(error))) {
        fprintf(stderr, "Resource-limit demo failed: %s\n", error);
        return;
    }

    printf("Original soft limit : %llu\n",
           (unsigned long long)result.original_soft_limit);
    printf("Original hard limit : %llu\n",
           (unsigned long long)result.original_hard_limit);
    printf("Applied soft limit  : %llu\n",
           (unsigned long long)result.applied_soft_limit);
    printf("Descriptors opened  : %zu\n",
           result.descriptors_opened);
    printf("Descriptors closed  : %zu\n",
           result.descriptors_closed);
    printf("Exhaustion detected : %s\n",
           result.exhaustion_error == EMFILE ? "YES (EMFILE)" : "NO");
    printf("Cleanup successful  : %s\n",
           result.cleanup_successful ? "YES" : "NO");
}

int main(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGINT, &action, NULL) < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    puts("========================================");
    puts("       CAMPUSCOMPUTE OS LAB");
    puts("========================================");

    char student_name[INPUT_SIZE];
    char student_srn[INPUT_SIZE];
    if (!authenticate_student(student_name, sizeof(student_name),
                             student_srn, sizeof(student_srn))) {
        return EXIT_FAILURE;
    }
    printf("\nAuthentication successful!\nWelcome, %s (%s).\n",
           student_name, student_srn);

    while (!interrupted) {
        puts("\n========== MAIN MENU ==========");
        puts("1. Job Manager");
        puts("2. CPU Scheduler");
        puts("3. Producer-Consumer");
        puts("4. Shared Memory IPC");
        puts("5. Banker's Algorithm");
        puts("6. Process Manager");
        puts("7. Resource Limits");
        puts("0. Exit");

        long choice;

        if (!read_number("Select option: ", &choice, 0, 7)) {
            break;
        }

        switch (choice) {
            case 1: job_manager_menu(); break;
            case 2: scheduler_menu(); break;
            case 3: producer_consumer_menu(); break;
            case 4: shared_memory_menu(); break;
            case 5: banker_menu(); break;
            case 6: process_manager_menu(); break;
            case 7: resource_limits_menu(); break;
            case 0:
                puts("Exiting CampusCompute.");
                return EXIT_SUCCESS;
            default:
                break;
        }
    }

    puts("\nCampusCompute stopped safely.");
    return EXIT_SUCCESS;
}
