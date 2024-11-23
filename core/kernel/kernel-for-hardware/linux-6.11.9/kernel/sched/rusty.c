# SPDX-License-Identifier: GPL-2.0-only
/*
 * AetherOS Rusty Scheduler Implementation
 */

#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/sched/rusty.h>
#include <linux/latencytop.h>
#include <linux/sched/debug.h>
#include <linux/cpumask.h>
#include <linux/slab.h>
#include <linux/profile.h>
#include <linux/interrupt.h>
#include <linux/mempolicy.h>
#include <linux/migrate.h>
#include <linux/task_work.h>
#include <linux/ratelimit.h>

/* Per-CPU runqueue */
DEFINE_PER_CPU(struct rusty_rq, rusty_rq);

/* Update minimum vruntime */
void update_min_vruntime(struct rusty_rq *rusty_rq)
{
    struct sched_rusty_entity *first = pick_next_entity_rusty(rusty_rq);
    if (first)
        rusty_rq->min_vruntime = first->vruntime;
}

/* Calculate delta based on task weight */
u64 calc_delta_fair(u64 delta, struct task_struct *p)
{
    if (unlikely(p->policy == SCHED_IDLE))
        delta = delta >> 1;
    return delta;
}

void init_rusty_rq(struct rusty_rq *rusty_rq)
{
    INIT_LIST_HEAD(&rusty_rq->queue);
    rusty_rq->min_vruntime = 0;
    rusty_rq->nr_running = 0;
    raw_spin_lock_init(&rusty_rq->lock);
    
    /* Initialize statistics */
    rusty_rq->nr_migrations_in = 0;
    rusty_rq->nr_migrations_out = 0;
    rusty_rq->exec_clock = 0;
    rusty_rq->wait_runtime = 0;
    
    /* Initialize RT bandwidth */
    init_rt_bandwidth(&rusty_rq->rt_bandwidth,
                     RUSTY_DEFAULT_LATENCY,
                     RUSTY_MIN_GRANULARITY);
    
    /* Initialize CPU info */
    rusty_rq->cpu = smp_processor_id();
    rusty_rq->online = cpu_online(rusty_rq->cpu);
    
    /* Initialize priorities */
    rusty_rq->local_prio = CONFIG_SCHED_RUSTY_LOCAL_PRIO;
    rusty_rq->global_prio = CONFIG_SCHED_RUSTY_GLOBAL_PRIO;
}

static void update_curr_rusty(struct rq *rq)
{
    struct task_struct *curr = rq->curr;
    struct sched_rusty_entity *se = &curr->rusty;
    struct rusty_rq *rusty_rq = &per_cpu(rusty_rq, cpu_of(rq));
    u64 now = sched_clock();
    u64 delta_exec;

    if (unlikely(!se->exec_start))
        se->exec_start = now;

    delta_exec = now - se->exec_start;
    if (unlikely((s64)delta_exec <= 0))
        return;

    se->sum_exec_runtime += delta_exec;
    se->vruntime += calc_delta_fair(delta_exec, curr);
    update_min_vruntime(rusty_rq);
    se->exec_start = now;
    
    /* Update statistics */
    rusty_rq->exec_clock += delta_exec;
}

static void enqueue_task_rusty(struct rq *rq, struct task_struct *p, int flags)
{
    struct sched_rusty_entity *se = &p->rusty;
    struct rusty_rq *rusty_rq = &per_cpu(rusty_rq, cpu_of(rq));
    unsigned long rf;

    raw_spin_lock_irqsave(&rusty_rq->lock, rf);
    
    if (!se->on_rq) {
        se->on_rq = 1;
        
        /* Handle task migration */
        if (task_cpu(p) != rq->cpu) {
            rusty_rq->nr_migrations_in++;
            se->vruntime = max_vruntime(se->vruntime, rusty_rq->min_vruntime);
        }
        
        list_add_tail(&se->run_list, &rusty_rq->queue);
        rusty_rq->nr_running++;
        
        /* Update task priorities */
        se->prio = p->prio;
        se->static_prio = p->static_prio;
        se->normal_prio = normal_prio(p);
    }
    
    raw_spin_unlock_irqrestore(&rusty_rq->lock, rf);
}

static void dequeue_task_rusty(struct rq *rq, struct task_struct *p, int flags)
{
    struct sched_rusty_entity *se = &p->rusty;
    struct rusty_rq *rusty_rq = &per_cpu(rusty_rq, cpu_of(rq));
    unsigned long rf;

    raw_spin_lock_irqsave(&rusty_rq->lock, rf);
    
    if (se->on_rq) {
        se->on_rq = 0;
        
        /* Handle task migration */
        if (flags & DEQUEUE_MOVE) {
            rusty_rq->nr_migrations_out++;
        }
        
        list_del_init(&se->run_list);
        rusty_rq->nr_running--;
        
        /* Update wait time statistics */
        if (p->state == TASK_RUNNING)
            rusty_rq->wait_runtime += sched_clock() - se->exec_start;
    }
    
    raw_spin_unlock_irqrestore(&rusty_rq->lock, rf);
}

static struct task_struct *pick_next_task_rusty(struct rq *rq)
{
    struct rusty_rq *rusty_rq = &per_cpu(rusty_rq, cpu_of(rq));
    struct sched_rusty_entity *se;
    struct task_struct *p;

    if (!rusty_rq->nr_running)
        return NULL;

    se = pick_next_entity_rusty(rusty_rq);
    if (!se)
        return NULL;

    p = container_of(se, struct task_struct, rusty);
    return p;
}

static void check_preempt_curr_rusty(struct rq *rq, struct task_struct *p, int flags)
{
    struct task_struct *curr = rq->curr;
    struct sched_rusty_entity *se = &curr->rusty;
    struct sched_rusty_entity *pse = &p->rusty;

    if (entity_before(pse, se))
        resched_curr(rq);
}

static void set_curr_task_rusty(struct rq *rq)
{
    struct task_struct *p = rq->curr;
    p->rusty.exec_start = sched_clock();
}

static void task_tick_rusty(struct rq *rq, struct task_struct *curr, int queued)
{
    update_curr_rusty(rq);
    if (needs_resched_rusty(&curr->rusty, &per_cpu(rusty_rq, cpu_of(rq))))
        resched_curr(rq);
}

static void switched_to_rusty(struct rq *rq, struct task_struct *p)
{
    if (!task_on_rq_queued(p) && p->policy == SCHED_RUSTY)
        enqueue_task_rusty(rq, p, 0);
}

const struct sched_class rusty_sched_class = {
    .next = &fair_sched_class,
    .enqueue_task = enqueue_task_rusty,
    .dequeue_task = dequeue_task_rusty,
    .check_preempt_curr = check_preempt_curr_rusty,
    .pick_next_task = pick_next_task_rusty,
    .set_curr_task = set_curr_task_rusty,
    .task_tick = task_tick_rusty,
    .switched_to = switched_to_rusty,
};
