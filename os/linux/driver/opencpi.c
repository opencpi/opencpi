/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * NOTE: This copyright and license does *not* cover user programs that use kernel
 * services in this driver via normal system calls - that is considered normal use
 * of the kernel, and does *not* fall under the heading of "derived work".
 * Other OpenCPI user code is covered by an LGPL license.
 */
#include <net/sock.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 33)
#include <linux/autoconf.h>
#else
#include <generated/autoconf.h>
#endif
// TODO: which version actually made this change?
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0)
#define ocpi_sk_for_each sk_for_each
#else
#define ocpi_sk_for_each(a,b,c) (void)b;sk_for_each(a,c)
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
#define GET_MINOR(file) iminor(file->f_inode)
#else
#define GET_MINOR(file) iminor(file->f_dentry->d_inode)
#endif
#include <linux/init.h>			// Kernel module initialization
#include <linux/kernel.h>		// Kernel common functions
#include <linux/module.h>		// Kernel module
#include <linux/moduleparam.h>		// Kernel module parameters
#include <linux/pci.h>
#include <linux/net.h>
#include <linux/netdevice.h>
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/errno.h>

#include <asm/current.h>		// Current process information
#include <asm/io.h>			// Virtual<->Physical address mapping
#include <asm/pgtable.h>		// Page file protections
#include <asm/uaccess.h>		// User space memory access functions
#include <asm/atomic.h>

#include <linux/fs.h>			// File systems (Load before cdev.h)
#include <linux/errno.h>		// Kernel error numbers
#include <linux/list.h>			// Kernel Linked Lists
#include <linux/pci.h>			// PCI API
#include <linux/slab.h>			// Kernel memory allocation
#include <linux/spinlock.h>		// Kernel locks
#include <linux/string.h>		// Kernel string library
#include <linux/sysfs.h>
#include <linux/pid.h>
#include <linux/cdev.h>			// Character devices (load this last due to bugs)
#include <linux/version.h>

// Removed in kernel 3.7
#ifndef VM_RESERVED
#define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

#include "HdlOCCP.h"
#include "HdlPciDriver.h"
#include "HdlNetDefs.h"
#include "KernelDriver.h"                    // includes shared with user mode

//#define _SCIF_ 1
#ifdef _SCIF_
#include "scif.h"
#include "mic/micscif.h"
#include "mic/micscif_rma.h"
#include "mic/micscif_map.h"
#include "mic_common.h"
#endif

#define DRIVER_NAME "opencpi"
#define DRIVER_ABBREV "ocpi"

#define log_emerg(args...) printk( KERN_EMERG   DRIVER_NAME ": " args)
#define log_alert(args...) printk( KERN_ALERT   DRIVER_NAME ": " args)
#define log_crit(args...)  printk( KERN_CRIT    DRIVER_NAME ": " args)
#define log_err(args...)   printk( KERN_ERR     DRIVER_NAME ": " args)
#define log_warn(args...)  printk( KERN_WARNING DRIVER_NAME ": " args)
#define log_info(args...)  printk( KERN_INFO    DRIVER_NAME ": " args)
#define log_note(args...)  printk( KERN_NOTICE  DRIVER_NAME ": " args)
#define log_debug(args...) printk( KERN_DEBUG   DRIVER_NAME ": " args)

#define OCPI_NET // include network driver code
// -----------------------------------------------------------------------------------------------
// Module constants
// -----------------------------------------------------------------------------------------------

#define OCPI_PCI_XHCI      0x00
#define OPENCPI_MINIMUM_MEMORY_ALLOCATION (16ul*1024)
#define OPENCPI_INITIAL_MEMORY_ALLOCATION (128ul*1024)
#define OPENCPI_MAXIMUM_MEMORY_ALLOCATION (128ul*1024)
#define ETH_P_OCPI_CP OCCP_ETHER_MTYPE
#define ETH_P_OCPI_DP OCDP_ETHER_TYPE
// -----------------------------------------------------------------------------------------------
// Module parameters - settable at load time
// -----------------------------------------------------------------------------------------------

static long	opencpi_size	    = -1;
static char *	opencpi_memmap	    = NULL;
static long     opencpi_max_devices = 4;

#ifdef _SCIF_
static scif_epd_t scif_fd=NULL; // handle to manage bus2memory access windows/apertures/resources
#define WIND_OFF 0x8000000
#endif

// -----------------------------------------------------------------------------------------------
// Data structures and definitions
// -----------------------------------------------------------------------------------------------

// Block type: what is behind the physical addresses
typedef enum {
  ocpi_mmio,        // the block is memory-mapped io space - to a device
  ocpi_reserved,    // the block is allocated from reserved (boot time) memory
  ocpi_dma,         // the block is allocated from the CMA region using the DMA API
  ocpi_kernel       // the block is allocated from the kernel memory at runtime
} ocpi_type_t;

// A block of address space, perhaps backed by allocated memory, initialized to zero
typedef struct {
  struct list_head	list;           // linked list for all blocks in driver
  ocpi_address_t	start_phys;	// physical address of start of block
  ocpi_address_t	end_phys;	// physical address of end of block
  ocpi_address_t	bus_addr;	// physical address as seen from the bus
  ocpi_size_t		size;		// block size
  void                 *virt_addr;      // for DMA API, the vaddr of the initial block
  ocpi_type_t           type;           // what does this block of addresses point to?
  atomic_t              refcnt;         // how many users of this block: mappings + minor devices
  bool                  isCached;       // should mappings be cached?
  bool                  available;      // is the block (of memory) available to allocate?
  struct file          *file;           // for allocations, which file allocated it
  pid_t                 pid;            // for debug: allocating pid
  u64                   kernel_alloc_id;// id of original kernel allocation (before any splits)
  u32			kernel_size;    // size of original size allocation (before any splits)
  unsigned              minor;          // minor of device of block, for dmam_free_coherent
} ocpi_block_t;

// Our per-device structure - initialized to zero
typedef struct {
  unsigned minor;            // set at creation
  struct cdev cdev;
  bool cdev_added;           // if non-zero cdev has been inited and added
  struct device *fsdev;      // if non-zero, fsdev has been created for core device (minor 0)
  struct pci_dev *pcidev;    // if non-zero it has been enabled
  u16 pci_command;           // value prior to enable
  bool got_pci_region;       // if non-zero pci region for bar0 has been reserved.
  OccpSpace *occp;           // if non-zero, bar0 has been mapped into kernel space
  ocpi_block_t *bar0, *bar1; // if non-zero, blocks created for pci regions
} ocpi_device_t;

/*
 *
 * getting rid of sockets for this:
 *  use an allocation, which could be non-contiguous
 *  basically a simple circular buffer for CP and real implementation for DP...
 */
typedef struct {
  struct sock        sk; // must be first
  struct net_device *netdev;
  bool any;
  ocpi_sockaddr_t    sockaddr;
} ocpi_sock_t;
static inline ocpi_sock_t *get_ocpi_sk(const struct sock *sk) { return (ocpi_sock_t *)sk;}
static inline struct sock *get_net_sk(const ocpi_sock_t *sk) { return (struct sock *)sk;}

// -----------------------------------------------------------------------------------------------
// Global variables for the driver
// -----------------------------------------------------------------------------------------------

static struct list_head block_list;             // list of blocks in the driver
static spinlock_t block_lock;                   // lock for block_list and contents
static bool block_init                 = false; // indication that the block list and lock was initialized

static u64 opencpi_kernel_alloc_id     = 0;     // running/counting id per kernel allocation
static ocpi_device_t **opencpi_devices = NULL;  // array of pointers to our (minor) devices
static dev_t opencpi_device_number     = 0;     // our allocated major num, with minor == 0
static struct class *opencpi_class     = NULL;  // our dynamically created class
static unsigned opencpi_ndevices       = 0;     // how many have we created (via probe)?
#ifdef CONFIG_PCI
static bool opencpi_pci_registered     = false; // bool to remember if we registered the driver
#endif
#ifdef OCPI_NET
static bool opencpi_proto_registered        = false; // we have registered our protocol
static bool opencpi_family_registered       = false; // we have registered our family
static bool opencpi_notifier_registered     = false; // we have registered our notifier
static bool opencpi_packet_type_registered  = false; // we have registered our notifier
static struct hlist_head opencpi_sklist[ocpi_role_limit];
static DEFINE_RWLOCK(opencpi_sklist_lock);
#endif

// -----------------------------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------------------------

static void
log_err_code(long err, const char *str) {
  log_err("Got code %ld when %s\n", err, str);
}

#if 0
unsigned char
ocpi_get_revision(struct pci_dev *dev) {
  u8 revision;

  pci_read_config_byte(dev, PCI_REVISION_ID, &revision);
  return revision;
}
#endif
// -----------------------------------------------------------------------------------------------
// Block management - blocks of physical addresses that we care about
// -----------------------------------------------------------------------------------------------

static void
log_debug_block(ocpi_block_t *block, char *msg) {
  log_debug("%s: block %p: %10lx @ %016llx type(%u) c(%u) pid(%d) prev(%p) next(%p) refcnt(%u)\n",
	    msg, block, (unsigned long)block->size, block->start_phys, block->type, block->isCached,
	    block->pid, block->list.prev, block->list.next, (unsigned)atomic_read(&block->refcnt));
}

