/**
    @file

    @brief
        Definitions for the DM35418 Sync Bus Library.

    $Id: dm35418_syncbus_library.h 60276 2012-06-05 16:04:15Z rgroner $
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


#ifndef _DM35418_SYNCBUS_LIBRARY__H_
#define _DM35418_SYNCBUS_LIBRARY__H_


#include "dm35418_gbc_library.h"

#ifdef __cplusplus
extern "C" {
#endif

 /**
  * @defgroup DM35418_Syncbus_Library_Constants DM35418 SyncBus Library Constants
  * @{
  */


/**
 * @brief
 *    Register value for Clock Event - Disable
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_DISABLE		0x00

/**
 * @brief
 *    Register value for Clock Event - SyncBus 0
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_SYNCBUS_0		0x80

/**
 * @brief
 *    Register value for Clock Event - SyncBus 1
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_SYNCBUS_1		0x81

/**
 * @brief
 *    Register value for Clock Event - Syncbus 2
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_SYNCBUS_2		0x82

/**
 * @brief
 *      Register value for Clock Source - Immediate
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_IMMEDIATE		0x00

/**
 * @brief
 *      Register value for Clock Source - Never
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_NEVER		0x01

/**
 * @brief
 *      Register value for Clock Source - Global 2
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_GBL2		0x02

/**
 * @brief
 *      Register value for Clock Source - Global 3
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_GBL3		0x03

/**
 * @brief
 *      Register value for Clock Source - Global 4
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_GBL4		0x04

/**
 * @brief
 *      Register value for Clock Source - Global 5
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_GBL5		0x05

/**
 * @brief
 *      Register value for Clock Source - Global 6
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_GBL6		0x06

/**
 * @brief
 *      Register value for Clock Source - Global 7
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_GBL7		0x07

/**
 * @brief
 *      Register value for Clock Source - Global 2 Inverted
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_GBL2_INV		0x0a

/**
 * @brief
 *      Register value for Clock Source - Global 3 Inverted
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_GBL3_INV		0x0b

/**
 * @brief
 *      Register value for Clock Source - Global 4 Inverted
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_GBL4_INV		0x0c

/**
 * @brief
 *      Register value for Clock Source - Global 5 Inverted
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_GBL5_INV		0x0d

/**
 * @brief
 *      Register value for Clock Source - Global 6 Inverted
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_GBL6_INV		0x0e

/**
 * @brief
 *      Register value for Clock Source - Global 7 Inverted
 */
#define DM35418_SYNCBUS_CLK_BUS_SRC_GBL7_INV		0x0f

/**
 * @brief
 *      Value for selecting SyncBus 0 Clock Source
 */
#define DM35418_SYNCBUS_0_CLK_SRC		0x00

/**
 * @brief
 *      Value for selecting SyncBus 1 Clock Source
 */
#define DM35418_SYNCBUS_1_CLK_SRC		0x01

/**
 * @brief
 *      Value for selecting SyncBus 2 Clock Source
 */
#define DM35418_SYNCBUS_2_CLK_SRC		0x02

/**
 * @brief
 *      Value for setting syncbus line to output
 */
#define DM35418_SYNCBUS_DIR_OUTPUT		1

/**
 * @brief
 *      Value for setting syncbus line to input
 */
#define DM35418_SYNCBUS_DIR_INPUT		0

/**
 * @brief
 *		Value for setting the clock select to local clock
 */
#define DM35418_SYNCBUS_CLK_SEL_LOCAL	0

/**
 * @brief
 *		Value for setting the clock select to syncbus clock
 */
#define DM35418_SYNCBUS_CLK_SEL_SYNC	1



 /**
  * @} DM35418_Syncbus_Library_Constants
  */

 /**
  * @defgroup DM35418_Syncbus_Library_Functions DM35418 Sync Bus Library Public Functions
  * @{
  */

/**
*******************************************************************************
@brief
   Open the Syncbus functional block, making it available for operations.

@param
   handle

   Address of the handle pointer, which will contain the device
   descriptor.

@param
   number
   
   Ordinal number of syncbus function block to open
   
@param
   func_block

   Pointer to the returned function block descriptor.

@retval
   0

   Success.

@retval
   Non-Zero

   Failure.
*/
DM35418LIB_API
int DM35418_Syncbus_Open(struct DM35418_Board_Descriptor *handle,
		unsigned int number,
		struct DM35418_Function_Block *func_block);


