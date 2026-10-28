#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define QUEUE_CAPACITY 8
#define DEFAULT_EVENTS 12

typedef struct {
    unsigned long sequence;
    char message[160];
    time_t created_at;
} Event;

typedef struct {
    Event items[QUEUE_CAPACITY];
    size_t head, tail, count;
    bool producer_done;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty, not_full;
    FILE *log_file;
    unsigned long produced, consumed;
    bool write_failed;
} EventQueue;

static EventQueue queue = {
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER
};

static void timestamp_text(time_t value, char *buffer, size_t length) {
    struct tm tm_value;
    if (localtime_r(&value, &tm_value) == NULL ||
        strftime(buffer, length, "%Y-%m-%d %H:%M:%S", &tm_value) == 0) {
        (void)snprintf(buffer, length, "unknown-time");
    }
}

static int queue_push(const char *message, unsigned long sequence) {
    int rc = pthread_mutex_lock(&queue.mutex);
    if (rc != 0) {
        fprintf(stderr, "producer: mutex lock failed: %s\n", strerror(rc));
        return -1;
    }
    while (queue.count == QUEUE_CAPACITY) {
        rc = pthread_cond_wait(&queue.not_full, &queue.mutex);
        if (rc != 0) {
            fprintf(stderr, "producer: wait failed: %s\n", strerror(rc));
            (void)pthread_mutex_unlock(&queue.mutex);
            return -1;
        }
    }

    Event *event = &queue.items[queue.tail];
    event->sequence = sequence;
    event->created_at = time(NULL);
    (void)snprintf(event->message, sizeof(event->message), "%s", message);
    queue.tail = (queue.tail + 1) % QUEUE_CAPACITY;
    queue.count++;
    queue.produced++;

    rc = pthread_cond_signal(&queue.not_empty);
    int unlock_rc = pthread_mutex_unlock(&queue.mutex);
    if (rc != 0 || unlock_rc != 0) {
        fprintf(stderr, "producer: synchronization failed\n");
        return -1;
    }
    return 0;
}

static bool queue_pop(Event *event) {
    int rc = pthread_mutex_lock(&queue.mutex);
    if (rc != 0) {
        fprintf(stderr, "consumer: mutex lock failed: %s\n", strerror(rc));
        return false;
    }
    while (queue.count == 0 && !queue.producer_done) {
        rc = pthread_cond_wait(&queue.not_empty, &queue.mutex);
        if (rc != 0) {
            fprintf(stderr, "consumer: wait failed: %s\n", strerror(rc));
            (void)pthread_mutex_unlock(&queue.mutex);
            return false;
        }
    }
    if (queue.count == 0 && queue.producer_done) {
        (void)pthread_mutex_unlock(&queue.mutex);
        return false;
    }

    *event = queue.items[queue.head];
    queue.head = (queue.head + 1) % QUEUE_CAPACITY;
    queue.count--;
    queue.consumed++;

    rc = pthread_cond_signal(&queue.not_full);
    int unlock_rc = pthread_mutex_unlock(&queue.mutex);
    if (rc != 0 || unlock_rc != 0) {
        fprintf(stderr, "consumer: synchronization failed\n");
        return false;
    }
    return true;
}

static void *producer_main(void *argument) {
    const unsigned long total = *(const unsigned long *)argument;
    for (unsigned long i = 1; i <= total; ++i) {
        char message[160];
        (void)snprintf(message, sizeof(message),
                       "OS activity event %lu: job state changed", i);
        if (queue_push(message, i) != 0) break;
        printf("[producer] queued event %lu\n", i);
    }

    int rc = pthread_mutex_lock(&queue.mutex);
    if (rc == 0) {
        queue.producer_done = true;
        (void)pthread_cond_broadcast(&queue.not_empty);
        (void)pthread_cond_broadcast(&queue.not_full);
        (void)pthread_mutex_unlock(&queue.mutex);
    } else {
        fprintf(stderr, "producer: unable to mark completion: %s\n", strerror(rc));
    }
    return NULL;
}

