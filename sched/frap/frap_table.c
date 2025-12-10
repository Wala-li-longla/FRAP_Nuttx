/* sched/frap/frap_table.c */
#include <nuttx/config.h>
#ifdef CONFIG_FRAP

#include <string.h>
#include <errno.h>
#include <nuttx/sched.h>
#include <nuttx/frap.h>

struct frap_tbl_ent
{
  bool     inuse;
  pid_t    pid;
  uint32_t resid;
  uint8_t  spin_prio;
};

static struct frap_tbl_ent g_tbl[CONFIG_FRAP_TABLE_SIZE];

int frap_set_spin_prio(pid_t pid, uint32_t resid, uint8_t spin_prio)
{
  /* update or insert */
  for (int i = 0; i < CONFIG_FRAP_TABLE_SIZE; i++)
    {
      if (g_tbl[i].inuse && g_tbl[i].pid == pid && g_tbl[i].resid == resid)
        {
          g_tbl[i].spin_prio = spin_prio;
          return OK;
        }
    }
  for (int i = 0; i < CONFIG_FRAP_TABLE_SIZE; i++)
    {
      if (!g_tbl[i].inuse)
        {
          g_tbl[i].inuse    = true;
          g_tbl[i].pid      = pid;
          g_tbl[i].resid    = resid;
          g_tbl[i].spin_prio= spin_prio;
          return OK;
        }
    }
  return -ENOSPC;
}

int frap_get_spin_prio(pid_t pid, uint32_t resid)
{
  for (int i = 0; i < CONFIG_FRAP_TABLE_SIZE; i++)
  {
    if (g_tbl[i].inuse && g_tbl[i].pid == pid && g_tbl[i].resid == resid)
    {
      return g_tbl[i].spin_prio;
    }
  }
  return -ENOENT;
}

#endif /* CONFIG_FRAP */
