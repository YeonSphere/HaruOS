/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_SCHED_RUSTY_H
#define _LINUX_SCHED_RUSTY_H

#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/list.h>
#include <linux/spinlock.h>

/* Rusty scheduler entity */
struct sched_rusty_entity {
    struct list_head run_list;
    u64 exec_start;
    u64 sum_exec_runtime;
    u64 vruntime;
    unsigned int on_rq;
    int prio;
    int static_prio;
    int normal_prio;
};

/* Rusty run queue */
struct rusty_rq {
    struct list_head queue;
    unsigned int nr_running;
    u64 min_vruntime;
    raw_spinlock_t lock;
    
    /* Load balancing */
    unsigned long nr_migrations_in;
    unsigned long nr_migrations_out;
    
    /* Statistics */
    u64 exec_clock;
    u64 wait_runtime;
    
    /* RT bandwidth enforcement */
    struct rt_bandwidth rt_bandwidth;
    
    /* CPU selection */
    int cpu;
    int online;
    
    /* Task group support */
    struct task_group *tg;
    
    /* Priority management */
    int local_prio;
    int global_prio;
};

/* Forward declarations */
extern const struct sched_class rusty_sched_class;

/* Rusty scheduler functions */
extern void init_rusty_rq(struct rusty_rq *rusty_rq);
extern void update_curr_rusty(struct rq *rq);
extern void update_min_vruntime(struct rusty_rq *rusty_rq);
extern u64 calc_delta_fair(u64 delta, struct task_struct *p);

/* Helper functions */
static inline bool entity_before(struct sched_rusty_entity *a,
                               struct sched_rusty_entity *b)
{
    return (s64)(a->vruntime - b->vruntime) < 0;
}

static inline bool needs_resched_rusty(struct sched_rusty_entity *se,
                                     struct rusty_rq *rusty_rq)
{
    u64 runtime = se->sum_exec_runtime;
    u64 min_runtime = rusty_rq->min_vruntime;
    return runtime > min_runtime + RUSTY_MIN_GRANULARITY;
}

static inline void requeue_task_rusty(struct rq *rq, struct task_struct *p)
{
    struct sched_rusty_entity *se = &p->rusty;
    struct rusty_rq *rusty_rq = &per_cpu(rusty_rq, cpu_of(rq));
    
    list_del(&se->run_list);
    list_add_tail(&se->run_list, &rusty_rq->queue);
}

static inline struct sched_rusty_entity *pick_next_entity_rusty(struct rusty_rq *rusty_rq)
{
    struct sched_rusty_entity *se;
    struct list_head *queue = &rusty_rq->queue;

    if (list_empty(queue))
        return NULL;

    se = list_first_entry(queue, struct sched_rusty_entity, run_list);
    return se;
}

/* Rusty scheduler tunables */
#define RUSTY_MIN_GRANULARITY (3 * NSEC_PER_MSEC)
#define RUSTY_DEFAULT_LATENCY (6 * NSEC_PER_MSEC)

#endif /* _LINUX_SCHED_RUSTY_H */
