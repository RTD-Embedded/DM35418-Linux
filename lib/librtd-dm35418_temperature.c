/**
	@file

	@brief
		DM35418 Temperature library source code

	$Id: librtd-dm35418_temperature.c 68954 2013-04-11 22:12:35Z rgroner $
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
#include "dm35418_board_access_structs.h"
#include "dm35418_board_access.h"

/******************************************************************************
 * Temperature Sensor Library Functions
 *****************************************************************************/
DM35418LIB_API
int DM35418_Temperature_Open(struct DM35418_Board_Descriptor *handle,
				unsigned int ordinal_to_open,
				struct DM35418_Function_Block *fb_temp)
{

	return DM35418_Function_Block_Open_Module(handle,
							DM35418_FUNC_BLOCK_TEMPERATURE_SENSOR,
							ordinal_to_open,
							fb_temp);


}




DM35418LIB_API
int DM35418_Temperature_Read(struct DM35418_Board_Descriptor *handle,
				struct DM35418_Function_Block *fb_temp,
							float *temperature)
{
	int16_t sdata16;
	int return_code = 0;
	union dm35418_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = fb_temp->control_offset +
						DM35418_OFFSET_TEMPERATURE;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_16;

	return_code = DM35418_Read(handle, &ioctl_request);
	if (return_code != 0) {
		return return_code;
	}

	sdata16 = (int16_t)ioctl_request.readwrite.access.data.data16;

	*temperature = sdata16 * 0.0625/8;
	return 0;
}