#if 0
static void
dump_memory_map(char * label) {
  ocpi_block_t *block;

  log_debug( "dump memory map: %s (%p) prev(%p) next(%p)\n",
	     label, &block_list, block_list.prev, block_list.next);

  list_for_each_entry(block, &block_list, list)
    log_debug_block(block, "dump");
}
#endif

// Make a new block, inserting it into the list, returing NULL on failure
// The block is meta data about previously made allocations
static ocpi_block_t *
make_block(ocpi_address_t phys_addr, ocpi_address_t bus_addr, ocpi_size_t size, ocpi_type_t type,
	   bool available, u64 kernel_alloc_id, void * virt_addr, unsigned minor) {
  ocpi_block_t *block;
  if (!phys_addr || // Guard against allocating to 0x00
      !(block = kzalloc(sizeof(ocpi_block_t), GFP_KERNEL)) || IS_ERR(block))
    return NULL;
  block->start_phys = phys_addr;
  block->bus_addr = bus_addr;
  block->end_phys = phys_addr + size;
  block->virt_addr = virt_addr;
  block->minor = minor;
  block->size = size;
  block->type = type;
  block->isCached = false;
  atomic_set(&block->refcnt, 1);
  block->pid = current->pid;
  block->available = available;
  block->kernel_alloc_id = kernel_alloc_id;
  block->kernel_size = size;
  spin_lock(&block_lock);
  list_add(&block->list, &block_list);
  spin_unlock(&block_lock);
  log_debug_block(block, "make_block");
  return block;
}

// Free a kernel block
static inline void
free_kernel_block(ocpi_block_t *block) {
  if (block->size != block->kernel_size) {
    log_debug_block(block, "failed to free kernel block - not merged");
    log_err("failed to free unmerged kernel block %p - it is leaked\n", block);
  } else
    __free_pages(pfn_to_page(block->start_phys >> PAGE_SHIFT), get_order(block->size));
}

// Free a DMA block
static inline void
free_dma_block(ocpi_block_t *block) {
  if (block->size != block->kernel_size) {
    log_debug_block(block, "failed to free dma block - not merged");
    log_err("failed to free unmerged dma block %p - it is leaked\n", block);
  } else
    dmam_free_coherent(opencpi_devices[block->minor]->fsdev, block->size, block->virt_addr,
		       block->bus_addr);
}
// This method will scan the list of memory blocks and merge any adjacent free blocks into a
// single free block. And if a kernel block is completely coalesced, it will be freed
static void
merge_free_memory(void) {
  ocpi_block_t *block, *next;

  spin_lock(&block_lock);
  list_for_each_entry_safe(block, next, &block_list, list)
    if (block->available) {
      // Merge all the next contiguous blocks into me
      log_debug("Merge: considering %p/%p/%p a %u %llx=%llx %llx=%llx\n", block, next,
		&block_list, next->available,  block->start_phys + block->size, next->start_phys,
		block->kernel_alloc_id, next->kernel_alloc_id);
      while (&next->list != &block_list && next->available &&
	     block->start_phys + block->size == next->start_phys &&
	     block->kernel_alloc_id == next->kernel_alloc_id) {
	block->size += next->size;
	block->end_phys += next->size;
	list_del(&next->list);
	kfree(next);
	next = list_entry(block->list.next, ocpi_block_t, list);
      }
      // If I now have a fully merged kernel allocation, I can free it
      if (block->type == ocpi_kernel && block->size == block->kernel_size) {
	free_kernel_block(block);
	list_del(&block->list);
	kfree(block);
      } else if (block->type == ocpi_dma && block->size == block->kernel_size &&
		 block->virt_addr != NULL) {
	// If I now have a fully merged dma allocation, I can free it
        free_dma_block(block);
        list_del(&block->list);
        kfree(block);
      }
    }
  spin_unlock(&block_lock);
}

// Free a block at runtime and return whether merging is required
static inline bool
free_block(ocpi_block_t *block) {
  if (block->type == ocpi_mmio) {
    list_del(&block->list);
    kfree(block);
    return false;
  }
  block->available = true;
  return true;
}

// Release (dereference) a block in a locked context
static inline bool
release_block_locked(ocpi_block_t *block) {
  return atomic_dec_and_test(&block->refcnt) ? free_block(block) : false;
}

// Release (dererence) this specific block, not in a context of processing the block list
static void
release_block(ocpi_block_t *block)
{
  bool do_merge = false;
  log_debug_block(block, "release_block");
  spin_lock(&block_lock);
  do_merge = release_block_locked(block);
  spin_unlock(&block_lock);
  if (do_merge)
    merge_free_memory();
}

// ----------------------------------------------------------------------------------------------
// Memory allocation management: blocks that are boot-time "reserved" or runtime "kernel" alloc'd
// ----------------------------------------------------------------------------------------------

// Retrieve status of available memory, return zero or negative error code
static int
get_status(ocpi_status_t *status) {
  ocpi_block_t *block;

  memset(status, 0, sizeof(*status));
  spin_lock(&block_lock);
  list_for_each_entry(block, &block_list, list)
    if (block->available) {
      status->available += block->size;
      if (block->size > status->largest) {
	status->largest = block->size;
      }
    }
  status->allocated = opencpi_size;
  spin_unlock(&block_lock);
  log_debug("ioctl status: allocated (%lx), available (%lx), largest (%lx)\n",
	    (unsigned long)status->allocated, (unsigned long)status->available,
	    (unsigned long)status->largest);
  return 0;
}

// Try to get/allocate a memory block.  Return it on success, or NULL on failure
// - sparep points to a spare block to use for splitting
// - *sparep should be set to zero if the spare is used
// if "file" is NULL it is an initial driver request, NOT a process request
// 'splits' a previously allocated block of memory per information in request
// adds the metadata to the list of metadata about blocks and returns
static long
get_memory(ocpi_request_t *request, struct file *file, ocpi_block_t **sparep) {
  long err = -ENOMEM;
  ocpi_block_t *block;
  spin_lock(&block_lock);
  list_for_each_entry(block, &block_list, list)
    if (block->available && block->size >= request->actual) {
      request->address = block->start_phys;
      request->bus_addr = block->bus_addr;
      if (block->size > request->actual) {
	ocpi_block_t *split = *sparep;
	*sparep = 0;
	*split = *block;
	split->start_phys += request->actual;
	split->size -= request->actual;
	block->size = request->actual;
	list_add(&split->list, &block->list);
      }
      atomic_set(&block->refcnt, 1);
      block->file = file;
      block->isCached = request->how_cached == ocpi_cached;
      if (file)
	block->available = false;
      err = 0;
      break;
    }
  spin_unlock(&block_lock);
  return err;
}

// Convert a CPU physical address to the address used by other bus masters to access the
// local memory at this CPU physical address
static uint64_t
phys2bus(uint64_t phys) {
  // For this processor physical memory is accessible from the bus at the same addr
  // This is the case on nearly all x64 PCI root complex processors.
  // It is also the case on the zynq.
#ifdef _SCIF_
  // On the MIC, the bus address is based a resource of bus-accessible apertures/windows
  // that we must allocate.
  scif_pinned_pages_t    spages;
  off_t nf;
  if (scif_fd == NULL)
    scif_fd =  scif_open();
  int ret = scif_pin_pages(pfn_to_kaddr(page_to_pfn(kpages)), PAGE_SIZE*4,
			   SCIF_PROT_WRITE | SCIF_PROT_READ, SCIF_MAP_KERNEL, &spages);
  printk("Return from scif_pin_pages = %d\n", ret);
  if (ret == 0)
    nf = scif_register_pinned_pages (scif_fd, spages, WIND_OFF, SCIF_MAP_FIXED);
  printk("*** (JM) -> Window addr = 0x%llx\n", nf);
  return 0x387c00000000ull + nf;
#else
  return phys;
#endif
}
// Convert a bus address to the physical address used by the local processor to access the
// memory at that bus address
static uint64_t
bus2phys(uint64_t bus) {
#ifdef _SCIF_
????
#else
  return bus;
#endif
}

// Establish a remote bus address block so we can mmap to it
static long
establish_remote(struct file *file, ocpi_request_t *request) {
  ocpi_address_t phys = bus2phys(request->bus_addr);
  bool found = false, bad = false;
  ocpi_address_t end_address = phys + request->actual;
  ocpi_block_t *block = NULL;
  unsigned minor = GET_MINOR(file);

  log_debug("Establishing remote from bus 0x%llx to phys 0x%llx size %x\n",
	    request->bus_addr, phys, request->actual);
  if (!phys)
    return -EIO;
  // Since this is outer loop, a linear search is ok.  Maybe a hash table someday.
  spin_lock(&block_lock);
  list_for_each_entry(block, &block_list, list) {
    if (phys >= block->start_phys && end_address <= block->end_phys) {
      found = true;
      request->address = block->start_phys + (phys - block->start_phys);
    } else if ((phys >= block->start_phys && phys < block->end_phys) ||
	       (end_address > block->start_phys && end_address <= block->end_phys))
      bad = true;
  }
  spin_unlock(&block_lock);
  if (found) {
    log_debug("Establishing remote: bus address 0x%llx already registered. Phys is 0x%llx\n",
	      request->bus_addr, phys);
    return 0;
  } else if (bad) {
    log_err("Establishing remote: bus region 0x%llx collides with existing. Phys is 0x%llx\n",
	    request->bus_addr, phys);
    return -EEXIST;
  }
  request->address = phys;
  return make_block(phys, request->bus_addr, request->actual, ocpi_mmio, false, 0, NULL, minor) ?
    0 : -EINVAL;
}

