#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "rusty_tune.h"
#include "rusty_tune.skel.h"

static volatile bool exiting;

void sig_handler(int sig)
{
    exiting = true;
}

static int handle_event(void *ctx, void *data, size_t data_sz)
{
    const struct event *e = data;
    printf("CPU %u: Scheduling latency %llu ns (PID %u -> %u)\n",
           e->cpu, e->latency, e->prev_pid, e->pid);
    return 0;
}

int main(int argc, char **argv)
{
    struct rusty_tune_bpf *skel;
    struct ring_buffer *rb = NULL;
    int err;

    /* Set up signal handler */
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    /* Open BPF application */
    skel = rusty_tune_bpf__open();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF program\n");
        return 1;
    }

    /* Load & verify BPF programs */
    err = rusty_tune_bpf__load(skel);
    if (err) {
        fprintf(stderr, "Failed to load BPF program: %d\n", err);
        goto cleanup;
    }

    /* Attach tracepoints */
    err = rusty_tune_bpf__attach(skel);
    if (err) {
        fprintf(stderr, "Failed to attach BPF program: %d\n", err);
        goto cleanup;
    }

    /* Set up ring buffer */
    rb = ring_buffer__new(bpf_map__fd(skel->maps.rb), handle_event, NULL, NULL);
    if (!rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer\n");
        goto cleanup;
    }

    printf("Successfully started! Please run `sudo cat /sys/kernel/debug/tracing/trace_pipe` "
           "to see output\n");

    /* Main loop */
    while (!exiting) {
        err = ring_buffer__poll(rb, 100 /* timeout, ms */);
        if (err < 0) {
            printf("Error polling ring buffer: %d\n", err);
            break;
        }
    }

cleanup:
    ring_buffer__free(rb);
    rusty_tune_bpf__destroy(skel);
    return err < 0 ? 1 : 0;
}
