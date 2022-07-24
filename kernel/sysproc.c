#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  //backtrace();
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  uint64 va, dstva;
  int n;

  if(argaddr(0, &va) < 0)
    return -1;
  if(argint(1, &n) < 0 && n > 32) // uint abits has 32 bits
    return -1;
  if(argaddr(2, &dstva) < 0)
    return -1;

  pagetable_t pagetable = myproc()->pagetable;
  pte_t *pte;
  uint64 pa, a;
  uint abits = 0;

  for(int i=0; i < n; i++){
    a = va + i * PGSIZE;
    if((pte = walk(pagetable, a, 0)) == 0)
      return -1;
    if(*pte & PTE_A)
      abits |= (1<<i);
    pa = PTE2PA(*pte);
    *pte = PA2PTE(pa) | (PTE_FLAGS(*pte) & (0x3FF - PTE_A));
  }

  if(copyout(pagetable, dstva, (char *)&abits, 4) < 0)
    return -1;

  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sigalarm(void)
{
  int ticks;
  void (*handler)();

  if(argint(0, &ticks) < 0)
    return -1;
  if(argaddr(1, &handler) < 0)
    return -1;

  myproc()->alarmticks = ticks;
  myproc()->alarmhandler = handler;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  memmove(p->trapframe, &p->restore, sizeof(p->restore));
  p->handling = 0;
  return 0;
}