// makes allocation using DMA API, makes block, and adds block to list
// returns 0 if successful
// this actually makes an allocation, not just massaging metadata
static long
get_dma_memory(ocpi_request_t *request, unsigned minor) {
  void *virtual_addr;
  dma_addr_t dma_handle;
  long err = -ENOMEM;

  // don't need to do a page alignment, so actual can be the same as needed
  // either it PAGE_ALIGN was already called to set request->actual or this is
  // the initial memory grab and it's not important
  // Continuing to use request->actual to mantain consistency with other similiar calls
  request->actual = request->needed;
  log_debug("memory request %lx -> %lx\n", (unsigned long)request->needed,
	    (unsigned long)request->actual);
  // dma_set_coherent_mask sets a bit mask describing which bits of an address the device
  // supports, default is 32
  if (dma_set_coherent_mask(opencpi_devices[minor]->fsdev, DMA_BIT_MASK(32)))
    log_debug("dma_set_coherent_mask failed for device %p\n", opencpi_devices[minor]->fsdev);
  if ((virtual_addr = dmam_alloc_coherent(opencpi_devices[minor]->fsdev, request->actual,
					  &dma_handle, GFP_KERNEL)) == NULL) {
    log_err("dmam_alloc_coherent failed\n");
    return err;
  }
  request->bus_addr = (ocpi_address_t) dma_handle;
  request->address = bus2phys(request->bus_addr);
  if (make_block(request->address, request->bus_addr, request->actual,  ocpi_dma,
		 true, ++opencpi_kernel_alloc_id, virtual_addr, minor) == NULL) {
    dmam_free_coherent(opencpi_devices[minor]->fsdev, request->actual, virtual_addr,
		       request->bus_addr);
    return err;
  }
  log_debug("requested (%lx) reserved %lx @ %016llx bus %016llx\n", (unsigned long)request->needed,
	    (unsigned long)request->actual, (unsigned long long)request->address,
	    (unsigned long long)request->bus_addr);
  err = 0;
  return err;
}
// If file == NULL, this is a request for the initial driver memory, not a minor 0 ioctl request
// this tries to split an existing block, if that fails it tries to allocate more memory
// and use the memory in the new block
static long
request_memory(struct file *file, ocpi_request_t *request) {
  ocpi_block_t *spare;
  long err = -ENOMEM;
  unsigned minor = 0;

  if (file != NULL)
    minor = GET_MINOR(file);
  request->actual = PAGE_ALIGN(request->needed);
  log_debug("memory request file %p %lx -> %lx\n",
	    file, (unsigned long)request->needed, (unsigned long)request->actual);
  // This is most likely to be used, but possibly will be given back
  // We allocate it outside the spin lock
  spare = kzalloc(sizeof(ocpi_block_t), GFP_KERNEL);
  if (IS_ERR(spare) || spare == NULL)
    return -ENOMEM;
  do {
    if ((err = get_memory(request, file, &spare)) == 0)
      break;
    log_debug("couldn't get allocation in first pass\n");
    // No memory in the list, try an allocation
    if (request->actual > OPENCPI_MAXIMUM_MEMORY_ALLOCATION) {
      log_err("memory request exceeded driver's specified limit of %ld\n",
	      OPENCPI_MAXIMUM_MEMORY_ALLOCATION);
      break;
    }
    // trying allocation from DMA API first
    // Centos doesn't support contiguous memory allocation as of Centos 7
    // AV-1645 - in centos 8 revist using the DMA API instead of memmap
    if ((err = get_dma_memory(request, minor)) != 0) {
      log_err("get_dma_memory in request_memory failed, trying fallback\n");
      log_err("if allocation failure occurs, see README for memmap configuration\n");
      // Try to make a kernel allocation and stick it in the list
      // can't use alloc_pages_exact for high mem in a 32 bit world
      {
	unsigned order = get_order(request->actual);
	struct page *kpages = alloc_pages(GFP_KERNEL | __GFP_HIGHMEM, order);
	ocpi_address_t phys_addr = (ocpi_address_t)page_to_pfn(kpages) << PAGE_SHIFT;
	if (kpages == NULL) {
	  log_err("memory request of %ld could not be satified by the kernel\n",
		  (unsigned long)request->actual);
	  break;
	}
	if (make_block(phys_addr, phys2bus(phys_addr), PAGE_SIZE << order, ocpi_kernel, true,
		       ++opencpi_kernel_alloc_id, NULL, minor) == NULL) {
	  __free_pages(kpages, order);
	  break;
	}
	log_debug("allocated kernel pages: %lx @ %016llx\n", PAGE_SIZE << order, phys_addr);
      }
    }
    // Now we try one more time after having added kernel pages or made a DMA allocation
    // The only thing that can go wrong is someone else slipping in
    // FIXME: should we iterate/retry in this case?
    if ((err = get_memory(request, file, &spare)))
      log_err("couldn't get to the allocation that was just made: unexpected\n");
  } while (0);
  if (spare)
    kfree(spare);
  if (err == 0)
    log_debug("requested (%lx) reserved %lx @ %016llx bus %016llx\n",
	      (unsigned long)request->needed, (unsigned long)request->actual,
	      (unsigned long long)request->address, (unsigned long long)request->bus_addr);
  return err;
}
// ----------------------------------------------------------------------------------------------
// Our virtual memory operations on vma areas created initially in our mmap
// ----------------------------------------------------------------------------------------------

// open is called either from our mmap or when the kernel does fork/munmap/split etc.
//   We just keep track of the refcount since multiple processes may map to the block,
//   and we need to know who is mapped to it across all processes etc.
static void
opencpi_vma_open(struct vm_area_struct *vma) {
  ocpi_block_t *block = vma->vm_private_data;

  log_debug("vma_open: vma %p size %lu @ offset %016llx\n", vma,
	    (unsigned long)(vma->vm_end - vma->vm_start),
	    (ocpi_address_t)vma->vm_pgoff << PAGE_SHIFT);
  atomic_inc(&block->refcnt);
  log_debug_block(block, "vma_open:");
}

// close: a process mapping is going away.
static void
opencpi_vma_close(struct vm_area_struct *vma) {
  size_t size = vma->vm_end - vma->vm_start;
  ocpi_address_t address = vma->vm_pgoff << PAGE_SHIFT;
  ocpi_block_t *block = vma->vm_private_data;

  log_debug("vma close %p block %p count %d: %zx @ %016llx\n",
	    vma, block, atomic_read(&block->refcnt), size, address);

  release_block(block);
}

// Map an individual page - in our case only for kernel allocation (not mmio, not reserved)
#if defined(RHEL_MAJOR)
#if RHEL_MAJOR==6 || RHEL_MAJOR==7
#define OCPI_RH6
#else
#define OCPI_RH5
#endif
#else
#define OCPI_RH6
#endif

