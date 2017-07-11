#include "ns.h"

extern union Nsipc nsipcbuf;

static struct jif_pkt *pkt = (struct jif_pkt *)REQVA;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	int i, r;
	int32_t length;
	struct jif_pkt *cpkt = pkt;
	
	for(i = 0; i < 10; i++)
		if ((r = sys_page_alloc(0, (void*)((uintptr_t)pkt + i * PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
			panic("sys_page_alloc: %e", r);
	
	i = 0;
	while(1) {
		while((length = sys_netpacket_recv((void*)((uintptr_t)cpkt + sizeof(cpkt->jp_len)), PGSIZE - sizeof(cpkt->jp_len))) < 0) {
			// cprintf("len: %d\n", length);
			sys_yield();
		}

		cpkt->jp_len = length;
		ipc_send(ns_envid, NSREQ_INPUT, cpkt, PTE_P | PTE_U);
		i = (i + 1) % 10;
		cpkt = (struct jif_pkt*)((uintptr_t)pkt + i * PGSIZE);
		sys_yield();
	}	
}
