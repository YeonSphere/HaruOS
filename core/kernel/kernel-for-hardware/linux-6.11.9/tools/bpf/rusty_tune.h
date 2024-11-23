#ifndef __RUSTY_TUNE_H
#define __RUSTY_TUNE_H

#define MAX_CPUS 256
#define RUSTY_MAX_LATENCY 1000000  // 1ms in nanoseconds

struct rusty_stats {
    __u64 switches;        // Context switch count
    __u64 wakeups;         // Task wakeup count
    __u64 migrations;      // Task migration count
    __u64 last_switch;     // Last context switch timestamp
    __u64 last_enqueue;    // Last task enqueue timestamp
};

struct event {
    __u32 cpu;            // CPU ID
    __u32 pid;            // Current PID
    __u32 prev_pid;       // Previous PID
    __u64 latency;        // Scheduling latency
};

#endif /* __RUSTY_TUNE_H */