#ifdef OCPI_RH5
static struct page *
opencpi_vma_nopage(struct vm_area_struct *vma, unsigned long virt_addr, int *type) {
  ocpi_block_t *block = vma->vm_private_data;

  if (block && block->type == ocpi_kernel) {
    unsigned long pfn = vma->vm_pgoff + ((virt_addr - vma->vm_start) >> PAGE_SHIFT);
    struct page *pageptr = pfn_to_page(pfn);

    log_debug("vma_nopage vma %p addr %lx pfn %lx\n", vma, virt_addr, pfn);
    log_debug_block(block, "vma_nopage:");

    get_page(pageptr);
    if (type)
      *type = VM_FAULT_MINOR;
    return pageptr;
  }
  return NOPAGE_SIGBUS; /* send a SIGBUS */
}
#else
static int opencpi_vma_fault
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
  (struct vm_fault *vmf) {
  ocpi_block_t *block = vmf->vma->vm_private_data;
#else
  (struct vm_area_struct *vma, struct vm_fault *vmf) {
  ocpi_block_t *block = vma->vm_private_data;
#endif
  if (block && block->type == ocpi_kernel) {
    unsigned long offset = vmf->pgoff << PAGE_SHIFT;
    struct page *pageptr = virt_to_page(offset);
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
    log_debug("vma_fault vma %p addr %lx pfn %lx\n",
	      vmf->vma, vmf->address,
#elif LINUX_VERSION_CODE == KERNEL_VERSION(4, 10, 0)
    log_debug("vma_fault vma %p addr %lx pfn %lx\n",
	      vma, vmf->address,
#else
    log_debug("vma_fault vma %p addr %p pfn %lx\n",
	      vma, vmf->virtual_address,
#endif
	      vmf->pgoff);
    log_debug_block(block, "vma_fault:");
    get_page(pageptr);
    vmf->page = pageptr;
    return 0;
  }
  return VM_FAULT_SIGBUS; /* send a SIGBUS */
}
#endif // end of not RHEL5 (early kernel)
// vma operations on our mappings
static struct vm_operations_struct opencpi_vm_ops = {
  .open = opencpi_vma_open,
  .close = opencpi_vma_close,
#ifdef OCPI_RH5
  .nopage = opencpi_vma_nopage,
#else
  .fault = opencpi_vma_fault,
#endif
};

// -----------------------------------------------------------------------------------------------
// Character device functions: only "release", "ioctl" and "mmap" for us
// -----------------------------------------------------------------------------------------------

static int
opencpi_io_open(struct inode *inode, struct file *file) {
  unsigned minor = iminor(inode);
  log_debug("open file(%p) minor(%u) initial size(%lld)\n",
	    file, minor, inode->i_size);
  if (minor != 0) {
    ocpi_device_t *mydev = opencpi_devices[minor];
    if (mydev->bar0 && mydev->bar1)
      inode->i_size = mydev->bar0->size + mydev->bar1->size;
  }
  return 0;
}
//static ssize_t opencpi_io_read(struct file *file, char *buf, size_t count, loff_t *ppos) {
//	log_debug("file read\n");
//	return 0;
//}
//
//static ssize_t opencpi_io_write(struct file *file, const char *buf, size_t count, loff_t *ppos) {
//	log_debug("file write\n");
//	return 0;
//}

// FIXME:  we could stash a list head in the file structure
// The only resource we deal with on close is allocations in minor 0
// For IO devices, the mapping system and the pci system take care of resources, and thus
// while we might have mapped to them under minor zero, we don't own them and didn't
// create them.
static int
opencpi_io_release(struct inode *inode, struct file *file) {
  unsigned minor = iminor(inode);

  log_debug("release file(%p) inode(%p) minor(%d)\n", file, inode, minor);

  if (minor == 0) {
    ocpi_block_t *block, *temp;
    bool do_merge = false;
    spin_lock(&block_lock);
    list_for_each_entry_safe(block, temp, &block_list, list)
      if (block->file == file) {
	block->file = NULL; // Even if we are not the last one using it, its not ours anymore
	do_merge = release_block_locked(block);
      }
    spin_unlock(&block_lock);
    if (do_merge)
      merge_free_memory();
  }
  return 0;
}

#ifdef CONFIG_PCI
static int get_pci(unsigned minor, ocpi_pci_t *pci);
#endif
// ioctl for getting memory status and requesting memory allocations
// FIXME: do copy_to/from_user return the right error codes anyway?
static
#ifdef HAVE_UNLOCKED_IOCTL
long
opencpi_io_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
  unsigned minor = GET_MINOR(file);
#else
int
opencpi_io_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg) {
  unsigned minor = iminor(inode);
#endif
  int err;
  switch (cmd) {
  case OCPI_CMD_PCI:
      if (minor == 0 || minor > opencpi_ndevices)
	return -EINVAL;
#ifdef CONFIG_PCI
      {
	ocpi_pci_t pci;
	if ((err = get_pci(minor, &pci)))
	  return err;
	if (copy_to_user((void __user *)arg, &pci, sizeof(pci)))
	  return -EFAULT;
	return 0;
      }
#else
      return -ENODEV;
#endif
  case OCPI_CMD_STATUS:
    {
      ocpi_status_t status;
      if ((err = get_status(&status)))
	return err;
      if (copy_to_user((void __user *)arg, &status, sizeof(status)))
	return -EFAULT;
      return 0;
    }
  case OCPI_CMD_REQUEST:
    {
      ocpi_request_t request;
      int err;
      if (copy_from_user(&request, (void __user *)arg, sizeof(request))) {
	log_err("unable to retrieve memory request\n");
	return -EFAULT;
      }
      err = (request.needed ? request_memory : establish_remote)(file, &request);
      if (copy_to_user((void __user *)arg, &request, sizeof(request))) {
	log_err("unable to return memory request - we will leak until exit\n");
	err = -EFAULT;
	// FIXME: release the memory here
      }
      return err;
    }
  case OCPI_CMD_DISCOVER:
    {
      // do ethernet discovery.
      // allow an interface name to limit the search.
      // return how many were discovered.
      // minor devices get added, remembering the mac addr.
      // then we can still look at /dev
      // q: should any of this happen automatically?
      // a: probably not. ocpihdl search probably just poke this.
      // how to exclude the search? perhaps a module string param?
      return -EINVAL;
    }
  default:
    log_err("ioctl invalid command (%08x)\n", cmd);
    return -EINVAL;
  }
}

// This mmap is used differently for the mem minor device vs. the pci minor devices.
// Minor zero, ocpi/mem, is used for two purposes:
// 1. To allocate DMA memory for the calling process
// 1a.  This may come from reserved memory (memmap command line)
// 1b.  This may also come from kernel allocated memory (get_free_pages)
// 2. To map to driver-known memory anywhere in the /dev/mem address space
// 2a.  This may be to allocations in reserved memory
// 2b.  This may be to allocations in kernel memory
// 2c.  This may be to I/O memory for discovered PCI devices/BARs.

static int
opencpi_io_mmap(struct file * file, struct vm_area_struct * vma) {
#if 0
  unsigned minor;
  log_debug("mmap: file %p, f_dentry %p inode %p vma %p\n",
	    file, file->f_dentry, file->f_dentry->d_inode, vma);
  minor = iminor(file->f_dentry->d_inode);
  log_debug("mmap: minor: %u\n", minor);
#else
  unsigned minor = GET_MINOR(file);
  ocpi_device_t *mydev = opencpi_devices[minor];
  ocpi_size_t size = vma->vm_end - vma->vm_start;
  ocpi_address_t
    start_address = (ocpi_address_t)vma->vm_pgoff << PAGE_SHIFT,
    end_address = start_address + size;
  int err = -EINVAL;
  ocpi_block_t *block = NULL;
  unsigned long pfn = vma->vm_pgoff;

  log_debug("mmap minor %d file %p dev %p, %lx @ %016lx (%016llx to %016llx)\n",
	    minor, file, mydev, (unsigned long)size, vma->vm_start,
	    start_address, end_address);
  if (minor == 0) {
    bool found = false;
    // Since this is outer loop, a linear search is ok.  Maybe a hash table someday.
    spin_lock(&block_lock);
    list_for_each_entry(block, &block_list, list)
      if (start_address >= block->start_phys && end_address <= block->end_phys) {
	found = true;
	break;
      }
    spin_unlock(&block_lock);
    if (found && block->available) {
      log_err("mmap to unallocated reserved memory %lx @ %016llx\n", (unsigned long)size, start_address);
      found = false;
    }
    if (!found) {
      log_err("mmap to bad addresses %lx @ %016llx\n", (unsigned long)size, start_address);
      block = NULL;
    }
  } else if (start_address == 0 && end_address == mydev->bar0->size) {
    log_debug("bar0 mapping\n");
    block = mydev->bar0;
    pfn = block->start_phys >> PAGE_SHIFT;
  } else if (start_address == mydev->bar0->size && size == mydev->bar1->size) {
    log_debug("bar1 mapping\n");
    block = mydev->bar1;
    pfn = block->start_phys >> PAGE_SHIFT;
  } else
    log_err("invalid device mapping\n");
  if (block) {
    log_debug_block(block, "mmap");

    // Set the VM operations for this map
    vma->vm_ops = &opencpi_vm_ops;
    vma->vm_private_data = block;
    // Check for uncached blocks
    if (!block->isCached)
      vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    vma->vm_flags |= VM_RESERVED;
    if (block->type == ocpi_mmio) {
      vma->vm_flags |= VM_IO;
      err = io_remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot);
    } else
      err = remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot);
  }
  if (err)
    log_err("mmap failed: minor %d file %p dev %p, %lx @ %016lx (%016llx to %016llx)\n",
	    minor, file, mydev, (unsigned long)size, vma->vm_start,
	    start_address, end_address);
  else {
    opencpi_vma_open(vma);
    log_debug("file mmap using %lx @ %016llx\n", (unsigned long)size, start_address);
  }
  return err;
#endif
}

/** define which file operations are implemented by this driver */
static struct file_operations opencpi_file_operations = {
	.owner = THIS_MODULE,
	.open = opencpi_io_open,
	.release = opencpi_io_release,
//	.read = opencpi_io_read,
//	.write = opencpi_io_write,
#ifdef HAVE_UNLOCKED_IOCTL
        .unlocked_ioctl
#else
	.ioctl
#endif
	 = opencpi_io_ioctl,
	.mmap = opencpi_io_mmap,
};
// -----------------------------------------------------------------------------------------------
// Device management functions for our "ocpi_device_t" structures
// -----------------------------------------------------------------------------------------------

// Make a device and put it into the global pointer array
static ocpi_device_t *
make_device(unsigned minor) {
  ocpi_device_t *mydev = NULL;
  if (minor >= opencpi_max_devices)
    log_err("Illegal minor %u vs max of %ld\n", minor, opencpi_max_devices);
  else if ((mydev = kzalloc(sizeof(ocpi_device_t), GFP_KERNEL)) == NULL)
    log_err("Can't allocate opencpi_device\n");
  else {
    mydev->minor = minor;
    opencpi_devices[minor] = mydev;
  }
  return mydev;
}

// Initialize and register this device as a character device in the system
static int
add_cdev(ocpi_device_t *mydev) {
  long err;
  dev_t dev = MKDEV(MAJOR(opencpi_device_number), mydev->minor);
  log_debug("adding char dev %d/%d\n", MAJOR(dev), MINOR(dev));
  cdev_init(&mydev->cdev, &opencpi_file_operations);
  mydev->cdev.owner = THIS_MODULE; // why is this not in cdev_init somehow?
  if ((err = cdev_add(&mydev->cdev, dev, 1))) {
    log_err_code(err, "Can't add cdev");
    return err;
  }
  log_debug("added char dev %d/%d\n", MAJOR(mydev->cdev.dev), MINOR(mydev->cdev.dev));
  mydev->cdev_added = 1;    // record that as added the cdev
  return 0;
}

