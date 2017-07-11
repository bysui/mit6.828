// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	int r;
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if ((err & FEC_WR) == 0 || (uvpt[PGNUM(addr)] & PTE_COW) == 0)
		panic("pgfault: it's not writable or attempt to access a non-cow page!");
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	envid_t envid = sys_getenvid();
	if ((r = sys_page_alloc(envid, (void *)PFTEMP, PTE_P | PTE_W | PTE_U)) < 0)
		panic("pgfault: page allocation failed %e", r);

	addr = ROUNDDOWN(addr, PGSIZE);
	memmove(PFTEMP, addr, PGSIZE);
	if ((r = sys_page_unmap(envid, addr)) < 0)
		panic("pgfault: page unmap failed %e", r);
	if ((r = sys_page_map(envid, PFTEMP, envid, addr, PTE_P | PTE_W |PTE_U)) < 0)
		panic("pgfault: page map failed %e", r);
	if ((r = sys_page_unmap(envid, PFTEMP)) < 0)
		panic("pgfault: page unmap failed %e", r);
	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	
	// LAB 4: Your code here.
	void *addr;
	pte_t pte;
	int perm;

	addr = (void *)((uint32_t)pn * PGSIZE);
	pte = uvpt[pn];
	if (pte & PTE_SHARE) {
		if ((r = sys_page_map(sys_getenvid(), addr, envid, addr, pte & PTE_SYSCALL)) < 0) {
			panic("duppage: page mapping failed %e", r);
			return r;
		}
	}
	else {
		perm = PTE_P | PTE_U;
		if ((pte & PTE_W) || (pte & PTE_COW)) 
			perm |= PTE_COW;
		if ((r = sys_page_map(thisenv->env_id, addr, envid, addr, perm)) < 0) {
			panic("duppage: page remapping failed %e", r);
			return r;
		}
		if (perm & PTE_COW) {
			if ((r = sys_page_map(thisenv->env_id, addr, thisenv->env_id, addr, perm)) < 0) {
				panic("duppage: page remapping failed %e", r);
				return r;
			}
		}
	}


	// panic("duppage not implemented");
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t envid;
	uint32_t addr;
	int i, j, pn, r;
	extern void _pgfault_upcall(void);

	set_pgfault_handler(pgfault);
	if ((envid = sys_exofork()) < 0) {
		panic("sys_exofork failed: %e", envid);
		return envid;
	}
	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (i = PDX(UTEXT); i < PDX(UXSTACKTOP); i++) {
		if (uvpd[i] & PTE_P) {
			for (j = 0; j < NPTENTRIES; j++) {
				pn = PGNUM(PGADDR(i, j, 0));
				if (pn == PGNUM(UXSTACKTOP - PGSIZE))
					break;
				if (uvpt[pn] & PTE_P)
					duppage(envid, pn);
			}
		}
	
	}
	
	if ((r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W)) < 0) {
		panic("fork: page alloc failed %e", r);
		return r;
	}
	if ((r = sys_page_map(envid, (void *)(UXSTACKTOP - PGSIZE), thisenv->env_id, PFTEMP, PTE_P | PTE_U | PTE_W)) < 0) {
		panic("fork: page map failed %e", r);
		return r;
	}
	memmove((void *)(UXSTACKTOP - PGSIZE), PFTEMP, PGSIZE);
	if ((r = sys_page_unmap(thisenv->env_id, PFTEMP)) < 0) {
		panic("fork: page unmap failed %e", r);
		return r;
	}
	sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0) {
		panic("fork: set child env status failed %e", r);
		return r;
	}

	return envid;
	// panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	envid_t envid;
	uint32_t addr;
	int i, j, pn, r;
	extern void _pgfault_upcall(void);

	set_pgfault_handler(pgfault);
	if ((envid = sys_exofork()) < 0) {
		panic("sys_exofork failed: %e", envid);
		return envid;
	}
	if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (i = PDX(UTEXT); i < PDX(UXSTACKTOP); i++) {
		if (uvpd[i] & PTE_P) {
			for (j = 0; j < NPTENTRIES; j++) {
				pn = PGNUM(PGADDR(i, j, 0));
				if (pn == PGNUM(UXSTACKTOP - PGSIZE))
					break;
				if (pn == PGNUM(USTACKTOP - PGSIZE)) {
					duppage(envid, pn);
					continue;
				}
				if (uvpt[pn] & PTE_P) {
					if ((r = sys_page_map(thisenv->env_id, (void *)PGADDR(i, j, 0), envid, (void *)PGADDR(i, j, 0), uvpt[pn] & 0xFFF)) < 0) {
						panic("fork: page map failed %e", r);
						return r;
					}	
				}
			}
		}
	
	}
	
	if ((r = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W)) < 0) {
		panic("fork: page alloc failed %e", r);
		return r;
	}
	if ((r = sys_page_map(envid, (void *)(UXSTACKTOP - PGSIZE), thisenv->env_id, PFTEMP, PTE_P | PTE_U | PTE_W)) < 0) {
		panic("fork: page map failed %e", r);
		return r;
	}
	memmove((void *)(UXSTACKTOP - PGSIZE), PFTEMP, PGSIZE);
	if ((r = sys_page_unmap(thisenv->env_id, PFTEMP)) < 0) {
		panic("fork: page unmap failed %e", r);
		return r;
	}
	sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0) {
		panic("fork: set child env status failed %e", r);
		return r;
	}

	return envid;
	// panic("sfork not implemented");
	// return -E_INVAL;
}
