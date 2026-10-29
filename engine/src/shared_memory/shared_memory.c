
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define SHM_NAME "/campuscompute_fr4_shm"
#define SEM_READY_NAME "/campuscompute_fr4_ready"
#define SEM_ACK_NAME "/campuscompute_fr4_ack"
#define MESSAGE_SIZE 256
#define WAIT_TIMEOUT 10

typedef struct {
    unsigned int sequence;
    char message[MESSAGE_SIZE];
    bool ready;
} SharedData;

static void usage(const char *program)
{
    printf("Usage:\n");
    printf("  %s writer <message>\n", program);
    printf("  %s reader\n", program);
    printf("  %s --help\n", program);
}

static int wait_for_sem(sem_t *sem, int seconds)
{
    struct timespec deadline;

    if (clock_gettime(CLOCK_REALTIME, &deadline) == -1) {
        perror("clock_gettime");
        return -1;
    }

    deadline.tv_sec += seconds;

    for (;;) {
        if (sem_timedwait(sem, &deadline) == 0)
            return 0;

        if (errno == EINTR)
            continue;

        if (errno == ETIMEDOUT)
            fprintf(stderr, "Error: semaphore wait timed out.\n");
        else
            perror("sem_timedwait");

        return -1;
    }
}

static void close_mapping(SharedData *data, int fd)
{
    if (data != MAP_FAILED)
        (void)munmap(data, sizeof(*data));

    if (fd >= 0)
        (void)close(fd);
}

/* Only the writer creates and unlinks the named IPC objects. */
static int writer(const char *message)
{
    int fd = -1;
    SharedData *data = MAP_FAILED;
    sem_t *ready = SEM_FAILED;
    sem_t *ack = SEM_FAILED;
    bool shm_created = false;
    bool ready_created = false;
    bool ack_created = false;
    int result = EXIT_FAILURE;

    fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) {
        perror("shm_open (writer)");
        return EXIT_FAILURE;
    }
    shm_created = true;

    if (ftruncate(fd, (off_t)sizeof(SharedData)) == -1) {
        perror("ftruncate");
        goto cleanup;
    }

    data = mmap(NULL, sizeof(*data), PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap (writer)");
        goto cleanup;
    }

    ready = sem_open(SEM_READY_NAME, O_CREAT | O_EXCL, 0600, 0);
    if (ready == SEM_FAILED) {
        perror("sem_open (ready)");
        goto cleanup;
    }
    ready_created = true;

    ack = sem_open(SEM_ACK_NAME, O_CREAT | O_EXCL, 0600, 0);
    if (ack == SEM_FAILED) {
        perror("sem_open (ack)");
        goto cleanup;
    }
    ack_created = true;

    data->sequence = 1;
    (void)snprintf(data->message, sizeof(data->message), "%s", message);
    data->ready = true;

    printf("[writer] published sequence=%u message=%s\n",
           data->sequence, data->message);
    fflush(stdout);

    if (sem_post(ready) == -1) {
        perror("sem_post (ready)");
        goto cleanup;
    }

    if (wait_for_sem(ack, WAIT_TIMEOUT) == -1)
        goto cleanup;

    printf("[writer] reader acknowledged the message.\n");
    result = EXIT_SUCCESS;

cleanup:
    if (ready != SEM_FAILED)
        (void)sem_close(ready);
    if (ack != SEM_FAILED)
        (void)sem_close(ack);

    close_mapping(data, fd);

    if (ack_created && sem_unlink(SEM_ACK_NAME) == -1) {
        perror("sem_unlink (ack)");
        result = EXIT_FAILURE;
    }
    if (ready_created && sem_unlink(SEM_READY_NAME) == -1) {
        perror("sem_unlink (ready)");
        result = EXIT_FAILURE;
    }
    if (shm_created && shm_unlink(SHM_NAME) == -1) {
        perror("shm_unlink");
        result = EXIT_FAILURE;
    }

    return result;
}

