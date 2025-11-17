/* sched/frap/frap_schedhook.c */
#include <nuttx/config.h>
#include "sched/sched.h"
#include <debug.h>
#ifdef CONFIG_FRAP

#include <nuttx/sched.h>
#include <nuttx/spinlock.h>
#include <sched.h>
#include "frap_internal.h"
#include "frap_compat.h"

/* 在上下文切换时由调度器调用：
 * newtcb 抢占 oldtcb。如果 oldtcb 正在“自旋等待”（未入临界段），
 * 则从 FIFO 摘除其请求、标记 cancelled，并恢复其基准优先级（R3）。
 */
void frap_on_preempt(FAR struct tcb_s *oldtcb, FAR struct tcb_s *newtcb)
{
  FAR struct frap_task_ext *ext;

  if (!oldtcb || !newtcb) return;

  /* 仅在优先级真正上升（抢占）时处理；同优先级轮转不处理 */
  if (newtcb->sched_priority <= oldtcb->sched_priority)
    {
      return;
    }

  ext = frap_get_ext(oldtcb);
  if (!ext) return;

  /* 仅在自旋阶段（未进CS）才取消请求 */
  if (ext->waiting_res && !ext->in_cs)
    {
      FAR struct frap_res *r = ext->waiting_res;
      irqstate_t flags = spin_lock_irqsave(&r->sl);

      frap_queue_remove(r, &ext->waiter); /* 如在队中则摘除 */
      ext->waiter.cancelled = true;

      spin_unlock_irqrestore(&r->sl, flags);

      /* 恢复为基准优先级 P_i（R3） */
      frap_set_prio(oldtcb, ext->waiter.base_prio);
      /* 不立即重新入队；等 oldtcb 再次运行时在 frap_lock 循环尾插 */
      /* ------- 日志：被更高优先级任务抢占，取消本次请求 ------- */
      sinfo("FRAP cancel: old=%d (prio %d->%d) by new=%d, resid=%u\n",
            oldtcb->pid,
            (int)ext->waiter.spin_prio, (int)ext->waiter.base_prio,
            newtcb->pid, (unsigned)r->id);
      /* --------------------------------------------------------- */
    }
}

#endif /* CONFIG_FRAP */