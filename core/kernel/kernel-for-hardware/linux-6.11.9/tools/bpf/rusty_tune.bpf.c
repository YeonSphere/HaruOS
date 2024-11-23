// SPDX-License-Identifier: GPL-2.0
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <linux/sched.h>
#include "rusty_tune.h"

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, MAX_CPUS);
    __type(key, u32);
    __type(value, struct rusty_stats);
} cpu_stats SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} rb SEC(".maps");

// Track scheduler latency
SEC("tp/sched/sched_switch")
int trace_sched_switch(struct trace_event_raw_sched_switch *ctx)
{
    u32 cpu = bpf_get_smp_processor_id();
    struct rusty_stats *stats;
    u64 ts = bpf_ktime_get_ns();
    
    stats = bpf_map_lookup_elem(&cpu_stats, &cpu);
    if (!stats) {
        struct rusty_stats new_stats = {};
        bpf_map_update_elem(&cpu_stats, &cpu, &new_stats, BPF_ANY);
        stats = bpf_map_lookup_elem(&cpu_stats, &cpu);
        if (!stats)
            return 0;
    }
    
    // Update statistics
    stats->switches++;
    stats->last_switch = ts;
    
    // Check for long scheduling latencies
    if (ctx->prev_state == TASK_RUNNING) {
        u64 delta = ts - stats->last_enqueue;
        if (delta > RUSTY_MAX_LATENCY) {
            struct event *e;
            e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
            if (e) {
                e->cpu = cpu;
                e->latency = delta;
                e->pid = ctx->next_pid;
                e->prev_pid = ctx->prev_pid;
                bpf_ringbuf_submit(e, 0);
            }
        }
    }
    
    return 0;
}

// Track task enqueue events
SEC("tp/sched/sched_wakeup")
int trace_sched_wakeup(struct trace_event_raw_sched_wakeup *ctx)
{
    u32 cpu = bpf_get_smp_processor_id();
    struct rusty_stats *stats;
    
    stats = bpf_map_lookup_elem(&cpu_stats, &cpu);
    if (!stats) {
        struct rusty_stats new_stats = {};
        bpf_map_update_elem(&cpu_stats, &cpu, &new_stats, BPF_ANY);
        stats = bpf_map_lookup_elem(&cpu_stats, &cpu);
        if (!stats)
            return 0;
    }
    
    stats->wakeups++;
    stats->last_enqueue = bpf_ktime_get_ns();
    
    return 0;
}

// Monitor migration events
SEC("tp/sched/sched_migrate_task")
int trace_sched_migrate_task(struct trace_event_raw_sched_migrate_task *ctx)
{
    u32 cpu = bpf_get_smp_processor_id();
    struct rusty_stats *stats;
    
    stats = bpf_map_lookup_elem(&cpu_stats, &cpu);
    if (!stats) {
        struct rusty_stats new_stats = {};
        bpf_map_update_elem(&cpu_stats, &cpu, &new_stats, BPF_ANY);
        stats = bpf_map_lookup_elem(&cpu_stats, &cpu);
        if (!stats)
            return 0;
    }
    
    stats->migrations++;
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
