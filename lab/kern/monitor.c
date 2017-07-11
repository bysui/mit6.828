// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line
#define TESTERR(a) {if (a) goto ERR;}  //test showmapping argument 

struct Command {
	const char *name;
	const char *desc;
	const char *usage;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", "help [cmd]\nshow usage of cmd", mon_help },
	{ "kerninfo", "Display information about the kernel","kernel", mon_kerninfo },
	{ "backtrace", "Display the backtrace of stacks", "backstrace", mon_backtrace },
	{ "showmapping", "Display the physical page mappings of special virtual address", "showmapping [begin] [end]\nshow the physical page mappings of virtual address form begin to end", mon_showmapping },
	{ "setpri", "Set the perimissions of any mapping in the current address space page", "setpri [address] [+-][pri]\np\\P:Present\nw\\W:Writeable\nu\\U:User\nt\\T:Write-Through\nc\\C:Cache-Disable\na\\A:Accessed\nd\\D:Dirty\ng\\G:Global", mon_setpri },
	{ "dump", "Dump the contentss of a range of memory given either a virtual or physical address range", "dump -[pv] [begin] [end]\nBy default dump virtual address, use -p to present physical address, -v to present virtual address\n", mon_dump }
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;
	
	if (argc == 2) {
		for (i = 0; i < NCOMMANDS; i++)
			if (strcmp(argv[1], commands[i].name) == 0)
				break;
		if (i >= NCOMMANDS)
			cprintf("Command \"%s\" hasn't been implemented!\n", argv[1]);
		else
			cprintf("%s\nUsage: %s\n", commands[i].desc, commands[i].usage);
	}
	else {
		for (i = 0; i < NCOMMANDS; i++)
			cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	}
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// The ebp value of the prgram, which calls the mon_backtrace.
	int regebp = read_ebp();
	int *ebp = (int *)regebp;

	cprintf("Stack backtrace:\n");
	
	while ((int)ebp != 0x0) {
		cprintf("  ebp %08x", (int)ebp);
		cprintf("  eip %08x", *(ebp+1));
		cprintf("  args");
		cprintf(" %08x", *(ebp+2));
		cprintf(" %08x", *(ebp+3));
		cprintf(" %08x", *(ebp+4));
		cprintf(" %08x", *(ebp+5));
		cprintf(" %08x\n", *(ebp+6));

		//printf epi address debuginfo: file, line, function, offset
		struct Eipdebuginfo info;
		debuginfo_eip(*(ebp+1), &info);
		cprintf("%s:%d: ", info.eip_file, info.eip_line);
		cprintf("%.*s+%d\n", info.eip_fn_namelen, info.eip_fn_name, *(ebp+1)-info.eip_fn_addr);
		
		ebp = (int *)(*ebp);
	}
	return 0;
}


int
mon_showmapping(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t begin, end;
	char *endptrb, *endptre;
	
	
	if (argc == 2) {  //showmapping [begin, begin+PGSIZE]
		begin = ROUNDDOWN((uint32_t) strtol(argv[1], &endptrb, 0), PGSIZE);
		end = begin + PGSIZE;
		TESTERR(*endptrb != '\0');
	}
	else if (argc == 3) {   //showmapping [begin, end]
		begin = ROUNDDOWN((uint32_t) strtol(argv[1], &endptrb, 0), PGSIZE);
		end = ROUNDUP((uint32_t) strtol(argv[2], &endptre, 0), PGSIZE);
		TESTERR(*endptrb != '\0' || *endptre != '\0');
	}
	else
		goto ERR;
	
	cprintf("\tVirtual\tPhysical\t\tPriority\t\tRefer\n");
	for (; begin <  end; begin += PGSIZE) {
		struct PageInfo *pp;  
		pte_t *ppte;
		char buf[13];
		
		pp = page_lookup(kern_pgdir, (void *) begin, &ppte);
		if (pp == NULL || *ppte == 0)
			cprintf("\t%08x\t%s\t%s\t\t%d\n", begin, "Not mapping", "None", 0);
		else
			cprintf("\t%08x\t%08x\t\t%s\t%d\n", begin, page2pa(pp), pagepri2str(*ppte, buf), pp->pp_ref); 
	}

	return 0;

ERR:
	cprintf("Wrong parameters!\n");
	return 0;
}

