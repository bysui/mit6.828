#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define PCI_E1000_VENDOR	0x8086
#define PCI_E1000_DEVICE	0x100E

/* Register Set. (82543, 82544)
 *  
 * Registers are defined to be 32 bits and  should be accessed as 32 bit values.
 * These registers are physically located on the NIC, but are mapped into the
 * host memory address space.
 *     
 * RW - register is both readable and writable
 * RO - register is read only
 * WO - register is write only
 * R/clr - register is read only and is cleared when read
 * A - register array
 */
#define CTRL     0x00000  /* Device Control - RW */
#define CTRL_DUP 0x00004  /* Device Control Duplicate (Shadow) - RW */
#define STATUS   0x00008  /* Device Status - RO */
#define EECD     0x00010  /* EEPROM/Flash Control - RW */
#define EERD     0x00014  /* EEPROM Read - RW */

#define TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define TDBAH    0x03804  /* TX Descriptor Base Address High - RW */
#define TDLEN    0x03808  /* TX Descriptor Length - RW */
#define TDH      0x03810  /* TX Descriptor Head - RW */
#define TDT      0x03818  /* TX Descripotr Tail - RW */
#define TCTL     0x00400  /* TX Control - RW */
#define TIPG     0x00410  /* TX Inter-packet gap -RW */

#define	RA       0x05400  /* Receive Address - RW Array */
#define MTA      0x05200  /* Multicast Table Array - RW Array */
#define RDBAL    0x02800  /* RX Descriptor Base Address Low - RW */
#define RDBAH    0x02804  /* RX Descriptor Base Address High - RW */
#define RDLEN    0x02808  /* RX Descriptor Length - RW */
#define RDH      0x02810  /* RX Descriptor Head - RW */
#define RDT      0x02818  /* RX Descriptor Tail - RW */
#define RCTL     0x00100  /* RX Control - RW */
#define ICR      0x000C0  /* Interrupt Cause Read - R/clr */
#define ICS      0x000C8  /* Interrupt Cause Set - WO */
#define IMS      0x000D0  /* Interrupt Mask Set - RW */
#define IMC      0x000D8  /* Interrupt Mask Clear - WO */
#define RAS_DEST       0x00000000
#define RAV	       0x80000000

/* Transmit Descriptor bit definitions */
#define TXD_CMD_EOP    0x01 /* End of Packet */
#define TXD_CMD_IFCS   0x02 /* Insert FCS (Ethernet CRC) */
#define TXD_CMD_IC     0x04 /* Insert Checksum */
#define TXD_CMD_RS     0x08 /* Report Status */
#define TXD_CMD_RPS    0x10 /* Report Packet Sent */
#define TXD_CMD_DEXT   0x20 /* Descriptor extension (0 = legacy) */
#define TXD_CMD_VLE    0x40 /* Add VLAN tag */
#define TXD_CMD_IDE    0x80 /* Enable Tidv register */
#define TXD_STAT_DD    0x01 /* Descriptor Done */
#define TXD_STAT_EC    0x02 /* Excess Collisions */
#define TXD_STAT_LC    0x04 /* Late Collisions */
#define TXD_STAT_TU    0x08 /* Transmit underrun */
#define TXD_CMD_TCP    0x01 /* TCP packet */
#define TXD_CMD_IP     0x02 /* IP packet */
#define TXD_CMD_TSE    0x04 /* TCP Seg enable */
#define TXD_STAT_TC    0x04 /* Tx Underrun */

/* Receive Descriptor bit definitions */
#define RXD_STAT_DD       0x01    /* Descriptor Done */
#define RXD_STAT_EOP      0x02    /* End of Packet */
#define RXD_STAT_IXSM     0x04    /* Ignore checksum */
#define RXD_STAT_VP       0x08    /* IEEE VLAN Packet */
#define RXD_STAT_UDPCS    0x10    /* UDP xsum caculated */
#define RXD_STAT_TCPCS    0x20    /* TCP xsum calculated */
#define RXD_STAT_IPCS     0x40    /* IP xsum calculated */
#define RXD_STAT_PIF      0x80    /* passed in-exact filter */
#define RXD_STAT_IPIDV    0x200   /* IP identification valid */
#define RXD_STAT_UDPV     0x400   /* Valid UDP checksum */
#define RXD_STAT_ACK      0x8000  /* ACK Packet indication */
#define RXD_ERR_CE        0x01    /* CRC Error */
#define RXD_ERR_SE        0x02    /* Symbol Error */
#define RXD_ERR_SEQ       0x04    /* Sequence Error */
#define RXD_ERR_CXE       0x10    /* Carrier Extension Error */
#define RXD_ERR_TCPE      0x20    /* TCP/UDP Checksum Error */
#define RXD_ERR_IPE       0x40    /* IP Checksum Error */
#define RXD_ERR_RXE       0x80    /* Rx Data Error */
#define RXD_SPC_VLAN_MASK 0x0FFF  /* VLAN ID is in lower 12 bits */
#define RXD_SPC_PRI_MASK  0xE000  /* Priority is in upper 3 bits */
#define RXD_SPC_PRI_SHIFT 13
#define RXD_SPC_CFI_MASK  0x1000  /* CFI is bit 12 */
#define RXD_SPC_CFI_SHIFT 12

/* Transmit Control */
#define TCTL_RST    0x00000001    /* software reset */
#define TCTL_EN     0x00000002    /* enable tx */
#define TCTL_BCE    0x00000004    /* busy check enable */
#define TCTL_PSP    0x00000008    /* pad short packets */
#define TCTL_CT     0x00000ff0    /* collision threshold */
#define TCTL_COLD   0x003ff000    /* collision distance */
#define TCTL_SWXOFF 0x00400000    /* SW Xoff transmission */
#define TCTL_PBE    0x00800000    /* Packet Burst Enable */
#define TCTL_RTLC   0x01000000    /* Re-transmit on late collision */
#define TCTL_NRTU   0x02000000    /* No Re-transmit on underrun */
#define TCTL_MULR   0x10000000    /* Multiple request support */


/* Receive Control */
#define RCTL_RST            0x00000001    /* Software reset */
#define RCTL_EN             0x00000002    /* enable */
#define RCTL_SBP            0x00000004    /* store bad packet */
#define RCTL_UPE            0x00000008    /* unicast promiscuous enable */
#define RCTL_MPE            0x00000010    /* multicast promiscuous enab */
#define RCTL_LPE            0x00000020    /* long packet enable */
#define RCTL_LBM_NO         0x00000000    /* no loopback mode */
#define RCTL_BAM            0x00008000    /* broadcast enable */
#define RCTL_SECRC          0x04000000    /* Strip Ethernet CRC */
#define RCTL_DTYP_MASK      0x00000C00    /* Descriptor type mask */
#define RCTL_BSIZE_2048     0x00000000
#define RCTL_BSIZE	    RCTL_BSIZE_2048

#define TXRING_LEN	64
#define RXRING_LEN	128
#define TBUFFSIZE	2048
#define RBUFFSIZE	2048

struct tx_desc
{
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
} __attribute__((packed));

struct rx_desc
{
	uint64_t addr;
	uint16_t length;
	uint16_t cs;
	uint8_t status;
	uint8_t errors;
	uint16_t special;
}  __attribute__((packed));	

struct packet
{
	char body[2048];
};

int pci_e1000_attach(struct pci_func *pcif);
int e1000_transmit(void *addr, size_t len);	
int e1000_receive(void *addr, size_t buflen);

#endif	// JOS_KERN_E1000_H
