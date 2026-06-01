#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"


uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
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
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
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

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
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

// sys_flip_display: zero-copy page flip.
//
// Syscall argument 0: user virtual address of a page-aligned buffer
// that is exactly GPU_FB_PAGES (300) * PGSIZE bytes (i.e. 640x480x4 =
// 1,228,800 bytes).  The buffer must already be fully mapped in the
// calling process's address space.
//
// TODO: Students implement this syscall.
uint64
sys_flip_display(void)
{
  return -1;
}

// sys_map_display: map the GPU's kernel framebuffer pages (fb[]) directly
// into the calling process's address space with PTE_U|PTE_R|PTE_W.
//
// Syscall argument 0: desired user virtual address (must be page-aligned).
//   Pass 0 to let the kernel auto-select the next available VA above p->sz.
//
// Returns the mapped virtual address on success, (uint64)-1 on failure.


static int check_free_memory(pagetable_t pagetable, uint64 va, uint64 size)
{
    if (va >= MAXVA || va + size < va || va + size > MAXVA)
        return 0;

    for (uint64 a = va; a < va + size; a += PGSIZE) {
        pte_t *pte = walk(pagetable, a, 0);

        if (pte != 0 && (*pte & PTE_V))
            return 0;
    }

    return 1;
}


// TODO: Students implement this syscall.
uint64
sys_map_display(void)
{
  int addr;
  argint(0, &addr);
  struct proc *p = myproc();
  uint64 size = PGSIZE*GPU_FB_PAGES;
  
  if (p->display_mapped) // check if not mapped already
    return -1;

  if(addr < 0) // make sure valid addr value
    return -1;
  
  if (addr != 0 && addr % PGSIZE != 0) // make sure if addr not 0 must be page alligned
    return -1;
  
  if(addr == 0){ // auto select va
    addr = PGROUNDUP(p->sz);
    int found = 0;
    while(addr + size > addr && addr + size <= MAXVA && !found){ // search for a valid adress
      if(check_free_memory(p->pagetable, addr, size))
        found = 1;
      addr += PGSIZE;
    }
    if(!found){
      return -1;
    }
  }
  else {
    if(!check_free_memory(p->pagetable, addr, size))
      return -1;
  }

  if(map_frame_buffer_map_display(p->pagetable, addr) < 0)
    return -1;
  
  p->display_mapped = 1;
  p->display_va = addr;
  return addr;
}

void unmap_display(pagetable_t pagetable, uint64 va)
{
    uvmunmap(pagetable, va, GPU_FB_PAGES, 0);
}
