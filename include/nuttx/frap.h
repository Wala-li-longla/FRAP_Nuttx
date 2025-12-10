/* include/nuttx/frap.h
 *
 * FRAP: Flexible Resource Accessing Protocol for SMP RTOS (NuttX)
 * Minimal runtime implementation: per-resource FIFO spin, non-preemptive CS,
 * cancel-on-preempt while spinning, and per(task,resource) spin priority.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <nuttx/config.h>

#ifdef CONFIG_FRAP

#include <stdint.h>
#include <stdbool.h>
#include <nuttx/queue.h>
#include <nuttx/spinlock.h>
#include <nuttx/sched.h>

#ifndef CONFIG_FRAP_MAX_TASKS
#  define CONFIG_FRAP_MAX_TASKS 64
#endif

#ifndef CONFIG_FRAP_TABLE_SIZE
#  define CONFIG_FRAP_TABLE_SIZE 64
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 全局/本地共享资源句柄（一个资源一个FIFO） */
struct frap_res
{
  spinlock_t        sl;        /* 保护本结构与 fifo */
  FAR struct tcb_s *owner;     /* 当前持有者（在临界段时） */
  dq_queue_t        fifo;      /* 等待者 FIFO 队列 */
  uint32_t          id;        /* 资源 ID（由调用者指定） */
  bool              is_global; /* true: 走FRAP; false: 走PCP（可选） */
  uint8_t           ceiling;   /* 本地资源天花板（可选） */
};

/* 等待节点（每个任务固定一个，避免动态分配） */
struct frap_waiter
{
  dq_entry_t        node;
  FAR struct tcb_s *tcb;
  uint8_t           base_prio;   /* P_i */
  uint8_t           spin_prio;   /* P_i^k */
  volatile bool     enqueued;    /* 是否在 FIFO 中 */
  volatile bool     cancelled;   /* 是否被抢占钩子取消 */
};

/* 对外 API */
int  frap_res_init(FAR struct frap_res *r, uint32_t id, bool global);

/* 获取/释放：spin_prio 由调用方提供（或用 frap_get_spin_prio 查询） */
int  frap_lock(FAR struct frap_res *r);
void frap_unlock(FAR struct frap_res *r);

/* （可选）本地资源PCP封装 */
int  frap_local_lock(FAR struct frap_res *r, uint8_t ceiling);
void frap_local_unlock(FAR struct frap_res *r);

/* (task,res)->P_i^k 表接口（简单表，无MCMF） */
int  frap_set_spin_prio(pid_t pid, uint32_t resid, uint8_t spin_prio);
int  frap_get_spin_prio(pid_t pid, uint32_t resid);

/* 调度器抢占钩子（在上下文切换时由调度器调用） */
void frap_on_preempt(FAR struct tcb_s *oldtcb, FAR struct tcb_s *newtcb);

#ifdef __cplusplus
}
#endif
#endif /* CONFIG_FRAP */
