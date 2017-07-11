/* See COPYRIGHT for copyright information. */

#ifndef JOS_KERN_PMAP_H
#define JOS_KERN_PMAP_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

#include <inc/memlayout.h>
#include <inc/assert.h>
struct Env;

extern char bootstacktop[], bootstack[];

extern struct PageInfo *pages;
extern size_t npages;

extern pde_t *kern_pgdir;


/* This macro takes a kernel virtual address -- an address that points above
 * KERNBASE, where the machine's maximum 256MB of physical memory is mapped --
 * and returns the corresponding physical address.  It panics if you pass it a
 * non-kernel virtual address.
 */
#define PADDR(kva) _paddr(__FILE__, __LINE__, kva)

static inline physaddr_t
_paddr(const char *file, int line, void *kva)
{
	if ((uint32_t)kva < KERNBASE)
		_panic(file, line, "PADDR called with invalid kva %08lx", kva);
	return (physaddr_t)kva - KERNBASE;
}

/* This macro takes a physical address and returns the corresponding kernel
 * virtual address.  It panics if you pass an invalid physical address. */
#define KADDR(pa) _kaddr(__FILE__, __LINE__, pa)

static inline void*
_kaddr(const char *file, int line, physaddr_t pa)
{
	if (PGNUM(pa) >= npages)
		_panic(file, line, "KADDR called with invalid pa %08lx", pa);
	return (void *)(pa + KERNBASE);
}


enum {
	// For page_alloc, zero the returned physical page.
	ALLOC_ZERO = 1<<0,
};

void	mem_init(void);

void	page_init(void);
struct PageInfo *page_alloc(int alloc_flags);
void	page_free(struct PageInfo *pp);
int	page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm);
void	page_remove(pde_t *pgdir, void *va);
struct PageInfo *page_lookup(pde_t *pgdir, void *va, pte_t **pte_store);
void	page_decref(struct PageInfo *pp);

void	tlb_invalidate(pde_t *pgdir, void *va);

void *	mmio_map_region(physaddr_t pa, size_t size);

int	user_mem_check(struct Env *env, const void *va, size_t len, int perm);
void	user_mem_assert(struct Env *env, const void *va, size_t len, int perm);

static inline physaddr_t
page2pa(struct PageInfo *pp)
{
	return (pp - pages) << PGSHIFT;
}

static inline struct PageInfo*
pa2page(physaddr_t pa)
{
	if (PGNUM(pa) >= npages)
		panic("pa2page called with invalid pa");
	return &pages[PGNUM(pa)];
}

static inline void*
page2kva(struct PageInfo *pp)
{
	return KADDR(page2pa(pp));
}

static inline char*
pagepri2str(pte_t pte, char *buf)
{
	int i;
	static const char *str[] = { "_________SR_", "AVLGPDACTUWP" }; 
	
	for (i = 0; i < 12; i++) 
		buf[i] = str[pte >> (11 - i) & 0x1][i];
	buf[i] = '\0';

	return buf;
}

static inline int
str2pagepri(const char *buf)
{
	int pri = 0;
	while (*buf != '\0') {
		switch (*buf++) {
			case 'p':
			case 'P':
				pri |= PTE_P;
				break;
			case 'w':
			case 'W':
				pri |= PTE_W;
				break;
			case 'u':
			case 'U':
				pri |= PTE_U;
				break;
			case 't':
			case 'T':
				pri |= PTE_PWT;
				break;
			case 'c':
			case 'C':
				pri |= PTE_PCD;
				break;
			case 'a':
			case 'A':
				pri |= PTE_A;
				break;
			case 'd':
			case 'D':
				pri |= PTE_D;
				break;
			/*
			case 's':
			case 'S':
				pri |= PTE_PS;
				break;
			*/
			case 'g':
			case 'G':
				pri |= PTE_G;
				break;
			default:
				break;
		}
	
	}
	return pri;
}

pte_t *pgdir_walk(pde_t *pgdir, const void *va, int create);

#endif /* !JOS_KERN_PMAP_H */
