/**
    @file

    @brief
        Structures for the DM35418 Board Access Library.

    $Id: dm35418_board_access.h 104840 2016-11-30 19:20:54Z rgroner $
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

#ifndef _DM35418_BOARD_ACCESS__H_
#define _DM35418_BOARD_ACCESS__H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "dm35418_board_access_structs.h"
#include "dm35418_types.h"

/**
 * Conditionally set up the library export symbol for the Windows
 * DLL. This will expand to nothing when compiled for Linux.
 */
#ifdef _WIN32
    #ifdef DM35418LIB_EXPORT /* defined in Visual Studio project file */
        #define DM35418LIB_API __declspec(dllexport)
    #else
        #define DM35418LIB_API __declspec(dllimport)
    #endif
#else /* Linux, expands to nothing */
    #define DM35418LIB_API
#endif

#include "dm35418_os.h"

/**
 * @defgroup DM35418_Board_Access_Structures DM35418 Board Access Structures
 * @{
 */

/*=============================================================================
Structures
 =============================================================================*/

/**
 * @brief
 *    Descriptor for the DMA on this board.
 */
struct DM35418_DMA_Descriptor {

	/**
	 * Offset to the DMA control register section
	 */
	uint32_t control_offset;

	/**
	 * Number of buffers for this DMA channel.
	 */
	uint8_t num_buffers;

	/**
	 * Offset to the beginning of the buffer control section.
	 */
	uint32_t buffer_start_offset[MAX_DMA_BUFFERS];

};



/**
  @brief
  DM35418 function block descriptor.  This structure holds information about
  a function block, including type, number of DMA channels and buffers, descriptors
  for each DMA channel, and memory offsets to various control locations.
 */

struct DM35418_Function_Block {

	/**
	 * Type of function block (ADC, DAC, DIO, etc)
	 */
	uint16_t type;

	/**
	 * Type of specific function block (ADC1, ADC2, ADC3, etc)
	 */
	uint16_t sub_type;

	/**
	 * Revision of subtype (internal use only)
	 */

	uint16_t type_revision;

	/**
	 * Offset to the beginning of the function block registers
	 */
	uint32_t fb_offset;

	/**
	 * Offset to the beginning of the DMA registers for this function block
	 */
	uint32_t dma_offset;

	/**
	 * Function block num (as identified in GBC)
	 */
	int fb_num;

	/**
	 * The ordinal number of this particular function block
	 * type (0th, 1st, etc)
	 */
	int ordinal_fb_type_num;

	/**
	 * Number of DMA buffers in this function block
	 */
	uint8_t num_dma_buffers;

	/**
	 * Number of DMA channels in this function block
	 */
	uint8_t num_dma_channels;

	/**
	 * Offset to the beginning of the control registers for this function block
	 */
	uint32_t control_offset;

	uint8_t control_size;

	uint8_t channel_size;

	/**
	 * Array of descriptors for each DMA channel
	 */
	struct DM35418_DMA_Descriptor dma_channel[MAX_DMA_CHANNELS];


};



/********************************************************************
* Board Level Functions
*********************************************************************/



/**
*******************************************************************************
@brief
    Open the board, providing the file descriptor that all future operations
    will reference.  Also allocate memory for the device descriptor.

@param
    dev_num

    The minor number of the device being opened.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@retval
    0

    Success.

@retval
    -1

    Failure.
    errno may be set as follows:
        @arg \c
            EBUSY	Cannot open specified device.
            ENOMEM	Cannot allocate memory for device descriptor.

 */
DM35418LIB_API
int
DM35418_Board_Open(uint8_t dev_num,
			struct DM35418_Board_Descriptor ** handle);


/**
*******************************************************************************
@brief
    Close the board, closing the open handle for the device file,
    and freeing the memory allocated for the decriptor.

@param
    handle

    Pointer to the device descriptor, which contains the open file id.

@retval
    0

    Success.

@retval
    -1

    Failure.
    errno may be set as follows:
        @arg \c
            ENODATA	Device handle is null.
 */
DM35418LIB_API
int DM35418_Board_Close(struct DM35418_Board_Descriptor *handle);


/**
*******************************************************************************
@brief
    Read from the board.

@param
    handle

    Pointer to the device descriptor, which contains the open file id.

@param
    ioctl_request

    Structure holding all required information for the read to complete, including
    register offset, data size, PCI region number, and pointer for
    returned data.

@retval
    0

    Success.

@retval
    -1

    Failure.
 */
DM35418LIB_API
int DM35418_Read(struct DM35418_Board_Descriptor *handle,
	union dm35418_ioctl_argument *ioctl_request);


/**
*******************************************************************************
@brief
    Write to the board.

@param
    handle

    Pointer to the device descriptor, which contains the open file id.

@param
    ioctl_request

    Structure holding all required information for the write to complete, including
    register offset, data size, PCI region number, and data to write.

@retval
    0

    Success.

@retval
    -1

    Failure.
 */
DM35418LIB_API
int DM35418_Write(struct DM35418_Board_Descriptor *handle,
	union dm35418_ioctl_argument *ioctl_request);


/**
*******************************************************************************
@brief
    Read/Modify/Write to the board.

@param
    handle

    Pointer to the device descriptor, which contains the open file id.

@param
    ioctl_request

    Structure holding all required information for the modify to complete, including
    register offset, data size, PCI region number, value mask, and data to write

@retval
    0

    Success.

@retval
    -1

    Failure.
 */
DM35418LIB_API
int DM35418_Modify(struct DM35418_Board_Descriptor *handle,
	union dm35418_ioctl_argument *ioctl_request);



/**
*******************************************************************************
@brief
    Perform a DMA operation.

@param
    handle

    Pointer to the device descriptor, which contains the open file id.

@param
    ioctl_request

    Structure holding all required information for request to complete, including a
    DMA descriptor.

@retval
    0

    Success.

@retval
    -1

    Failure.

@warning
    This function is not compatible with the Windows driver package and is
    therefore not included in the Windows DLL.
 */
int DM35418_Dma(struct DM35418_Board_Descriptor *handle,
    			union dm35418_ioctl_argument *ioctl_request);




/**
 * @} DM35418_Board_Access_Structures
 */

 #ifdef __cplusplus
}
#endif

#endif

