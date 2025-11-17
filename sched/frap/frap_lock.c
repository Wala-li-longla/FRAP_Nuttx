/* sched/frap/frap_lock.c */
#include <nuttx/config.h>
#include "sched/sched.h"
#include <debug.h>
#ifdef CONFIG_FRAP

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sched.h>
#include <nuttx/spinlock.h>
#include <nuttx/queue.h>
#include <nuttx/sched.h>
#include "frap_internal.h"
#include "frap_compat.h"

/* === 全局资源：FRAP R1/R2/R3 === */

int frap_lock(FAR struct frap_res *r, uint8_t spin_prio)
{
  FAR struct tcb_s *tcb = this_task();
  FAR struct frap_task_ext *ext = frap_get_ext(tcb);
  irqstate_t flags;

  if (!r || !ext) return -EINVAL;

  /* R1: 自旋优先级必须满足 P_i <= P_i^k */
  if (spin_prio < (uint8_t)tcb->sched_priority)
    {
      return -EINVAL;
    }

  /* 初始化等待者状态 */
  memset(&ext->waiter, 0, sizeof(ext->waiter));
  ext->waiter.tcb       = tcb;
  ext->waiter.base_prio = (uint8_t)tcb->sched_priority;
  ext->waiter.spin_prio = spin_prio;
  ext->waiting_res      = r;
  ext->in_cs            = false;

  /* 提升为自旋优先级（R1） */
  frap_set_prio(tcb, spin_prio);

  for (;;)
    {
      flags = spin_lock_irqsave(&r->sl);

      /* 如果无人持有，并且队列空或我在队首 ⇒ 进入临界段 */
      bool can_enter = false;

      if (r->owner == NULL)
        {
          FAR struct frap_waiter *head = frap_queue_peek_head(r);
          if (head == NULL)
            {
              /* 队列空，先把自己放到头，再立即占用 */
              frap_queue_enqueue_head_if_needed(r, &ext->waiter);
              can_enter = true;
            }
          else if (head == &ext->waiter)
            {
              can_enter = true;
            }
        }

      if (can_enter)
        {
          /* 从队列摘除并占有资源 */
          frap_queue_remove(r, &ext->waiter);
          r->owner = tcb;
          spin_unlock_irqrestore(&r->sl, flags);

          /* R2: 非抢占执行临界段（同核不可被更高优先级打断） */
          sched_lock();
          ext->in_cs = true;

          /* ------- 日志：进入临界段 ------- */
          sinfo("FRAP enter: pid=%d resid=%u (spin_prio=%u base=%u)\n",
                tcb->pid, (unsigned)r->id,
                (unsigned)ext->waiter.spin_prio,
                (unsigned)ext->waiter.base_prio);
    /* -------------------------------- */

          return OK;
        }

      /* 否则：尾插等待，自旋 */
      frap_queue_enqueue_tail(r, &ext->waiter);
      spin_unlock_irqrestore(&r->sl, flags);

      /* 如果我们曾被抢占钩子取消，则清掉标记，继续自旋（尾插已完成） */
      if (ext->waiter.cancelled)
        {
          /* ------- 日志：恢复并尾插重排 ------- */
          sinfo("FRAP resume+requeue: pid=%d resid=%u\n",
                tcb->pid, (unsigned)r->id);
          /* ----------------------------------- */
          ext->waiter.cancelled = false;
        }

      /* 让出时间片，避免空转占满 */
      (void)sched_yield();
    }
}

void frap_unlock(FAR struct frap_res *r)
{
  FAR struct tcb_s *tcb = this_task();
  FAR struct frap_task_ext *ext = frap_get_ext(tcb);
  irqstate_t flags;

  DEBUGASSERT(r && ext && ext->in_cs && r->owner == tcb);

  /* 退出非抢占临界段（R2 完结） */
  ext->in_cs = false;
  sched_unlock();

  /* 释放资源 */
  flags = spin_lock_irqsave(&r->sl);
  r->owner = NULL;
  spin_unlock_irqrestore(&r->sl, flags);

  /* 恢复基准优先级 P_i */
  frap_set_prio(tcb, ext->waiter.base_prio);

  /* ------- 日志：退出临界段 ------- */
  sinfo("FRAP exit : pid=%d resid=%u\n", tcb->pid, (unsigned)r->id);
  /* -------------------------------- */

  /* 清状态 */
  ext->waiting_res = NULL;
}

/* === 本地资源：简化 PCP（可选） === */

int frap_local_lock(FAR struct frap_res *r, uint8_t ceiling)
{
  FAR struct tcb_s *tcb = this_task();
  FAR struct frap_task_ext *ext = frap_get_ext(tcb);

  if (!r || !ext) return -EINVAL;
  r->ceiling = ceiling;

  /* 提升到 max(P_i, ceiling) 并进入非抢占临界段 */
  uint8_t base = (uint8_t)tcb->sched_priority;
  uint8_t eff  = (base > ceiling) ? base : ceiling;
  frap_set_prio(tcb, eff);
  sched_lock();
  ext->in_cs = true;
  r->owner   = tcb;
  return OK;
}

void frap_local_unlock(FAR struct frap_res *r)
{
  FAR struct tcb_s *tcb = this_task();
  FAR struct frap_task_ext *ext = frap_get_ext(tcb);

  DEBUGASSERT(r && ext && ext->in_cs && r->owner == tcb);
  ext->in_cs = false;
  sched_unlock();
  r->owner = NULL;
  frap_set_prio(tcb, (uint8_t)tcb->sched_priority); /* 若系统有保存，按需恢复 */
}

#endif /* CONFIG_FRAP */