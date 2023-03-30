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

    r = (struct run *) pa;
    if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");
    // acquiring the spinlock to get the true refcount, 
    // and then decrease the refcount and release the lock
    acquire(&kmem.lock);
    int pageNum = (uint64) r / PGSIZE; // r = pa
    if (refcounter[pageNum] < 1)
        panic("kfree panic");
    refcounter[pageNum]--;
    int count = refcounter[pageNum];
    release(&kmem.lock);

    if (count > 0) return;
    
    // Fill with junk to catch dangling refs.
    memset(pa, 0, PGSIZE);

    // acquire the lock again to add the page to the freelist
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
        int pageNum = (uint64)r / PGSIZE;
        if (refcounter[pageNum] != 0)
        {
            panic("refcounter kalloc panic");
        }
        refcounter[pageNum] = 1;
        kmem.freelist = r->next;
    }

    release(&kmem.lock);

    if (r)
        memset((char *)r, 0, PGSIZE); // fill with junk
    return (void *)r;
}

// Increase the refcount of the page
void refCountIncrement(uint64 address)
{
    // pageNum is the page number
    int pageNum = address / PGSIZE;
    if (address > PHYSTOP || refcounter[pageNum] < 1)
    {
        printf("Error in attempting to increment refcount of page %d, which has refcount %d", pageNum, refcounter[pageNum]);
        return;
    }
    // increase the refcounter
    refcounter[pageNum]++;
}

void refCountDecrement(uint64 address)
{
    // pageNum is the page number
    int pageNum = address / PGSIZE;
    if (address > PHYSTOP || refcounter[pageNum] < 1)
    {
        printf("Error in attempting to decrement refcount of page %d, which has refcount %d", pageNum, refcounter[pageNum]);
        return;
    }
    // decrease the refcounter
    refcounter[pageNum]--;
}

// Get the refcount of the page
int getRefCount(uint64 address)
{
    int pageNum = address / PGSIZE;
    if (address > PHYSTOP)
    {
        printf("Error in attempting to get refcount of page %d, which has refcount %d", pageNum, refcounter[pageNum]);
        return -1;
    }
    return refcounter[pageNum];
}
