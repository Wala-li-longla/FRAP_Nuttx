/* sched/frap/frap_queue.c */
#include <nuttx/config.h>
#ifdef CONFIG_FRAP

#include <nuttx/queue.h>
#include <nuttx/spinlock.h>
#include "frap_internal.h"

void frap_queue_init(FAR struct frap_res *r)
{
  r->sl = SP_UNLOCKED;
  dq_init(&r->fifo);
}

void frap_queue_enqueue_tail(FAR struct frap_res *r, FAR struct frap_waiter *w)
{
  if (!w->enqueued)
    {
      dq_addlast(&w->node, &r->fifo);
      w->enqueued = true;
    }
}

void frap_queue_enqueue_head_if_needed(FAR struct frap_res *r, FAR struct frap_waiter *w)
{
  if (!w->enqueued)
    {
      dq_addfirst(&w->node, &r->fifo);
      w->enqueued = true;
    }
}

void frap_queue_remove(FAR struct frap_res *r, FAR struct frap_waiter *w)
{
  if (w->enqueued)
    {
      dq_rem(&w->node, &r->fifo);
      w->enqueued = false;
    }
}

FAR struct frap_waiter *frap_queue_peek_head(FAR struct frap_res *r)
{
  FAR dq_entry_t *e = dq_peek(&r->fifo);
  return e ? (FAR struct frap_waiter *)e : NULL;
}

#endif /* CONFIG_FRAP */