/* Retry briefly when the reader starts before the writer. */
static int open_shared_memory(int *fd_out)
{
    struct timespec pause_time = {0, 100000000L};
    struct timespec deadline;

    if (clock_gettime(CLOCK_REALTIME, &deadline) == -1) {
        perror("clock_gettime");
        return -1;
    }
    deadline.tv_sec += WAIT_TIMEOUT;

    for (;;) {
        int fd = shm_open(SHM_NAME, O_RDWR, 0600);

        if (fd >= 0) {
            *fd_out = fd;
            return 0;
        }

        if (errno != ENOENT) {
            perror("shm_open (reader)");
            return -1;
        }

        struct timespec now;
        if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
            perror("clock_gettime");
            return -1;
        }

        if (now.tv_sec > deadline.tv_sec ||
            (now.tv_sec == deadline.tv_sec &&
             now.tv_nsec >= deadline.tv_nsec)) {
            fprintf(stderr,
                    "Error: writer did not start within %d seconds.\n",
                    WAIT_TIMEOUT);
            return -1;
        }

        while (nanosleep(&pause_time, &pause_time) == -1) {
            if (errno != EINTR) {
                perror("nanosleep");
                return -1;
            }
        }

        pause_time.tv_sec = 0;
        pause_time.tv_nsec = 100000000L;
    }
}

static int open_semaphore(const char *name, sem_t **sem_out)
{
    struct timespec pause_time = {0, 100000000L};
    struct timespec deadline;

    if (clock_gettime(CLOCK_REALTIME, &deadline) == -1) {
        perror("clock_gettime");
        return -1;
    }
    deadline.tv_sec += WAIT_TIMEOUT;

    for (;;) {
        sem_t *sem = sem_open(name, 0);

        if (sem != SEM_FAILED) {
            *sem_out = sem;
            return 0;
        }

        if (errno != ENOENT) {
            perror("sem_open (reader)");
            return -1;
        }

        struct timespec now;
        if (clock_gettime(CLOCK_REALTIME, &now) == -1) {
            perror("clock_gettime");
            return -1;
        }

        if (now.tv_sec > deadline.tv_sec ||
            (now.tv_sec == deadline.tv_sec &&
             now.tv_nsec >= deadline.tv_nsec)) {
            fprintf(stderr, "Error: semaphore was not created in time.\n");
            return -1;
        }

        while (nanosleep(&pause_time, &pause_time) == -1) {
            if (errno != EINTR) {
                perror("nanosleep");
                return -1;
            }
        }

        pause_time.tv_sec = 0;
        pause_time.tv_nsec = 100000000L;
    }
}

static int reader(void)
{
    int fd = -1;
    SharedData *data = MAP_FAILED;
    sem_t *ready = SEM_FAILED;
    sem_t *ack = SEM_FAILED;
    int result = EXIT_FAILURE;

    if (open_shared_memory(&fd) == -1)
        return EXIT_FAILURE;

    struct stat info;
    if (fstat(fd, &info) == -1) {
        perror("fstat");
        goto cleanup;
    }

    if (info.st_size < (off_t)sizeof(SharedData)) {
        fprintf(stderr, "Error: shared-memory region is too small.\n");
        goto cleanup;
    }

    data = mmap(NULL, sizeof(*data), PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap (reader)");
        goto cleanup;
    }

    if (open_semaphore(SEM_READY_NAME, &ready) == -1)
        goto cleanup;

    if (open_semaphore(SEM_ACK_NAME, &ack) == -1)
        goto cleanup;

    if (wait_for_sem(ready, WAIT_TIMEOUT) == -1)
        goto cleanup;

    if (!data->ready ||
        memchr(data->message, '\0', sizeof(data->message)) == NULL) {
        fprintf(stderr, "Error: shared message is invalid.\n");
        goto cleanup;
    }

    printf("[reader] received sequence=%u message=%s\n",
           data->sequence, data->message);
    fflush(stdout);

    data->ready = false;

    if (sem_post(ack) == -1) {
        perror("sem_post (ack)");
        goto cleanup;
    }

    result = EXIT_SUCCESS;

cleanup:
    if (ready != SEM_FAILED)
        (void)sem_close(ready);
    if (ack != SEM_FAILED)
        (void)sem_close(ack);

    close_mapping(data, fd);
    return result;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        usage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (argc == 3 && strcmp(argv[1], "writer") == 0) {
        if (argv[2][0] == '\0' ||
            strlen(argv[2]) >= MESSAGE_SIZE) {
            fprintf(stderr,
                    "Error: message must contain 1 to %d bytes.\n",
                    MESSAGE_SIZE - 1);
            return EXIT_FAILURE;
        }
        return writer(argv[2]);
    }

    if (argc == 2 && strcmp(argv[1], "reader") == 0)
        return reader();

    usage(argv[0]);
    return EXIT_FAILURE;
}
