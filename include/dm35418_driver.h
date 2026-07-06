/**
    @file

    @brief
        Structures and defines for the DM35418 driver module.


    $Id: dm35418_driver.h 141380 2024-02-29 20:50:59Z lfrankenfield $
*/

//----------------------------------------------------------------------------
//  COPYRIGHT (C) RTD EMBEDDED TECHNOLOGIES, INC.  ALL RIGHTS RESERVED.
//
//  This software package is dual-licensed.  Source code that is compiled for
//  kernel mode execution is licensed under the GNU General Public License
//  version 2.  For a copy of this license, refer to the file
//  LICENSE_GPLv2.TXT (which should be included with this software) or contact
//  the Free Software Foundation.  Source code that is compiled for user mode
//  execution is licensed under the RTD End-User Software License Agreement.
//  For a copy of this license, refer to LICENSE.TXT or contact RTD Embedded
//  Technologies, Inc.  Using this software indicates agreement with the
//  license terms listed above.
//----------------------------------------------------------------------------

#ifndef __DM35418_DRIVER_H__
#define __DM35418_DRIVER_H__


#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/types.h>

/*=============================================================================
Constants
 =============================================================================*/

/**
 * @defgroup DM35418_Driver_Constants DM35418 Driver Constants
 * @{
 */

/**
 * @brief
 * DM35418 Maximum number of devices
 */
#define DM35418_MAX_NUM_DEVICES 	8

/**
 * @brief
 * DM35418 Number of minors per device
 */
#define DM35418_NUM_MINORS_PER_DEVICE 1

/**
 * @brief
 * DM35418 Max possible board name length.
 */
#define DM35418_NAME_LENGTH 200

/**
 * @brief
 * Number of standard PCI regions
 */

#define DM35418_PCI_NUM_REGIONS	  PCI_ROM_RESOURCE

/**
 * @brief
 * Number of interrupts to hold in a queue for processing
 */
 #define DM35418_INT_QUEUE_SIZE		256

/**
 * @} DM35418_Driver_Constants
 */


/*=============================================================================
Enumerations
 =============================================================================*/

/**
 * @defgroup DM35418_Driver_Enumerations DM35418 Driver Enumerations
 * @{
 */

/**
 * @brief
 * Direction of access to standard PCI region
 */

enum dm35418_pci_region_access_dir {

	/**
	 * Read from the region
	 */

	DM35418_PCI_REGION_ACCESS_READ = 0,

	/**
	 * Write to the region
	 */

	DM35418_PCI_REGION_ACCESS_WRITE
};


/**
 * @} DM35418_Driver_Enumerations
 */



/*=============================================================================
Structures
 =============================================================================*/

/**
 * @defgroup DM35418_Driver_Structures DM35418 Driver Structures
 * @{
 */

/**
 * @brief
 *	  DM35418 PCI region descriptor.  This structure holds information about
 *	  one of a device's PCI memory regions.
 */

struct dm35418_pci_region {

	/**
	 * I/O port number if I/O mapped
	 */

	unsigned long io_addr;

	/**
	 * Length of region in bytes
	 */

	unsigned long length;

	/**
	 * Region's physical address if memory mapped or I/O port number if I/O
	 * mapped
	 */

	unsigned long phys_addr;

	/**
	 * Address at which region is mapped in kernel virtual address space if
	 * memory mapped
	 */

	void __iomem *virt_addr;

	/**
	 * Flag indicating whether or not the I/O-mapped memory ranged was
	 * allocated.  A value of zero means the memory range was not allocated.
	 * Any other value means the memory range was allocated.
	 */

	uint8_t allocated;
};

/**
 * @brief
 *	  DM35418 DMA descriptor.  This structure holds information about
 *	  a single DMA buffer.
 */

struct dm35418_dma_descriptor {

	/**
	 * Function block number this DMA is associated with.
	 */
	uint32_t fb_num;

	/**
	 * DMA channel this buffer is in.
	 */
	int channel;

	/**
	 * DMA buffer number this descriptor represents.
	 */
	int buffer;

	/**
	 * System memory address for buffer
	 */
	void *virt_addr;


	/**
	 * Bus memory address for buffer.
	 */
	dma_addr_t bus_addr;


	/**
	 * Size of this allocated buffer
	 */
	unsigned int buffer_size;


	/**
	 * List head so that descriptors can be kept in a linked list.
	 */
	struct list_head list;


};


/**
 * @brief
 *	  DM35418 Device Descriptor.  The identifying info for this
 *        particular board.
 */

struct dm35418_device_descriptor {

	/**
	 * Device name used when requesting resources; a NUL terminated string of
	 * the form rtd-dm35418-x where x is the device minor number.
	 */

	char name[DM35418_NAME_LENGTH];

	/**
	 * Device index
	 */
	int device_index;

	/**
	 * Information about each of the standard PCI regions
	 */

	struct dm35418_pci_region pci[PCI_ROM_RESOURCE];

	/**
	 * PCI device pointer
	 */
	struct pci_dev * pdev;

	/**
	 * Concurrency control
	 */

	spinlock_t device_lock;

	/**
	 * Character device
	 */
	struct cdev cdev;

	/**
	 * Number of entities which have the device file open.  Used to enforce
	 * single open semantics.
	 */

	uint8_t reference_count;

	/**
	 * IRQ line number
	 */

	unsigned int irq_number;


	/**
	* Used to assist poll in shutting down the thread waiting for interrupts
	*/

	uint8_t remove_isr_flag;

	/**
	* Queue of processes waiting to be woken up when an interrupt occurs
	*/

	wait_queue_head_t int_wait_queue;

	/**
	* Queue of processes waiting to be woken up when an interrupt occurs
	*/

	wait_queue_head_t dma_wait_queue;

	/**
	* Interrupt queue containing which functional blocks caused interrupts
	*/

	int interrupt_fb[DM35418_INT_QUEUE_SIZE];


	/**
	* Number of interrupts missed because of a full queue
	*/

	unsigned int int_queue_missed;

	/**
	* Number of interrupts currently in the queue
	*/

	unsigned int int_queue_count;

	/**
	* Where in the queue new entries are put
	*/

	unsigned int int_queue_in_marker;

	/**
	* Where in the queue entries are pulled from
	*/

	unsigned int int_queue_out_marker;


	/**
	 * A list of all allocated DMA buffers
	 */
	struct list_head dma_descr_list;


};

/**
 * @brief
 *    Placeholder protoype for file ops struct
 */
static struct file_operations dm35418_file_ops;

/**
 * @} DM35418_Driver_Structures
 */


#endif