//It's just change the pripority, don't have any way to adjust to the change.
int
mon_setpri(int argc, char **argv, struct Trapframe *t)
{
	uint32_t pte, begin, pri;
	pte_t *ppte;
	struct PageInfo *pp;
	char *endptr;
	char buf_old[13], buf_new[13];

	TESTERR(argc != 3 && argc != 4);

	begin = ROUNDDOWN((uint32_t) strtol(argv[1], &endptr, 0), PGSIZE);
	TESTERR(*endptr != '\0');
	
	pp = page_lookup(kern_pgdir, (void *) begin, &ppte);
	if (pp == NULL || *ppte == 0) {
		cprintf("\tVirtual\tPhysical\tOld Priority\tNew Priority\tRefer\n");  
		cprintf("\t%08x\t%s\t%s\t\t%s\t\t%d\n", begin, "No Mapping", "None", "None", 0);
		return 0;
	}
	pte = *ppte;
	
	if (argc == 3) {   //setpri address [+-]pri
		if (*argv[2] == '+') 
			*ppte |= str2pagepri(argv[2] + 1);
		else if (*argv[2] == '-')
			*ppte &= ~str2pagepri(argv[2] + 1);
		else{
			pri = strtol(argv[2], &endptr, 0);
			TESTERR(*endptr != '\0');
			*ppte = PTE_ADDR(*ppte) | pri;
		}
	}
	else {     //setpri address +pri -pri
		if (*argv[2] == '+' && *argv[3] == '-') 
			*ppte = (*ppte | str2pagepri(argv[2] + 1)) & (~str2pagepri(argv[3] + 1));
		else if(*argv[2] == '-' && *argv[3] == '+')
			*ppte = (*ppte & ~str2pagepri(argv[2] + 1)) | str2pagepri(argv[3] + 1);
		else
			goto ERR;
	}

	cprintf("\tVirtual\tPhysical\tOld Priority\tNew Priority\tRefer\n");  
	cprintf("\t%08x\t%08x\t%s\t%s\t%d\n", begin, page2pa(pp), pagepri2str(pte,buf_old), pagepri2str(*ppte, buf_new), pp->pp_ref);
        return 0;

ERR:
	cprintf("Wrong parameters!\n");
	return 0;
}

int
mon_dump(int argc, char **argv, struct Trapframe *t)
{
	uint32_t begin, end;
	char *endptrb, *endptre;
	int flag = 0;

	TESTERR(argc != 2 && argc != 3 && argc != 4);

	if (argc == 2) {  //dump addr (virtual)
		begin = strtol(argv[1], &endptrb, 0);
		end = begin + 16;
		TESTERR(*endptrb != '\0');
	}
	else if (argc == 3) {   
		if (*argv[1] == '-') {
			if (argv[1][1] == 'p') //dump -p addr (physical)
				flag = 1;
			else if (argv[1][1] == 'v') //dump -v addr (virtual)
				flag = 0;
			else 
				goto ERR;
			begin = strtol(argv[2], &endptrb, 0);
			end = begin + 16;
			TESTERR(*endptrb != '\0');
		}
		else {   //dump begin end (virtual)
			begin = strtol(argv[1], &endptrb, 0);
			end = strtol(argv[2], &endptre, 0);
			TESTERR(*endptrb != '\0' || *endptre != '\0');
		}
	}
	else {
		if (*argv[1] == '-') {
			if (argv[1][1] == 'p') //dump -p addr (physical)
				flag = 1;
			else if (argv[1][1] == 'v') //dump -v addr (virtual)
				flag = 0;
			else 
				goto ERR;
			begin = strtol(argv[2], &endptrb, 0);
			end = strtol(argv[2], &endptre, 0);
			TESTERR(*endptrb != '\0' || *endptre != '\0');
		}
		else
			goto ERR;
	}

	if (flag) {   //process physical address
		if ((PGNUM(begin) >= npages) || (PGNUM(end) >= npages)) {
			cprintf("Over than max physical memory!\n");
			return 0;
		}
		begin = (uint32_t) KADDR(begin);
		end = (uint32_t) KADDR(end);
	}

	cprintf("\tVirtual\tPhysical\tMemory\n");  
	while (begin < end) {
		int i;
		pte_t *ppte;
		
		cprintf("\t%08x\t", begin);
		if (page_lookup(kern_pgdir, (void *) begin, &ppte) == NULL || *ppte == 0) {
			cprintf("No Mapping\n");
			begin += PGSIZE - begin % PGSIZE;
			continue;
		}

		cprintf("%08x\t", PTE_ADDR(*ppte) | PGOFF(begin));
		for (i = 0; i < 16; i++, begin++) 
			cprintf("%02x ", *(unsigned char *) begin);
		cprintf("\n");
	}
	return 0;

ERR:
	cprintf("Wrong parameters!\n");
	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
