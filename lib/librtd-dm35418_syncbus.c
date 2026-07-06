/**
	@file

	@brief
		DM35418 Sync Bus library source code


	$Id: librtd-dm35418_syncbus.c 68954 2013-04-11 22:12:35Z rgroner $
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "dm35418_registers.h"
#include "dm35418_gbc_library.h"
#include "dm35418_syncbus_library.h"
#include "dm35418_board_access_structs.h"
#include "dm35418_board_access.h"

/******************************************************************************
 * SyncBus Library Functions
 *****************************************************************************/

DM35418LIB_API
int DM35418_Syncbus_Open(struct DM35418_Board_Descriptor *handle,
		unsigned int number,
		struct DM35418_Function_Block *func_block)
{

	return DM35418_Function_Block_Open_Module(handle,
						DM35418_FUNC_BLOCK_SYNCBUS,
						number,
						func_block);
}

DM35418LIB_API
int DM35418_Syncbus_Enable(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block)
{

	union dm35418_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset +
						DM35418_OFFSET_SYNCBUS_ENABLE;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = 1;

	return DM35418_Write(handle, &ioctl_request);

}

DM35418LIB_API
int DM35418_Syncbus_Disable(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block)
{

	union dm35418_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset +
						DM35418_OFFSET_SYNCBUS_ENABLE;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = 0;

	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Syncbus_Termination_Enable(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block)
{

	union dm35418_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset +
						DM35418_OFFSET_SYNCBUS_TERM_ENABLE;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = 1;

	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Syncbus_Termination_Disable(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block)
{

	union dm35418_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset +
						DM35418_OFFSET_SYNCBUS_TERM_ENABLE;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = 0;

	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Syncbus_Set_Direction(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block,
				int dir_0, int dir_1, int dir_2, int dir_3)
{

	union dm35418_ioctl_argument ioctl_request;
	uint8_t dir = 0;

	if (dir_0 == DM35418_SYNCBUS_DIR_OUTPUT) {
		dir |= (0x01 << 0);
	}
	if (dir_1 == DM35418_SYNCBUS_DIR_OUTPUT) {
		dir |= (0x01 << 1);
	}
	if (dir_2 == DM35418_SYNCBUS_DIR_OUTPUT) {
		dir |= (0x01 << 2);
	}
	if (dir_3 == DM35418_SYNCBUS_DIR_OUTPUT) {
		dir |= (0x01 << 3);
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset +
						DM35418_OFFSET_SYNCBUS_DIRECTION;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = dir;

	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Syncbus_Set_Clk_Sel(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block,
				int clksel)
{
	union dm35418_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset +
						DM35418_OFFSET_SYNCBUS_CLK_SEL;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = clksel;

	return DM35418_Write(handle, &ioctl_request);
}

DM35418LIB_API
int DM35418_Syncbus_Get_Clk_Sel(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					int *clksel)
{

	union dm35418_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
						DM35418_OFFSET_SYNCBUS_CLK_SEL;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;

	return_code = DM35418_Read(handle, &ioctl_request);

	*clksel = ioctl_request.readwrite.access.data.data8;

	return return_code;
}

DM35418LIB_API
int DM35418_Syncbus_Get_PLL_Locked(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					int *pll_locked)
{

	union dm35418_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
						DM35418_OFFSET_SYNCBUS_PLL_LOCKED;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;

	return_code = DM35418_Read(handle, &ioctl_request);

	*pll_locked = ioctl_request.readwrite.access.data.data8;

	return return_code;
}

DM35418LIB_API
int DM35418_Syncbus_Validate_Clock_Global_Source(int select,
					int source)
{


	switch (source) {
		/* breaks intentionally omitted */
	case DM35418_SYNCBUS_CLK_BUS_SRC_DISABLE:
	case DM35418_SYNCBUS_CLK_BUS_SRC_SYNCBUS_0:
	case DM35418_SYNCBUS_CLK_BUS_SRC_SYNCBUS_1:
	case DM35418_SYNCBUS_CLK_BUS_SRC_SYNCBUS_2:
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	switch (select) {
		/* breaks intentionally omitted */
	case DM35418_CLK_SRC_BUS2:
	case DM35418_CLK_SRC_BUS3:
	case DM35418_CLK_SRC_BUS4:
	case DM35418_CLK_SRC_BUS5:
	case DM35418_CLK_SRC_BUS6:
	case DM35418_CLK_SRC_BUS7:
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	return 0;

}

DM35418LIB_API
int DM35418_Syncbus_Clock_Source_Global_Set(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					int clock_select,
					int clock_source)
{

	int return_code = 0;
	union dm35418_ioctl_argument ioctl_request;
	unsigned int offset;

	return_code = DM35418_Syncbus_Validate_Clock_Global_Source(clock_select,
								clock_source);

	if (return_code != 0) {
		return return_code;
	}


	switch (clock_select) {
	case DM35418_CLK_SRC_BUS2:
		offset = DM35418_OFFSET_SYNCBUS_CLK_BUS2;
		break;
	case DM35418_CLK_SRC_BUS3:
		offset = DM35418_OFFSET_SYNCBUS_CLK_BUS3;
		break;
	case DM35418_CLK_SRC_BUS4:
		offset = DM35418_OFFSET_SYNCBUS_CLK_BUS4;
		break;
	case DM35418_CLK_SRC_BUS5:
		offset = DM35418_OFFSET_SYNCBUS_CLK_BUS5;
		break;
	case DM35418_CLK_SRC_BUS6:
		offset = DM35418_OFFSET_SYNCBUS_CLK_BUS6;
		break;
	case DM35418_CLK_SRC_BUS7:
		offset = DM35418_OFFSET_SYNCBUS_CLK_BUS7;
		break;
	default:
		return -1;
		break;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset +
						offset;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = (char) clock_source;

	return DM35418_Write(handle, &ioctl_request);
}

DM35418LIB_API
int DM35418_Syncbus_Clock_Source_Global_Get(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					int clock_select,
					int *clock_source)
{

	int return_code = 0;
	union dm35418_ioctl_argument ioctl_request;
	unsigned int offset;

	switch (clock_select) {
	case DM35418_CLK_SRC_BUS2:
		offset = DM35418_OFFSET_SYNCBUS_CLK_BUS2;
		break;
	case DM35418_CLK_SRC_BUS3:
		offset = DM35418_OFFSET_SYNCBUS_CLK_BUS3;
		break;
	case DM35418_CLK_SRC_BUS4:
		offset = DM35418_OFFSET_SYNCBUS_CLK_BUS4;
		break;
	case DM35418_CLK_SRC_BUS5:
		offset = DM35418_OFFSET_SYNCBUS_CLK_BUS5;
		break;
	case DM35418_CLK_SRC_BUS6:
		offset = DM35418_OFFSET_SYNCBUS_CLK_BUS6;
		break;
	case DM35418_CLK_SRC_BUS7:
		offset = DM35418_OFFSET_SYNCBUS_CLK_BUS7;
		break;
	default:
		return -1;
		break;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset +
						offset;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;

	return_code = DM35418_Read(handle, &ioctl_request);

	*clock_source = (int) ioctl_request.readwrite.access.data.data8;

	return return_code;

}

DM35418LIB_API
int DM35418_Syncbus_Validate_Drive_Source(int select,
					int source)
{


	switch (source) {
		/* breaks intentionally omitted */
	case DM35418_CLK_SRC_IMMEDIATE:
	case DM35418_CLK_SRC_NEVER:
	case DM35418_CLK_SRC_BUS2:
	case DM35418_CLK_SRC_BUS3:
	case DM35418_CLK_SRC_BUS4:
	case DM35418_CLK_SRC_BUS5:
	case DM35418_CLK_SRC_BUS6:
	case DM35418_CLK_SRC_BUS7:
	case DM35418_CLK_SRC_BUS2_INV:
	case DM35418_CLK_SRC_BUS3_INV:
	case DM35418_CLK_SRC_BUS4_INV:
	case DM35418_CLK_SRC_BUS5_INV:
	case DM35418_CLK_SRC_BUS6_INV:
	case DM35418_CLK_SRC_BUS7_INV:
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	switch (select) {
		/* breaks intentionally omitted */
	case DM35418_SYNCBUS_0_CLK_SRC:
	case DM35418_SYNCBUS_1_CLK_SRC:
	case DM35418_SYNCBUS_2_CLK_SRC:
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	return 0;

}

DM35418LIB_API
int DM35418_Syncbus_Drive_Source_Set(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					int syncbus_select,
					int clock_source)
{

	int return_code = 0;
	union dm35418_ioctl_argument ioctl_request;
	unsigned int offset;

	return_code = DM35418_Syncbus_Validate_Drive_Source(syncbus_select,
								clock_source);

	if (return_code != 0) {
		return return_code;
	}


	switch (syncbus_select) {
	case DM35418_SYNCBUS_0_CLK_SRC:
		offset = DM35418_OFFSET_SYNCBUS_0_SRC;
		break;
	case DM35418_SYNCBUS_1_CLK_SRC:
		offset = DM35418_OFFSET_SYNCBUS_1_SRC;
		break;
	case DM35418_SYNCBUS_2_CLK_SRC:
		offset = DM35418_OFFSET_SYNCBUS_2_SRC;
		break;
	default:
		return -1;
		break;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset +
						offset;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = (char) clock_source;

	return DM35418_Write(handle, &ioctl_request);
}

DM35418LIB_API
int DM35418_Syncbus_Drive_Source_Get(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					int syncbus_select,
					int *clock_source)
{

	int return_code = 0;
	union dm35418_ioctl_argument ioctl_request;
	unsigned int offset;

	switch (syncbus_select) {
	case DM35418_SYNCBUS_0_CLK_SRC:
		offset = DM35418_OFFSET_SYNCBUS_0_SRC;
		break;
	case DM35418_SYNCBUS_1_CLK_SRC:
		offset = DM35418_OFFSET_SYNCBUS_1_SRC;
		break;
	case DM35418_SYNCBUS_2_CLK_SRC:
		offset = DM35418_OFFSET_SYNCBUS_2_SRC;
		break;
	default:
		return -1;
		break;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset +
						offset;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;

	return_code = DM35418_Read(handle, &ioctl_request);

	*clock_source = (int) ioctl_request.readwrite.access.data.data8;

	return return_code;

}