#ifdef CONFIG_PCI
static void check(struct pci_dev *d, const char *msg) {
  u16 cmd;
  pci_read_config_word(d, PCI_COMMAND, &cmd);
  log_debug("at %s cmd is %x\n", msg, cmd);
}
#endif
// Free the device and associated resources
static void
free_device(ocpi_device_t *mydev) {
  log_debug("removing minor device %d\n", mydev->minor);
  if (mydev->bar0)
    release_block(mydev->bar0);
  if (mydev->bar1)
    release_block(mydev->bar1);
  if (mydev->fsdev) {
    device_destroy(opencpi_class, mydev->cdev.dev);
    mydev->fsdev = NULL;
  }
  if (mydev->cdev_added) {
    cdev_del(&mydev->cdev);
    mydev->cdev_added = 0;
  }
#ifdef CONFIG_PCI
  if (mydev->occp != NULL) {
    check(mydev->pcidev, "before iounmap");
    pci_iounmap(mydev->pcidev, mydev->occp);
    mydev->occp = NULL;
  }
  if (mydev->got_pci_region) {
    check(mydev->pcidev, "before release region");
    pci_release_region(mydev->pcidev, 0);
    mydev->got_pci_region = 0;
  }
  if (mydev->pcidev) {
    u16 cmd;
    check(mydev->pcidev, "before disable");
    pci_disable_device(mydev->pcidev);
    check(mydev->pcidev, "after disable");
    pci_read_config_word(mydev->pcidev, PCI_COMMAND, &cmd);
    if (cmd != mydev->pci_command) {
      log_err("restoring pci status to what it was before we were probed: %x (after disable it was %x)\n",
	      mydev->pci_command, cmd);
      pci_write_config_word(mydev->pcidev, PCI_COMMAND, mydev->pci_command);
      check(mydev->pcidev, "after restore");
    }
    mydev->pcidev = NULL;
  }
#endif
  if (opencpi_devices)
    opencpi_devices[mydev->minor] = 0;
  kfree(mydev);
}
#ifdef CONFIG_PCI
static void log_pci_err(struct pci_dev *pcidev, long err, const char *str) {
  log_err("PCI-related error on %04x:%02x:%02x.%x:\n",
	  pci_domain_nr(pcidev->bus), pcidev->bus->number,
	  PCI_SLOT(pcidev->devfn), PCI_FUNC(pcidev->devfn));
  log_err_code(err, str);
}

// -----------------------------------------------------------------------------------------------
// PCI management functions for us as a PCI driver
// -----------------------------------------------------------------------------------------------

static int
get_pci(unsigned minor, ocpi_pci_t *pci) {
  ocpi_device_t *mydev = opencpi_devices[minor];
  if (!mydev || !mydev->bar0 || !mydev->bar1)
    return -EINVAL;
  pci->bar0 = mydev->bar0->start_phys;
  pci->bar1 = mydev->bar1->start_phys;
  pci->size0 = mydev->bar0->size;
  pci->size1 = mydev->bar1->size;
  return 0;
}

// This pci probe has the side-effect that if everything is successful,
// a new minor device (non-zero), is added to the system
static int
probe_pci(struct pci_dev *pcidev, const struct pci_device_id *id) {
#define NBARS PCI_ROM_RESOURCE // how many bars that might be RAM
  u64 sizes[NBARS];
  unsigned nbars = 0;
  unsigned new_device = opencpi_ndevices + 1;
  ocpi_device_t *mydev = NULL;
  unsigned i;
  long err;

  log_debug("pci probe\n");
  for (i = 0; i < NBARS; i++) {
    sizes[i] = pci_resource_len(pcidev, i);
    if (sizes[i])
      nbars++;
  }
  if (!opencpi_devices || opencpi_devices[new_device]) {
    log_pci_err(pcidev, 0, "unexpected internal driver error sequence");
    return -ENODEV;
  }
  if (nbars != 2 ||
      (pci_resource_flags(pcidev, 0) & PCI_BASE_ADDRESS_SPACE) ==
      PCI_BASE_ADDRESS_SPACE_IO ||
      (pci_resource_flags(pcidev, 1) & PCI_BASE_ADDRESS_SPACE) ==
      PCI_BASE_ADDRESS_SPACE_IO ||
      pci_resource_flags(pcidev, 0) & PCI_BASE_ADDRESS_MEM_PREFETCH ||
      pci_resource_flags(pcidev, 0) & IORESOURCE_PREFETCH ||
      pci_resource_flags(pcidev, 0) & IORESOURCE_IO ||
      pci_resource_flags(pcidev, 1) & PCI_BASE_ADDRESS_MEM_PREFETCH ||
      pci_resource_flags(pcidev, 1) & IORESOURCE_PREFETCH ||
      pci_resource_flags(pcidev, 1) & IORESOURCE_IO ||
      (pci_resource_flags(pcidev, 0) & PCI_BASE_ADDRESS_MEM_TYPE_MASK) !=
      PCI_BASE_ADDRESS_MEM_TYPE_32 ||
      // FIXME: make this based on the HdlOCCP stuff?
      (sizes[0] != 64*1024*1024 && sizes[0] != 16*1024*1024)) {
    log_pci_err(pcidev, 0, "opencpi PCI device misconfigured, rejected");
    return -ENODEV;
  }
  // now we start to obtain resources
  if ((mydev = make_device(new_device)) == NULL)
    return -ENODEV;

  do { // break to cleanup to undo resources
    check(pcidev, "before enable");
    pci_read_config_word(pcidev, PCI_COMMAND, &mydev->pci_command);
    if ((err = pci_enable_device(pcidev))) {
      log_pci_err(pcidev, err, "pci_enable_device failed");
      break;
    }
    mydev->pcidev = pcidev;   // record that we have enabled
    check(pcidev, "before request");
    if ((err = pci_request_region(pcidev, 0, DRIVER_NAME))) {
      log_pci_err(pcidev, err, "pci_request_region failed");
      break;
    }
    mydev->got_pci_region = 1; // record that we got the region
    check(pcidev, "before iomap");
    {
      void *bar0 = pci_iomap(pcidev, 0, 0);
      if (bar0 == NULL || IS_ERR(bar0)) {
	log_pci_err(pcidev, PTR_ERR(bar0), "pci_iomap failed");
	break;
      }
      mydev->occp = bar0;     // record that we mapped bar0
    }
    {
      u64 magic = mydev->occp->admin.magic;
      if (magic != OCCP_MAGIC) {
	log_err("pci device has bad magic: 0x%llx, should be 0x%llx\n",
		magic, OCCP_MAGIC);
	log_pci_err(pcidev, 0, "failed magic number test");
	break;
      }
    }
    log_debug("Discovered opencpi PCI device %d of %ld, with good magic,"
	      " at %04x:%02x:%02x.%x vendor %x device %x class %x subvendor %x subdevice %x\n",
	      new_device, opencpi_max_devices, pci_domain_nr(pcidev->bus), pcidev->bus->number,
	      PCI_SLOT(pcidev->devfn), PCI_FUNC(pcidev->devfn), pcidev->vendor, pcidev->device,
	      pcidev->class, pcidev->subsystem_vendor, pcidev->subsystem_device);
    // We assume no real side effects on this initialization (nothing to undo)
    if ((err = add_cdev(mydev)))
      break;
#ifdef OCPI_RH6
    log_debug("pci device name set to: %s\n",
	      mydev->pcidev->dev.init_name ? mydev->pcidev->dev.init_name : "nothing");
#else
    log_debug("pci device name set to: %s\n",
	      mydev->pcidev->dev.bus_id[0] ? mydev->pcidev->dev.bus_id : "nothing");
#endif
    {
      struct device *fsdev =
#ifdef OCPI_RH6
	device_create(opencpi_class, NULL, mydev->cdev.dev, NULL,
		      "%s=p=%04x:%02x:%02x.%d", DRIVER_ABBREV, pci_domain_nr(pcidev->bus),
		      pcidev->bus->number, PCI_SLOT(pcidev->devfn), PCI_FUNC(pcidev->devfn));
#else
        device_create(opencpi_class, NULL, mydev->cdev.dev,
		      "%s=p=%04x:%02x:%02x.%d", DRIVER_ABBREV, pci_domain_nr(pcidev->bus),
		      pcidev->bus->number, PCI_SLOT(pcidev->devfn), PCI_FUNC(pcidev->devfn));
#endif
      if (fsdev == NULL || IS_ERR(fsdev)) {
	log_pci_err(pcidev, PTR_ERR(fsdev), "device_create failed");
	break;
      }
      mydev->fsdev = fsdev;
    }
    {
      ocpi_address_t
	phys_b0 = pci_resource_start(pcidev, 0),
	phys_b1 = pci_resource_start(pcidev, 1);
      if ((mydev->bar0 = make_block(phys_b0, phys_b0, sizes[0], ocpi_mmio, false, 0, NULL,
				    new_device)) == NULL ||
	  (mydev->bar1 = make_block(phys_b1, phys_b1, sizes[1], ocpi_mmio, false, 0, NULL,
				    new_device)) == NULL)
	break;
    }
    check(pcidev, "before drvdata");
    pci_set_drvdata(pcidev, mydev);
    opencpi_devices[new_device] = mydev;
    opencpi_ndevices = new_device;
    return 0;
  } while(0);
  // clean up whatever was accomplished since we are failing
  free_device(mydev);
  return -ENODEV;
}

