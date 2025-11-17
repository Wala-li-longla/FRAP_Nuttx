/* sched/frap/frap_core.c */
#include <nuttx/config.h>
#ifdef CONFIG_FRAP

#include <string.h>
#include <nuttx/spinlock.h>
#include <nuttx/sched.h>
#include "frap_internal.h"
// #include "sched/sched.h"   /* 提供 this_task、tcb_s、nxsched_* */


struct frap_ext_entry
{
  bool                 inuse;
  FAR struct tcb_s    *tcb;
  struct frap_task_ext ext;
};

static struct frap_ext_entry g_ext_pool[CONFIG_FRAP_MAX_TASKS];
static spinlock_t g_ext_lock = SP_UNLOCKED;

FAR struct frap_task_ext *frap_get_ext(FAR struct tcb_s *tcb)
{
  irqstate_t flags = spin_lock_irqsave(&g_ext_lock);

  /* 找现有 */
  for (int i = 0; i < CONFIG_FRAP_MAX_TASKS; i++)
    {
      if (g_ext_pool[i].inuse && g_ext_pool[i].tcb == tcb)
        {
          spin_unlock_irqrestore(&g_ext_lock, flags);
          return &g_ext_pool[i].ext;
        }
    }

  /* 分配一个 */
  for (int i = 0; i < CONFIG_FRAP_MAX_TASKS; i++)
    {
      if (!g_ext_pool[i].inuse)
        {
          memset(&g_ext_pool[i], 0, sizeof(g_ext_pool[i]));
          g_ext_pool[i].inuse = true;
          g_ext_pool[i].tcb   = tcb;
          spin_unlock_irqrestore(&g_ext_lock, flags);
          return &g_ext_pool[i].ext;
        }
    }

  spin_unlock_irqrestore(&g_ext_lock, flags);
  return NULL; /* 池耗尽：可提升为ASSERT或返回错误 */
}

/* 资源初始化 */
int frap_res_init(FAR struct frap_res *r, uint32_t id, bool global)
{
  if (!r) return -EINVAL;
  r->sl        = SP_UNLOCKED;
  r->owner     = NULL;
  dq_init(&r->fifo);
  r->id        = id;
  r->is_global = global;
  r->ceiling   = 0;
  return OK;
}

#endif /* CONFIG_FRAP */