/**
*******************************************************************************
@brief
    Enable the Syncbus drivers.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the open function block descriptor.

@note
    The direction of the driver is decided by DM35418_Syncbus_Set_Output_Enable().

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35418LIB_API
int DM35418_Syncbus_Enable(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block);


/**
*******************************************************************************
@brief
    Disable the Syncbus drivers.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the open function block descriptor.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35418LIB_API
int DM35418_Syncbus_Disable(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block);


/**
*******************************************************************************
@brief
    Enable the 100 Ohm differential termination on the Syncbus.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the open function block descriptor.

@note
    The termination must be enabled on both end of the bus,
    but must not be enabled on the boards in the middle of the bus.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35418LIB_API
int DM35418_Syncbus_Termination_Enable(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block);


/**
*******************************************************************************
@brief
    Disable the 100 Ohm differential termination on the Syncbus.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the open function block descriptor.

@note
    The termination must be enabled on both end of the bus,
    but must not be enabled on the boards in the middle of the bus.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35418LIB_API
int DM35418_Syncbus_Termination_Disable(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block);


/**
*******************************************************************************
@brief
    Set the direction of the individual Syncbus lines.
    There must be only one board that has output direction enabled (the master),
    the other board should be configured as input (slave).
    The syncbus0..syncbus2 lines are data (trigger) lines, which can be connected
    to the internal clock bus.
    The syncbus3 is the 10MHz clock line. One of the boards must be the master,
    driving that line, the others are slave, synchronizing their PLL from the
    Syncbus clock.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the open function block descriptor.

@param
    dir_0

    Direction of syncbus 0.

@param
    dir_1

    Direction of syncbus 1.

@param
    dir_2

    Direction of syncbus 2.

@param
    dir_3

    Direction of syncbus 3.

@note
    For Direction, 0 == Input, 1 == Output

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35418LIB_API
int DM35418_Syncbus_Set_Direction(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block,
				int dir_0, int dir_1, int dir_2, int dir_3);


/**
*******************************************************************************
@brief
    Set master clock source of the board. It can be either the local onboard
    oscillator (clksel=0) or the syncbus clock line (clksel=1, provided that
    the PLL is locked).

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the open function block descriptor.

@param
    clksel

    boolean value of clk select.
    	0 (or false):	onboard oscillator
    	1 (or true):	syncbus clock PLL

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35418LIB_API
int DM35418_Syncbus_Set_Clk_Sel(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block,
				int clksel);


/**
*******************************************************************************
@brief
    Get master clock source of the board. It can be either the local onboard
    oscillator (clksel=0) or the syncbus clock line (clksel=1, provided that
    the PLL is locked).

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the open function block descriptor.

@param
    clksel

    Preallocated boolean variable for clk select output.
    	0 (or false):	onboard oscillator
    	1 (or true):	syncbus clock PLL

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35418LIB_API
int DM35418_Syncbus_Get_Clk_Sel(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					int *clksel);


/**
*******************************************************************************
@brief
    Get the syncbus clock PLL lock status

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the open function block descriptor.

@param
    pll_locked

    Preallocated boolean variable for the PLL lock status
    	0 (or false):	syncbus clock PLL is not locked
    	1 (or true):	syncbus clock PLL is locked

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35418LIB_API
int DM35418_Syncbus_Get_PLL_Locked(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					int *pll_locked);


/**
*******************************************************************************
@brief
    Set a syncbus line as a source (input) to one of the internal global
    clock bus lines. The sppecified sycnbus line must be set as input.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the open function block descriptor.

@param
    clock_select

    The identifier of the global clock bus line to be used as a target.

@param
    clock_source

    The identifier syncbus line to be used as a source.
    DM35418_SYNCBUS_CLK_EVENT_DISABLE means the specified global clock bus is
    not driven by the syncbus functional block.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.
 */
DM35418LIB_API
int DM35418_Syncbus_Clock_Source_Global_Set(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					int clock_select,
					int clock_source);


/**
*******************************************************************************
@brief
    Get the source syncbus line of the specified global clock bus line.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the open function block descriptor.

@param
    clock_select

    The identifier of the global clock bus line to be queried.
    Refer to the board manual for valid values

@param
    clock_source

    Preallocated integer variable, output the identifier of the syncbus line
    to be used as source. DM35418_SYNCBUS_CLK_EVENT_DISABLE means the 
    specified global clock bus line is not driven by the yncbus functional block.
    Refer to the board manual for valid values.

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.@n@n
 */
DM35418LIB_API
int DM35418_Syncbus_Clock_Source_Global_Get(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					int clock_select,
					int *clock_source);


/**
*******************************************************************************
@brief
    Set the source global clock bus line used to drive the specified syncbus line.
    The specified syncbus line must be set as output.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the open function block descriptor.

@param
    syncbus_select

    The identifier of the syncbus line to be used as target.


@param
    clock_source

    The identifier global clock bus line to be used as a source.
    Refer to the board manual for valid values.    

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.@n@n
 */
DM35418LIB_API
int DM35418_Syncbus_Drive_Source_Set(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					int syncbus_select,
					int clock_source);


/**
*******************************************************************************
@brief
    Get the source global clock bus line used to drive the specified syncbus line.
    The specified syncbus line must be set as output.

@param
    handle

    Address of the handle pointer, which will contain the device
    descriptor.

@param
    func_block

    Pointer to the open function block descriptor.

@param
    syncbus_select

    The identifier of the syncbus line to be queried.

@param
    clock_source

    Preallocated integer variable, output the identifier of the
    global clock bus line to be used as source.
    Refer to the board manual for valid values.    

@retval
    0

    Success.

@retval
    Non-Zero

    Failure.@n@n
 */
DM35418LIB_API
int DM35418_Syncbus_Drive_Source_Get(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					int syncbus_select,
					int *clock_source);


/**
 * @} DM35418_Syncbus_Library_Functions
 */

#ifdef __cplusplus
}
#endif

#endif