// Cleanup things associated with the pci device that are not in use by others.
// The challenge: if some other process has mapped to the PCI.
static void
remove_pci(struct pci_dev *dev) {
  /* clean up any allocated resources and stuff here.
   * like call release_region();
   */
  ocpi_device_t *mydev = pci_get_drvdata(dev);
  if (mydev) {
    log_debug("pci removing minor device %d at %p\n", mydev->minor, mydev);
    free_device(mydev);
  } else
    log_err("removing pci with no drvdata?\n");
}

// PCI Device IDs table
static struct pci_device_id pci_device_ids[] = {
  {
    .vendor = OCPI_HDL_PCI_OLD_VENDOR_ID,
    .device = OCPI_HDL_PCI_OLD_DEVICE_ID,
    .subvendor = PCI_ANY_ID,
    .subdevice = PCI_ANY_ID,
    .class = (OCPI_HDL_PCI_OLD_CLASS << 16) |
             (OCPI_HDL_PCI_OLD_SUBCLASS << 8) |
             OCPI_PCI_XHCI,
    .class_mask = ~0,
  },
  {
    .vendor = OCPI_HDL_PCI_VENDOR_ID,
    .device = PCI_ANY_ID,
    .subvendor = OCPI_HDL_PCI_VENDOR_ID,
    .subdevice = PCI_ANY_ID,
    .class = (OCPI_HDL_PCI_CLASS << 16) |
             (OCPI_HDL_PCI_SUBCLASS << 8) |
             OCPI_PCI_XHCI,
    .class_mask = ~0,
  },
  {}
};
MODULE_DEVICE_TABLE(pci, pci_device_ids);

// PCI Driver functions
static struct pci_driver opencpi_pci_driver = {
  .name = DRIVER_NAME,
  .id_table = pci_device_ids,
  .probe = probe_pci,
  .remove = remove_pci,
};
#endif
#ifdef OCPI_NET
// -----------------------------------------------------------------------------------------------
// Ethernet packet support
// -----------------------------------------------------------------------------------------------
// A callback when things happen to a network interface
static int
net_notify(struct notifier_block *this, unsigned long msg, void *data) {
#if 0
  struct net_device *dev = (struct net_device *)data;
  Since we have a socket per interface, it might be useful to know when it is dead.
  switch (msg) {
  case NETDEV_UP:
  case NETDEV_DOWN:
  case NETDEV_REBOOT:
  case NETDEV_CHANGE:
  case NETDEV_REGISTER:
  case NETDEV_UNREGISTER:
  case NETDEV_CHANGEMTU:
  case NETDEV_CHANGEADDR:
  case NETDEV_GOING_DOWN:
  case NETDEV_CHANGENAME:
  case NETDEV_FEAT_CHANGE:
  case NETDEV_BONDING_FAILOVER:
  case NETDEV_PRE_UP:
  case NETDEV_RELEASE:
  case NETDEV_JOIN:
  }
#endif
  return NOTIFY_DONE;
}
static const struct proto_ops opencpi_socket;
static struct proto opencpi_proto;
//static DEFINE_MUTEX(net_mutex);

// A user level socket is being created - do our part
static int
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 3, 0) || defined(OCPI_RH6)
net_create(struct net *net, struct socket *sock, int protocol, int kern) {
#else
net_create(struct socket *sock, int protocol) {
#endif
  struct sock *sk;
  if (sock->type != SOCK_DGRAM)
    return -ESOCKTNOSUPPORT;
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 3, 0)
  if ((sk = sk_alloc(net, PF_OPENCPI, GFP_KERNEL, &opencpi_proto, 1)) == NULL)
#elif defined(OCPI_RH6)
  if ((sk = sk_alloc(net, PF_OPENCPI, GFP_KERNEL, &opencpi_proto)) == NULL)
#else
  if ((sk = sk_alloc(PF_OPENCPI, GFP_KERNEL, &opencpi_proto, 1)) == NULL)
#endif
    return -ENOBUFS;
  sock->state = SS_UNCONNECTED;
  sock->ops = &opencpi_socket;
  // if we just use the minimal sockcommon, we might not need this ?
  sock_init_data(sock, sk); // this sets refcnt to 1
  sock_reset_flag(sk, SOCK_ZAPPED); // weird initialization
  // other sk fields: sk_reuse, sk_bound_dev_if!!
  get_ocpi_sk(sk)->sockaddr.ocpi_family = PF_OPENCPI;
  return 0;
}
// The user is giving us more information about a socket.
// - All sockets are bound to an interface
// - CP discovery in user mode means send to broadcast, but receive source address
// - CP master means bind to other end's address
// - CP emulation means accept
// - DP means bind to local endpoint id, but pass
static int
net_bind(struct socket *sock, struct sockaddr *uaddr, int addr_len) {
  struct net_device *d = NULL;
  ocpi_sockaddr_t *s = (ocpi_sockaddr_t *)uaddr;
  ocpi_sock_t *sk = get_ocpi_sk(sock->sk);

  log_debug("bind alen %d family %u role %u ifi %u remote[0] %x ep %d\n",
	    addr_len, s->ocpi_family, s->ocpi_role, s->ocpi_ifindex, s->ocpi_remote[0],
	    s->ocpi_endpoint);
  if (s->ocpi_ifindex == 0) {
    int n;
    for (n = 1; n < 20; n++) {
#ifdef OCPI_RH6
      struct net_device *d = dev_get_by_index(&init_net, n);
#else
      struct net_device *d = dev_get_by_index(n);
#endif
      if (d != NULL && d->type == ARPHRD_ETHER &&
	  (dev_get_flags(d) & (IFF_UP|IFF_LOWER_UP)) == (IFF_UP|IFF_LOWER_UP))
	break;
    }
    if (n >= 20)
      return -ENODEV;
    s->ocpi_ifindex = n;
  }
  if (addr_len < sizeof(ocpi_sockaddr_t) ||
      s->ocpi_family != PF_OPENCPI ||
      s->ocpi_role == ocpi_none || s->ocpi_role >= ocpi_role_limit ||
#ifdef OCPI_RH6
      (d = dev_get_by_index(&init_net, s->ocpi_ifindex)) == NULL ||
#else
      (d = dev_get_by_index(s->ocpi_ifindex)) == NULL ||
#endif
      (s->ocpi_role == ocpi_master &&
       (is_zero_ether_addr(s->ocpi_remote) ||
	is_broadcast_ether_addr(s->ocpi_remote))))
    return -EINVAL;
  // Note that discovery can have an address.
  sk->any =
    s->ocpi_role == ocpi_slave ||
    (s->ocpi_role == ocpi_discovery &&
     (is_zero_ether_addr(s->ocpi_remote) ||
      is_broadcast_ether_addr(s->ocpi_remote)));
  sk->netdev = d;
  sk->sockaddr = *s;
  write_lock_bh(&opencpi_sklist_lock);
  sk_add_node(sock->sk, &opencpi_sklist[s->ocpi_role]);
  write_unlock_bh(&opencpi_sklist_lock);
  return 0;
}
// A user level socket is going away, do our part
static int
net_release(struct socket *sock) {
  struct sock *sk = sock->sk;
  if (sk) {
    // FIXME: verify how much of this is needed...
    lock_sock(sk);
    sock_orphan(sk);
    sock->sk = NULL;
    write_lock_bh(&opencpi_sklist_lock);
    sk_del_node_init(sk);
    write_unlock_bh(&opencpi_sklist_lock);
    skb_queue_purge(&sk->sk_receive_queue);
    release_sock(sk);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
    log_debug("socket %p count %d\n", sock, refcount_read(&sk->sk_refcnt));
#else
    log_debug("socket %p count %d\n", sock, atomic_read(&sk->sk_refcnt));
#endif
    sock_put(sk); // decrement ref count from sock_init_data
  }
  return 0;
}

