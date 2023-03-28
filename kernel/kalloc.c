// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

uint64 MAX_PAGES = 0;
uint64 FREE_PAGES = 0;

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run
{
    struct run *next;
};

struct
{
    struct spinlock lock;
    struct run *freelist;
} kmem;

void kinit()
{
    initlock(&kmem.lock, "kmem");
    freerange(end, (void *)PHYSTOP);
    MAX_PAGES = FREE_PAGES;
}

int refcounter[PHYSTOP / PGSIZE];

void freerange(void *pa_start, void *pa_end)
{
    char *p;
    p = (char *)PGROUNDUP((uint64)pa_start);
    for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    {
        refcounter[(uint64)p / PGSIZE] = 1;
        kfree(p);
    }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
    struct run *r;
    r = (struct run *)pa;
    if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");
    // acquiring the spinlock to get the true refcount, 
    // and then decrease the refcount and release the lock
    acquire(&kmem.lock);
    int pn = (uint64)r / PGSIZE;
    if (refcounter[pn] < 1)
        panic("kfree panic");
    refcounter[pn]--;
    int tmp = refcounter[pn];
    release(&kmem.lock);

    if (tmp > 0) {
        return;
    }
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
    struct run *r;

    acquire(&kmem.lock);
    r = kmem.freelist;

    if (r)
    {
        int pn = (uint64)r / PGSIZE;
        if (refcounter[pn] != 0)
        {
            panic("refcounter kalloc panic");
        }
        refcounter[pn] = 1;
        kmem.freelist = r->next;
    }

    release(&kmem.lock);

    if (r)
        memset((char *)r, 5, PGSIZE); // fill with junk
    return (void *)r;
}

// Increase the refcount of the page
void refCountIncrement(uint64 pa)
{
    // pn is the page number
    int pn = pa / PGSIZE;
    if (pa > PHYSTOP || refcounter[pn] < 1)
    {
        panic("Panic when increasing ref counter");
    }
    // increase the refcounter
    refcounter[pn]++;
}

// Get the refcount of the page
int getRefCount(uint64 pa)
{
    int pn = pa / PGSIZE;
    if (pa > PHYSTOP)
    {
        panic("Panic when getting ref counter");
    }
    return refcounter[pn];
}
