#include "job_manager.h"
#include "scheduler.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_SIZE 256

static bool read_line(const char *prompt, char *out, size_t size) {
    fputs(prompt, stdout);
    fflush(stdout);
    if (fgets(out, (int)size, stdin) == NULL) return false;
    size_t n = strlen(out);
    if (n > 0 && out[n - 1] == '\n') {
        out[n - 1] = '\0';
        return true;
    }
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) { }
    fprintf(stderr, "Input too long; please try again.\n");
    out[0] = '\0';
    return true;
}

static bool parse_long_value(const char *text, long *out) {
    char *end = NULL;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno != 0 || end == text) return false;
    while (*end == ' ' || *end == '\t' || *end == '\r') ++end;
    if (*end != '\0') return false;
    *out = value;
    return true;
}

static bool read_number(const char *prompt, long *out) {
    char buffer[INPUT_SIZE];
    if (!read_line(prompt, buffer, sizeof(buffer))) return false;
    if (!parse_long_value(buffer, out)) {
        fprintf(stderr, "Invalid number; enter a whole number.\n");
        return false;
    }
    return true;
}

static bool load_jobs(JobManager *manager) {
    char error[256];
    long count;
    if (!read_number("Number of jobs (1-10000): ", &count)) return false;
    if (count < 1 || count > 10000) {
        fprintf(stderr, "Job count must be between 1 and 10000.\n");
        return false;
    }
    for (long i = 0; i < count; ++i) {
        char id[INPUT_SIZE];
        long arrival, burst, priority;
        printf("\nJob %ld of %ld\n", i + 1, count);
        if (!read_line("ID: ", id, sizeof(id)) ||
            !read_number("Arrival time (>= 0): ", &arrival) ||
            !read_number("CPU burst (> 0): ", &burst) ||
            !read_number("Priority (-1000000..1000000): ", &priority)) return false;
        if (!job_manager_add(manager, id, arrival, burst, priority, error, sizeof(error))) {
            fprintf(stderr, "Cannot add job: %s\n", error);
            return false;
        }
    }
    return true;
}

static bool run_and_print(const JobManager *manager, SchedulingPolicy policy,
                          unsigned int quantum) {
    ScheduleResult result;
    char error[256];
    if (!scheduler_run(manager, policy, quantum, &result, error, sizeof(error))) {
        fprintf(stderr, "Scheduling failed: %s\n", error);
        return false;
    }
    scheduler_print_result(manager, policy, &result);
    schedule_result_destroy(&result);
    return true;
}

static void usage(const char *program) {
    printf("CampusCompute CPU Scheduler (FR2)\nUsage: %s [--help]\n", program);
    puts("Interactive menu: FCFS, Round Robin, non-preemptive Priority, or compare all.");
}

int main(int argc, char **argv) {
    if (argc > 2 || (argc == 2 && strcmp(argv[1], "--help") != 0 &&
                     strcmp(argv[1], "-h") != 0)) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2) {
        usage(argv[0]);
        return EXIT_SUCCESS;
    }

    JobManager manager;
    job_manager_init(&manager);
    char choice[INPUT_SIZE];
    bool loaded = false;

    for (;;) {
        puts("\n=== CampusCompute | CPU Scheduler (FR2) ===");
        puts("1. Enter/replace jobs\n2. Run FCFS\n3. Run Round Robin");
        puts("4. Run non-preemptive Priority\n5. Compare all policies");
        puts("6. List current jobs\n0. Exit");
        if (!read_line("Choice: ", choice, sizeof(choice))) break;
        if (strcmp(choice, "0") == 0) break;
        if (strcmp(choice, "1") == 0) {
            job_manager_destroy(&manager);
            job_manager_init(&manager);
            loaded = load_jobs(&manager);
            if (!loaded) {
                job_manager_destroy(&manager);
                job_manager_init(&manager);
            } else {
                puts("Jobs loaded successfully.");
            }
        } else if (strcmp(choice, "6") == 0) {
            job_manager_list(&manager);
        } else if (strcmp(choice, "2") == 0 || strcmp(choice, "3") == 0 ||
                   strcmp(choice, "4") == 0 || strcmp(choice, "5") == 0) {
            if (!loaded || manager.count == 0) {
                fprintf(stderr, "Enter a valid job set first (menu option 1).\n");
                continue;
            }
            unsigned int quantum = 1;
            if (strcmp(choice, "3") == 0 || strcmp(choice, "5") == 0) {
                long q;
                if (!read_number("Round Robin time quantum (> 0): ", &q)) continue;
                if (q <= 0 || (unsigned long)q > UINT_MAX) {
                    fprintf(stderr, "Quantum must be between 1 and %u.\n", UINT_MAX);
                    continue;
                }
                quantum = (unsigned int)q;
            }
            if (strcmp(choice, "2") == 0) {
                (void)run_and_print(&manager, SCHED_FCFS, 1);
            } else if (strcmp(choice, "3") == 0) {
                (void)run_and_print(&manager, SCHED_ROUND_ROBIN, quantum);
            } else if (strcmp(choice, "4") == 0) {
                (void)run_and_print(&manager, SCHED_PRIORITY, 1);
            } else {
                (void)run_and_print(&manager, SCHED_FCFS, 1);
                (void)run_and_print(&manager, SCHED_ROUND_ROBIN, quantum);
                (void)run_and_print(&manager, SCHED_PRIORITY, 1);
            }
        } else {
            fprintf(stderr, "Unknown menu choice. Select 0 through 6.\n");
        }
    }

    job_manager_destroy(&manager);
    puts("Scheduler exited cleanly.");
    return EXIT_SUCCESS;
}