// Receive an ethernet packet.
// skb->data points past the ethernet header
// sk_mac_header(skb) points to the mac header, which is before the
static int
net_receive_cp(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt,
	    struct net_device *orig_dev) {

  log_debug("Got cp packet: %d %d %d %p %p %d %zd %p %ld\n",
	    skb_headlen(skb), skb->len, dev->ifindex, skb->data, skb_mac_header(skb), skb->pkt_type,
	    sizeof(struct ethhdr), skb->data - sizeof(struct ethhdr),
	    ((ulong)skb_mac_header(skb)) & 3);
  // Handle the packet if it meets all our assumptions
  if (// dev_net(dev) == &init_net &&                      // needed > 2.6.11?
      skb->pkt_type != PACKET_OTHERHOST &&                 // ignore promiscuous receives
      skb_mac_header(skb) == skb->data - sizeof(struct ethhdr) && // has an etherheader directly before
      (skb = skb_share_check(skb, GFP_ATOMIC)) != NULL) {  // get our own copy to pass to user
    ocpi_role_t role;
    unsigned char *source = eth_hdr(skb)->h_source;
    struct sock *sk;
    struct hlist_node *node;
    EtherControlHeader *ech;
    unsigned my_len;
    log_debug("packet accepted2\n");

    __skb_push(skb, sizeof(uint16_t));                     // make our "payload" start with ethertype
    // Note we are MISALIGNED here so we should not access 32 bit fields.
    ech = (EtherControlHeader *)skb->data;
    my_len = ntohs(ech->length) + sizeof(uint16_t);
    if (my_len <= skb->len && my_len >= sizeof(EtherControlHeader)) {
      ocpi_sock_t *found = NULL;
      skb_trim(skb, my_len);
      if (OCCP_ETHER_MESSAGE_TYPE(ech->typeEtc) == OCCP_RESPONSE)
	if (OCCP_ETHER_UNCACHED(ech->typeEtc))
	  role = ocpi_discovery;
	else
	  role = ocpi_master;
      else
	role = ocpi_slave;
      ocpi_sk_for_each(sk, node, &opencpi_sklist[role]) {
	ocpi_sock_t *osk = get_ocpi_sk(sk);
	if (osk->any)
	  found = osk;
	else if
#ifdef OCPI_RH5
	(!compare_ether_addr(osk->sockaddr.ocpi_remote, source))
#else
	(ether_addr_equal(osk->sockaddr.ocpi_remote, source))
#endif
	  {
	  found = osk;
	  break;
	}
      }
      if (found) {
	memcpy(skb->cb, &found->sockaddr, offsetof(ocpi_sockaddr_t, ocpi_remote));
	memcpy(((ocpi_sockaddr_t *)skb->cb)->ocpi_remote, source, ETH_ALEN);
	if (sock_queue_rcv_skb(get_net_sk(found), skb) == 0)
	  return NET_RX_SUCCESS;
      }
    }
  }
  log_debug("dropped\n");
  kfree_skb(skb);
  return NET_RX_DROP;
}
static int
net_receive_dp(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt,
	    struct net_device *orig_dev) {
  log_debug("Got dp packet: %d %d %d %p %p\n",
	    skb_headlen(skb), skb->len, dev->ifindex, skb->data, skb_mac_header(skb));
  // Handle the packet if it meets all our assumptions
  if (// dev_net(dev) == &init_net &&                      // needed > 2.6.11?
      skb->pkt_type != PACKET_OTHERHOST &&                 // ignore promiscuous receives
      skb_mac_header(skb) == skb->data - sizeof(struct ethhdr) && // has an etherheader directly before
      //      ((ulong)skb_mac_header(skb) & 3) == 0 &&         // alignment as expected
      (skb = skb_share_check(skb, GFP_ATOMIC))) {          // get our own copy to pass to user
    unsigned char *source = eth_hdr(skb)->h_source;
    struct sock *sk;
    struct hlist_node *node; // unused in later kernels

    __skb_push(skb, sizeof(uint16_t));                     // make our "payload" start with ethertype
    ocpi_sk_for_each(sk, node, &opencpi_sklist[ocpi_data]) {
      ocpi_sock_t *osk = get_ocpi_sk(sk);

      if (((uint16_t *)skb->data)[1] == osk->sockaddr.ocpi_endpoint) {
	memcpy(osk->sockaddr.ocpi_remote, source, sizeof(osk->sockaddr.ocpi_remote));
	memcpy(&skb->cb, &osk->sockaddr, sizeof(osk->sockaddr));
	if (sock_queue_rcv_skb(sk, skb) == 0)
	  return NET_RX_SUCCESS;
	else
	  break;
      }
    }
  }
  log_debug("dropped\n");
  kfree_skb(skb);
  return NET_RX_DROP;
}
static int net_recvmsg
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
  (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t total_len, int flags)
#else
  (struct socket *sock, struct msghdr *msg, size_t total_len, int flags)
#endif
{
  int error = 0;
  struct sk_buff *skb =
    skb_recv_datagram(sock->sk, flags & ~MSG_DONTWAIT, flags & MSG_DONTWAIT, &error);
  if (skb != NULL) {
    total_len = min_t(size_t, total_len, skb->len);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
    error = skb_copy_datagram_iovec(skb, 0, msg->msg_iov, total_len);
#else
    error = skb_copy_datagram_iter(skb, 0, &msg->msg_iter, total_len);
#endif
    if (error == 0) {
      msg->msg_namelen = sizeof(ocpi_sockaddr_t);
      if (msg->msg_name)
	memcpy(msg->msg_name, skb->cb, sizeof(ocpi_sockaddr_t));
      error = total_len;
    }
    kfree_skb(skb);
  }
  return error;
}