static void *consumer_main(void *unused) {
    (void)unused;
    Event event;
    while (queue_pop(&event)) {
        char when[32];
        timestamp_text(event.created_at, when, sizeof(when));
        printf("[consumer] %s | seq=%lu | %s\n",
               when, event.sequence, event.message);
        if (fprintf(queue.log_file, "%s\t%lu\t%s\n",
                    when, event.sequence, event.message) < 0) {
            queue.write_failed = true;
            fprintf(stderr, "consumer: failed to write activity log\n");
        }
    }
    return NULL;
}

static bool parse_count(const char *text, unsigned long *value) {
    if (text[0] == '-') return false;
    char *end = NULL;
    errno = 0;
    unsigned long parsed = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed == 0 || parsed > 100000) return false;
    *value = parsed;
    return true;
}

static void usage(const char *program) {
    printf("Usage: %s [event-count] [log-file]\n", program);
    printf("  event-count: integer from 1 to 100000 (default %d)\n", DEFAULT_EVENTS);
    printf("  log-file:    output path (default logs/activity.log)\n");
}

int main(int argc, char **argv) {
    unsigned long event_count = DEFAULT_EVENTS;
    const char *log_path = "logs/activity.log";

    if (argc > 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (argc >= 2) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        if (!parse_count(argv[1], &event_count)) {
            fprintf(stderr, "Error: event-count must be an integer from 1 to 100000.\n");
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (argc == 3) {
        if (argv[2][0] == '\0') {
            fprintf(stderr, "Error: log-file path must not be empty.\n");
            return EXIT_FAILURE;
        }
        log_path = argv[2];
    }

    queue.log_file = fopen(log_path, "w");
    if (queue.log_file == NULL) {
        fprintf(stderr, "Error: cannot open log file '%s': %s\n",
                log_path, strerror(errno));
        fprintf(stderr, "Create the parent directory or pass a writable path.\n");
        return EXIT_FAILURE;
    }
    if (fprintf(queue.log_file, "timestamp\tsequence\tmessage\n") < 0) {
        fprintf(stderr, "Error: cannot initialize log file '%s'.\n", log_path);
        (void)fclose(queue.log_file);
        return EXIT_FAILURE;
    }

    pthread_t producer, consumer;
    bool consumer_started = false, producer_started = false;
    int rc = pthread_create(&consumer, NULL, consumer_main, NULL);
    if (rc != 0) {
        fprintf(stderr, "Error: cannot start consumer thread: %s\n", strerror(rc));
        (void)fclose(queue.log_file);
        return EXIT_FAILURE;
    }
    consumer_started = true;

    rc = pthread_create(&producer, NULL, producer_main, &event_count);
    if (rc != 0) {
        fprintf(stderr, "Error: cannot start producer thread: %s\n", strerror(rc));
        int lock_rc = pthread_mutex_lock(&queue.mutex);
        if (lock_rc == 0) {
            queue.producer_done = true;
            (void)pthread_cond_broadcast(&queue.not_empty);
            (void)pthread_mutex_unlock(&queue.mutex);
        }
    } else {
        producer_started = true;
    }

    if (producer_started) {
        rc = pthread_join(producer, NULL);
        if (rc != 0) fprintf(stderr, "Error: joining producer failed: %s\n", strerror(rc));
    }
    if (consumer_started) {
        rc = pthread_join(consumer, NULL);
        if (rc != 0) fprintf(stderr, "Error: joining consumer failed: %s\n", strerror(rc));
    }

    if (fflush(queue.log_file) != 0) queue.write_failed = true;
    if (fclose(queue.log_file) != 0) {
        fprintf(stderr, "Warning: closing activity log failed: %s\n", strerror(errno));
        queue.write_failed = true;
    }

    printf("\nSummary: produced=%lu consumed=%lu queued=%zu\n",
           queue.produced, queue.consumed, queue.count);
    printf("Log written to: %s\n", log_path);

    (void)pthread_cond_destroy(&queue.not_empty);
    (void)pthread_cond_destroy(&queue.not_full);
    (void)pthread_mutex_destroy(&queue.mutex);

    if (!producer_started || queue.produced != event_count ||
        queue.consumed != queue.produced || queue.count != 0 || queue.write_failed) {
        fprintf(stderr, "Error: logger did not finish cleanly; inspect messages above.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
