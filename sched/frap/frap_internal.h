/* sched/frap/frap_internal.h */
#pragma once
#include <nuttx/config.h>

#ifdef CONFIG_FRAP

#include <nuttx/sched.h>
#include <nuttx/queue.h>
#include <nuttx/spinlock.h>
#include <stdint.h>
#include <stdbool.h>
#include <nuttx/frap.h>

struct frap_task_ext
{
  FAR struct frap_res *waiting_res; /* 正在自旋等待的资源（否则NULL） */
  struct frap_waiter   waiter;      /* 内嵌等待者节点 */
  bool                 in_cs;       /* 是否在临界段 */
  uint8_t              saved_prio;  /* 备用 */
};

/* 任务扩展注册/获取（实现见 frap_core.c） */
FAR struct frap_task_ext *frap_get_ext(FAR struct tcb_s *tcb);

/* FIFO 操作（实现见 frap_queue.c） */
void frap_queue_init(FAR struct frap_res *r);
void frap_queue_enqueue_tail(FAR struct frap_res *r, FAR struct frap_waiter *w);
void frap_queue_enqueue_head_if_needed(FAR struct frap_res *r, FAR struct frap_waiter *w);
void frap_queue_remove(FAR struct frap_res *r, FAR struct frap_waiter *w);
FAR struct frap_waiter *frap_queue_peek_head(FAR struct frap_res *r);

#endif /* CONFIG_FRAP */