// our sockets our bound to an interface except for data,
// which is dynamic
static int net_sendmsg
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)
  (struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t total_len) {
#else
  (struct socket *sock, struct msghdr *msg, size_t total_len) {
#endif
  struct sock *sk = sock->sk;
  struct net_device *dev = get_ocpi_sk(sk)->netdev;
  ocpi_sockaddr_t *mysa = &get_ocpi_sk(sk)->sockaddr;
  size_t actual_len = total_len - sizeof(uint16_t);
  struct sk_buff *skb;
  int error;


  lock_sock(sk);
  if (sock_flag(sk, SOCK_DEAD) || mysa->ocpi_role == ocpi_none)
    error = -ENOTCONN;
  else if (total_len > (dev->mtu + dev->hard_header_len))
    error = -EMSGSIZE;
  else if (mysa->ocpi_role != ocpi_master &&
	   (!msg->msg_name || msg->msg_namelen < sizeof(ocpi_sockaddr_t)))
    error = -EINVAL;
  else if ((skb = sock_wmalloc(sk, actual_len + dev->hard_header_len + 32,
			       0, GFP_KERNEL)) == NULL)
    error = -ENOMEM;
  else {
    unsigned char *data;
    unsigned short etype = mysa->ocpi_role == ocpi_data ? ETH_P_OCPI_DP : ETH_P_OCPI_CP;

    skb_reserve(skb, dev->hard_header_len);
    skb_reset_network_header(skb);
    skb->dev = dev;
    skb->priority = sk->sk_priority;
    skb->protocol = etype;
    data = skb_put(skb, actual_len);
    if ((error =
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
                 memcpy_fromiovecend(data, msg->msg_iov, sizeof(uint16_t), actual_len)) < 0
#else
                 copy_from_iter(data, actual_len, &msg->msg_iter)) < 0
#endif
    )
      kfree_skb(skb);
    else {
      error = total_len;
#ifdef OCPI_RH6
      dev_hard_header(skb, dev, etype,
#else
      dev->hard_header(skb, dev, etype,
#endif
		       mysa->ocpi_role == ocpi_master ?
		       mysa->ocpi_remote : ((ocpi_sockaddr_t *)msg->msg_name)->ocpi_remote,
		       NULL, actual_len);
      dev_queue_xmit(skb);
    }
  }
  release_sock(sk);
  return error;
}
static const struct proto_ops opencpi_socket = {
  .family =	PF_OPENCPI,
  .owner =	THIS_MODULE,
  .release =	net_release,
  .bind =	net_bind, // net_bind,
  .connect =	sock_no_connect,
  .socketpair =	sock_no_socketpair,
  .accept =	sock_no_accept,
  .getname =	sock_no_getname, // net_getname,
  .poll =       datagram_poll,
  .ioctl =	sock_no_ioctl, // net_ioctl,
  .listen =	sock_no_listen,
  .shutdown =	sock_no_shutdown,
  .setsockopt =	sock_no_setsockopt,
  .getsockopt =	sock_no_getsockopt,
  .sendmsg =	net_sendmsg,
  .recvmsg =	net_recvmsg,
  .mmap =       sock_no_mmap,
  .sendpage =	sock_no_sendpage,
};
static struct proto opencpi_proto = {
  .name	  = "OCPI_ETHER",
  .owner  = THIS_MODULE,
  .obj_size = sizeof(ocpi_sock_t),
};
static struct net_proto_family opencpi_family = {
  .family = PF_OPENCPI,
  .create = net_create,
  .owner = THIS_MODULE,
};
static struct packet_type opencpi_packet_type_cp = {
	.type =		__constant_htons(ETH_P_OCPI_CP),
	.func =		net_receive_cp,
};
static struct packet_type opencpi_packet_type_dp = {
	.type =		__constant_htons(ETH_P_OCPI_DP),
	.func =		net_receive_dp,
};
static struct notifier_block opencpi_notifier = {
  .notifier_call = net_notify,
};
#endif
// -----------------------------------------------------------------------------------------------
// Module level functions for when we load and unload the driver
// -----------------------------------------------------------------------------------------------

// release all known resources from this driver, if errors at load time, or at unload time
static void
free_driver(void) {
  log_debug( "Cleanup entered\n");
#ifdef OCPI_NET
  if (opencpi_notifier_registered) {
    unregister_netdevice_notifier(&opencpi_notifier);
    opencpi_notifier_registered = false;
  }
  if (opencpi_packet_type_registered) {
    dev_remove_pack(&opencpi_packet_type_cp);
    dev_remove_pack(&opencpi_packet_type_dp);
    opencpi_packet_type_registered = false;
  }
  if (opencpi_family_registered) {
    sock_unregister(opencpi_family.family);
    opencpi_family_registered = false;
  }
  if (opencpi_proto_registered) {
    proto_unregister(&opencpi_proto);
    opencpi_proto_registered = false;
  }
#endif
#ifdef CONFIG_PCI
  if (opencpi_pci_registered) {
    // This will/might remove all the pci devices
    pci_unregister_driver(&opencpi_pci_driver);
    opencpi_pci_registered = false;
  }
#endif
  // merge_free_memory now relies on devices still being available
  if (block_init) {
    ocpi_block_t *block, *temp;
    // FIXME:  how to shut down dma from devices...
    spin_lock(&block_lock);
    list_for_each_entry_safe(block, temp, &block_list, list)
      free_block(block);
    spin_unlock(&block_lock);
    merge_free_memory();
    spin_lock(&block_lock);
    list_for_each_entry_safe(block, temp, &block_list, list) {
      if (block->type == ocpi_kernel)
	free_kernel_block(block);
      list_del(&block->list);
      kfree(block);
    }
    spin_unlock(&block_lock);
    block_init = 0;
  }
  if (opencpi_devices) {
    unsigned n;
    for (n = 0; n <= opencpi_ndevices; n++)
      if (opencpi_devices[n])
	free_device(opencpi_devices[n]);
    kfree(opencpi_devices);
    opencpi_devices = NULL;
  }
  log_debug("removed all our devices\n");
  if (opencpi_class)
    class_destroy(opencpi_class);
  if (opencpi_device_number)
    unregister_chrdev_region(opencpi_device_number, opencpi_max_devices + 1);
  log_debug("removed our class and device number allocations\n");
  log_debug("Cleanup done\n");
}

#ifdef CONFIG_ARCH_ZYNQ
#if 0 // here if we ever want it again..
static void enable_counters(void*info) {
  /* enable user-mode access to the performance counter*/
  asm volatile("MCR p15, 0, %0, C9, C14, 0\n\t" :: "r"(1));
  /* disable counter overflow interrupts (just in case)*/
  asm volatile("MCR p15, 0, %0, C9, C14, 2\n\t" :: "r"(0x8000000f));
  // program the performance-counter control-register:
  asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(0x11));
  /* enable all counters */
  asm volatile("mcr p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));
  // clear overflows:
  asm volatile ("MCR p15, 0, %0, c9, c12, 3\t\n" :: "r"(0x8000000f));
}
#endif
#endif

// Initialize the driver - at load time
static int __init
opencpi_init(void) {
  ocpi_address_t physical = 0;
  ocpi_device_t *mydev = NULL;
  long err = -EIO;

  log_debug("loading/initializing\n");
  spin_lock_init(&block_lock );
  INIT_LIST_HEAD(&block_list);
  block_init = 1;

  do {
    // Create a driver class for this module: sets opencpi_class
    opencpi_class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(opencpi_class)) {
      log_err_code(PTR_ERR(opencpi_class), "Error creating class");
      opencpi_class = NULL;
    }
    // Allocate a Major/Minor device number: sets opencpi_device_number non-zero)
    log_debug("Allocating region for %ld devices\n", opencpi_max_devices + 1);
    if ((err = alloc_chrdev_region( &opencpi_device_number, 0,
				    opencpi_max_devices + 1, DRIVER_NAME)) != 0) {
      log_err_code(err, "Can't register device");
      break;
    }
    // Allocate our array of device pointers: sets opencpi_devices
    if ((opencpi_devices = kcalloc(opencpi_max_devices + 1,
				   sizeof(ocpi_device_t *), GFP_KERNEL)) == NULL) {
      log_err("Can't allocate pointers for opencpi_max_devices (%ld)\n", opencpi_max_devices);
      break;
    }
    // Create the base device: sets mydev (and add to opencpi_devices)
    if ((mydev = make_device(0)) == NULL)
      break;
    // Add the cdev of the base device (and records this in mydev)
    if ((err = add_cdev(mydev)))
      break;
    // Create a kernel device object (and sysfs and /dev) for the base device: sets mydev->fsdev
    {
      struct device *fsdev =
#ifdef OCPI_RH6
	device_create(opencpi_class, NULL, opencpi_device_number, NULL, "%s=mem", DRIVER_ABBREV);
#else
	device_create(opencpi_class, NULL, opencpi_device_number, "%s=mem", DRIVER_ABBREV);
#endif
      if (fsdev == NULL || IS_ERR(fsdev)) {
	log_err_code(PTR_ERR(fsdev), "Error creating device");
	break;
      }
      mydev->fsdev = fsdev;
      log_debug("creating device in sysfs: %p kname '%s'\n", fsdev, fsdev->kobj.name);
    }

#ifdef CONFIG_PCI
    // Register as a PCI driver, which might cause probe callbacks: sets opencpi_pci_registered
    if (pci_register_driver(&opencpi_pci_driver) != 0) {
      log_err("Can't register pci device\n");
      break;
    }
    opencpi_pci_registered = 1;
#endif
#ifdef CONFIG_ARCH_ZYNQ
    // Register the memory range of the control plane GP0 or GP1 on the PL
    if (make_block(0x40000000, 0x40000000, sizeof(OccpSpace), ocpi_mmio, false, 0,
		   NULL, 0) == NULL ||
        make_block(0x80000000, 0x80000000, sizeof(OccpSpace), ocpi_mmio, false, 0,
		   NULL, 0) == NULL)
      break;
    log_debug("Control Plane physical addr space for Zynq/PL/AXI GP0 and GP1 slave reserved\n");
    {
#if 0
     int
       online = num_online_cpus(),
       possible = num_possible_cpus(),
       present = num_present_cpus();
     printk(KERN_INFO "Online Cpus=%d\nPossible Cpus=%d\nPresent Cpus=%d\n",
	    online, possible, present);
     on_each_cpu(enable_counters , NULL, 1);
#endif
    }
#endif
    // Allocate initial memory space: sets virtual/physical/opencpi_size: set opencpi_allocation
    // TODO: Automatically detect 'opencpi_memmap'/'memmap' on the Kernel commandline (currently handled by ocpi_linux_driver script)
    log_debug("parameters (opencpi_size = %ld (0x%lx), opencpi_memmap = %s)\n", opencpi_size, opencpi_size, opencpi_memmap );
    if (opencpi_memmap) { // If there is already a reserved block of memory, use it
      char *p = NULL;
      opencpi_size = memparse(opencpi_memmap, &p);
      if (*p++ == '$') {
	physical = memparse(p, &p); // Handle K/M/G etc for address as well. (AV-1710)
        // Don't care about return pointer, but kernel made return optional between 2.4 and 3.10.
#if 0
	if (!page_is_ram(physical >> PAGE_SHIFT)) {
	  log_err("reserved memory: %s, not a valid ram address\n", opencpi_memmap);
	  break;
	}
	// If page_is_ram, then the physical should convert ok.
	// FIXME: But there are probably better checks
#endif
      }
      if (make_block(physical, physical, opencpi_size, ocpi_reserved, true, 0, NULL, 0) == NULL)
	break;
      // log_debug("parameters after parsing (opencpi_memmap = %s => opencpi_size = %ld (0x%lx) @ physical = 0x%lx)\n", opencpi_memmap, opencpi_size, opencpi_size, physical);
      log_debug("Using reserved memory %lx @ %llx\n", opencpi_size, physical);
    } else {
      // if memmap wasn't set, then try using the DMA API
      // if the kernel was compiled with CMA support and the allocation has been requested
      // either on the boot cmdline or by a kernel parameter then the memory chould be contiguous
      // Using the DMA API is preferred to memmap or page allocation
      // the fallback is to request kernel pages, but that can fail with limited memory
      ocpi_request_t request;
      if (opencpi_size == -1)
	opencpi_size = OPENCPI_INITIAL_MEMORY_ALLOCATION;
      request.needed = opencpi_size;
      request.how_cached = ocpi_uncached;
      if ((err = get_dma_memory (&request,0)) != 0) {
	log_err("get_dma_memory failed in opencpi_init, trying fallback\n");
        for (request.needed = opencpi_size; request.needed >= OPENCPI_MINIMUM_MEMORY_ALLOCATION;
	     request.needed /= 2)
	  if ((err = request_memory(NULL, &request)) == 0)
	    break;
      }
      if (err) {
        log_err("get_dma_memory and fallback failed in opencpi_init\n");
        break;
      }
      opencpi_size = request.actual;
      log_debug("Using allocated kernel memory size %lx\n", opencpi_size);
    }
#ifdef OCPI_NET
    if ((err = proto_register(&opencpi_proto, 0))) {
      log_err("protocol registration failed (%ld)\n", err);
      break;
    }
    opencpi_proto_registered = true;
    if ((err = sock_register(&opencpi_family))) {
      log_err("family registration failed (%ld)\n", err);
      break;
    }
    opencpi_family_registered = true;
    dev_add_pack(&opencpi_packet_type_cp); // no error code
    dev_add_pack(&opencpi_packet_type_dp); // no error code
    opencpi_packet_type_registered = true;
    if ((err = register_netdevice_notifier(&opencpi_notifier))) {
      log_err("notifier registration failed (%ld)\n", err);
      break;
    }
    opencpi_notifier_registered = true;
#endif
    log_debug("driver loaded on device (%d,%d)\n", MAJOR(opencpi_device_number), MINOR(opencpi_device_number));
    return 0;
  } while (0);
  free_driver();
  return -EIO;
}

/**
 * opencpi_exit -- release kernel resources when the module is unloaded (rmmod)
 */
static void __exit
opencpi_exit(void) {
  log_debug("unloading\n");

  free_driver();

  log_debug("unloaded\n");
}

// Declare the init/exit functions for this module

module_init(opencpi_init);
module_exit(opencpi_exit);

// Declare the kernel metadata

module_param(opencpi_size, long, 0);
MODULE_PARM_DESC(opencpi_size, "Amount of memory to pre-allocate");
module_param(opencpi_memmap, charp, 0);
MODULE_PARM_DESC(opencpi_memmap, "Copy of kernel memmap parameter, if it exists");
module_param(opencpi_max_devices, long, 0);
MODULE_PARM_DESC(opencpi_max_devices, "Number of devices to support");

MODULE_AUTHOR("Jim Kulp/Craig Trader");
MODULE_DESCRIPTION("OpenCPI Memory/PCI Driver");
MODULE_LICENSE("Dual BSD/GPL");
