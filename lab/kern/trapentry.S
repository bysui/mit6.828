/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 * Use ec = 1 for traps where the CPU automatically push an error code and ec = 0 for not.
 * Use user = 1 for a syscall; user = 0 for a normal trap.
 */
#define TRAPHANDLER(name, num, ec, user)				\
.text;									\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	.if ec==0;							\
		pushl $0;						\
	.endif;								\
	pushl $(num);							\
	jmp _alltraps;							\
.data;									\
	.long  name, num, user					

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.data
	.globl  entry_data
	entry_data:
.text
TRAPHANDLER(divide_entry, T_DIVIDE, 0, 0);
TRAPHANDLER(debug_entry, T_DEBUG, 0, 0);
TRAPHANDLER(nmi_entry, T_NMI, 0, 0);
TRAPHANDLER(brkpt_entry, T_BRKPT, 0, 1);
TRAPHANDLER(oflow_entry, T_OFLOW, 0, 0);
TRAPHANDLER(bound_entry, T_BOUND, 0, 0);
TRAPHANDLER(illop_entry, T_ILLOP, 0, 0);
TRAPHANDLER(device_entry, T_DEVICE, 0, 0);	
TRAPHANDLER(dblflt_entry, T_DBLFLT, 1, 0);
TRAPHANDLER(tts_entry, T_TSS, 1, 0);
TRAPHANDLER(segnp_entry, T_SEGNP, 1, 0);
TRAPHANDLER(stack_entry, T_STACK, 1, 0);
TRAPHANDLER(gpflt_entry, T_GPFLT, 1, 0);
TRAPHANDLER(pgflt_entry, T_PGFLT, 1, 0);
TRAPHANDLER(fperr_entry, T_FPERR, 0, 0);
TRAPHANDLER(align_entry, T_ALIGN, 1, 0);
TRAPHANDLER(mchk_entry, T_MCHK, 0, 0);
TRAPHANDLER(simderr_entry, T_SIMDERR, 0, 0);
TRAPHANDLER(irq0_entry, IRQ_OFFSET + 0, 0, 0);
TRAPHANDLER(irq1_entry, 33, 0, 0);
TRAPHANDLER(irq2_entry, 34, 0, 0);
TRAPHANDLER(irq3_entry, 35, 0, 0);
TRAPHANDLER(irq4_entry, 36, 0, 0);
TRAPHANDLER(irq5_entry, 37, 0, 0);
TRAPHANDLER(irq6_entry, 38, 0, 0);
TRAPHANDLER(irq7_entry, 39, 0, 0);
TRAPHANDLER(irq8_entry, 40, 0, 0);
TRAPHANDLER(irq9_entry, 41, 0, 0);
TRAPHANDLER(irq10_entry, 42, 0, 0);
TRAPHANDLER(irq11_entry, 43, 0, 0);
TRAPHANDLER(irq12_entry, 44, 0, 0);
TRAPHANDLER(irq13_entry, 45, 0, 0);
TRAPHANDLER(irq14_entry, 46, 0, 0);
TRAPHANDLER(irq15_entry, 47, 0, 0);
TRAPHANDLER(syscall_entry, T_SYSCALL, 0, 1);
.data
	.long 0, 0, 0   // interupt end identify
/*
 * Lab 3: Your code here for _alltraps
 */
.text
_alltraps:
	pushl %ds
	pushl %es
	pushal
	
	// save FPU
	//subl $524, %esp
	//fxsave (%esp)

	movl $GD_KD, %eax
	movl %eax, %ds
	movl %eax, %es

	push %esp
	call trap	
