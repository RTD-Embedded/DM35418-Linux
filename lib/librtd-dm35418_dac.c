/**
	@file

	@brief
		DM35418 DAC library source code


	$Id: librtd-dm35418_dac.c 103507 2016-10-12 20:58:56Z rgroner $
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
#include "dm35418.h"
#include "dm35418_util_library.h"
#include "dm35418_dac_library.h"
#include "dm35418_types.h"
#include "dm35418_board_access_structs.h"
#include "dm35418_board_access.h"

/******************************************************************************
 * DAC Library Functions
 *****************************************************************************/
/**
 * @defgroup DM35418_DAC_Library_Source DM35418 DAC library source code
 * @{
 */

/*=============================================================================
Private functions
 =============================================================================*/

/**
 * @defgroup DM35418_DAC_Library_Private_Functions DM35418 DAC library private functions source code
 * @{
 */

 /**
  * @internal
  */


/**
*******************************************************************************
@brief
    Validate the passed in trigger value

@param
    trigger

    The clock trigger value to validate

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL	Invalid trigger value

 */
int
DM35418_Dac_Validate_Trigger(uint8_t trigger)
{

	switch (trigger) {
		/* breaks intentionally omitted */
	case DM35418_CLK_SRC_IMMEDIATE:
	case DM35418_CLK_SRC_NEVER:
	case DM35418_CLK_SRC_BUS2:
	case DM35418_CLK_SRC_BUS3:
	case DM35418_CLK_SRC_BUS4:
	case DM35418_CLK_SRC_BUS5:
	case DM35418_CLK_SRC_BUS6:
	case DM35418_CLK_SRC_BUS7:
	case DM35418_CLK_SRC_CHAN_THRESH:
	case DM35418_CLK_SRC_CHAN_THRESH_INV:
	case DM35418_CLK_SRC_BUS2_INV:
	case DM35418_CLK_SRC_BUS3_INV:
	case DM35418_CLK_SRC_BUS4_INV:
	case DM35418_CLK_SRC_BUS5_INV:
	case DM35418_CLK_SRC_BUS6_INV:
	case DM35418_CLK_SRC_BUS7_INV:
		return 0;
	default:
		errno = EINVAL;
		return -1;
	}

}


/**
*******************************************************************************
@brief
    Validate the passed in clock select and source

@param
    clock_select

    The clock select to validate.

@param
    clock_driver

    The clock driver to validate.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL	Invalid clock select or source

 */
int
DM35418_Dac_Validate_Clock_Global_Src_Args(enum DM35418_Clock_Sources clock_select,
						enum DM35418_Dac_Clock_Events clock_driver)
{

	switch (clock_select) {
	case DM35418_CLK_SRC_BUS2:
		/* breaks intentionally omitted */
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

	switch (clock_driver) {
	case DM35418_DAC_CLK_BUS_SRC_DISABLE:
		/* breaks intentionally omitted */
	case DM35418_DAC_CLK_BUS_SRC_CONVERSION_SENT:
	case DM35418_DAC_CLK_BUS_SRC_CHAN_MARKER:
	case DM35418_DAC_CLK_BUS_SRC_START_TRIG:
	case DM35418_DAC_CLK_BUS_SRC_STOP_TRIG:
	case DM35418_DAC_CLK_BUS_SRC_CONV_COMPL:
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	return 0;
}


/**
*******************************************************************************
@brief
    Validate the output range

@param
    range

    The desired output range of the DAC.

@retval
    0

    Success.

@retval
    Non-zero

    Failure.@n@n
    errno may be set as follows:
        @arg \c
            EINVAL	Invalid range selected

 */
int
DM35418_Dac_Validate_Output_Range(enum DM35418_Output_Ranges range)
{


	switch (range) {
	/* Breaks intentionally omitted */
	case DM35418_DAC_RNG_BIPOLAR_2_5V:
	case DM35418_DAC_RNG_BIPOLAR_5V:
	case DM35418_DAC_RNG_BIPOLAR_10V:
		break;


	default:
		errno = EINVAL;
		return -1;
		break;
	}

	return 0;
}



/**
 * @} DM35418_DAC_Library_Private_Functions
 */

/*=============================================================================
Public functions
 =============================================================================*/

/**
 * @defgroup DM35418_DAC_Library_Public_Functions DM35418 DAC source
 *		code for public library functions
 * @{
 */



DM35418LIB_API
int DM35418_Dac_Open(struct DM35418_Board_Descriptor *handle,
			unsigned int number_of_type,
			struct DM35418_Function_Block *func_block)
{

	return DM35418_Function_Block_Open_Module(handle,
						DM35418_FUNC_BLOCK_DAC,
						number_of_type,
						func_block);

}




DM35418LIB_API
int DM35418_Dac_Set_Clock_Src(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block,
				enum DM35418_Clock_Sources source)
{

	union dm35418_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_CLK_SRC;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = (uint8_t) source;

	return DM35418_Write(handle, &ioctl_request);


}


DM35418LIB_API
int DM35418_Dac_Get_Clock_Src(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block,
				enum DM35418_Clock_Sources *source)
{

	union dm35418_ioctl_argument ioctl_request;
	int return_value;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_CLK_SRC;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;

	return_value = DM35418_Read(handle, &ioctl_request);

	*source = (enum DM35418_Clock_Sources) ioctl_request.readwrite.access.data.data8;

	return return_value;

}


DM35418LIB_API
int DM35418_Dac_Get_Clock_Div(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					uint32_t *divider)
{

	union dm35418_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_CLK_DIV;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;

	return_code = DM35418_Read(handle, &ioctl_request);

	*divider = ioctl_request.readwrite.access.data.data32;

	return return_code;
}


DM35418LIB_API
int DM35418_Dac_Set_Clock_Div(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					uint32_t divider)
{

	union dm35418_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_CLK_DIV;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = divider;

	return DM35418_Write(handle, &ioctl_request);
}



DM35418LIB_API
int DM35418_Dac_Set_Conversion_Rate(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					uint32_t requested_rate,
					uint32_t *actual_rate)
{


	union dm35418_ioctl_argument ioctl_request;
	int return_code;
	uint32_t system_clock_rate;
	uint32_t divider;
	int is_std_clk = 1;

	if ((requested_rate == 0) || (requested_rate > DM35418_DAC_MAX_RATE)) {
		errno = EINVAL;
		return -1;
	}

	return_code = DM35418_Gbc_Get_Sys_Clock_Freq(handle,
							&system_clock_rate,
							&is_std_clk);

	if (return_code != 0) {
		return return_code;
	}

	if (!is_std_clk) {
		errno = ENODEV;
		return -1;
	}

	divider = system_clock_rate / requested_rate;

	*actual_rate = system_clock_rate / divider;

	/* The actual formula calls for this minus 1, but we won't do it if
	   divisor would end up less than 1, which is its minimum value.
	*/
	if (divider < 2) {
		divider = 1;
	}
	else {
		divider --;
	}

	if (divider > system_clock_rate) {
		divider = system_clock_rate;
	}


	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_CLK_DIV;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = divider;
	return DM35418_Write(handle, &ioctl_request);

}

DM35418LIB_API
int DM35418_Dac_Interrupt_Set_Config(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					uint16_t interrupt_src,
					int enable)
{
	union dm35418_ioctl_argument ioctl_request;
	int return_code;
	uint32_t value = 0;

	if (enable) {
		value = (uint32_t) 0xFFFF0000;

		// clear the interrupt status before enabling so we don't get an interrupt
		// from previous data
		return_code = DM35418_Dac_Interrupt_Clear_Status(handle, func_block,
		    							interrupt_src);
		if (return_code != 0) {
			return return_code;
		}
	}

	value |= (uint32_t) interrupt_src;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_INT_ENABLE;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = value;

	return DM35418_Write(handle, &ioctl_request);


}

DM35418LIB_API
int DM35418_Dac_Interrupt_Get_Config(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					uint16_t *interrupt_ena)
{
	union dm35418_ioctl_argument ioctl_request;
	int result = 0;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_INT_ENABLE;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;

	result = DM35418_Read(handle, &ioctl_request);

	*interrupt_ena = (ioctl_request.readwrite.access.data.data32 >> 16);

	return result;

}


DM35418LIB_API
int DM35418_Dac_Set_Start_Trigger(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					uint8_t trigger_value)
{

	union dm35418_ioctl_argument ioctl_request;

	if (DM35418_Dac_Validate_Trigger(trigger_value)) {
		return -1;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_START_TRIG;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = trigger_value;

	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Dac_Set_Stop_Trigger(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					uint8_t trigger_value)
{

	union dm35418_ioctl_argument ioctl_request;

	if (DM35418_Dac_Validate_Trigger(trigger_value)) {
		return -1;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_STOP_TRIG;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = trigger_value;

	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Dac_Get_Start_Trigger(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					uint8_t *trigger_value)
{

	union dm35418_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_START_TRIG;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;

	return_code = DM35418_Read(handle, &ioctl_request);

	*trigger_value = ioctl_request.readwrite.access.data.data8;

	return return_code;

}


DM35418LIB_API
int DM35418_Dac_Get_Stop_Trigger(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					uint8_t *trigger_value)
{

	union dm35418_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_STOP_TRIG;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;

	return_code = DM35418_Read(handle, &ioctl_request);

	*trigger_value = ioctl_request.readwrite.access.data.data8;

	return return_code;
}


DM35418LIB_API
int DM35418_Dac_Start(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block)
{

	union dm35418_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = DM35418_DAC_MODE_GO_SINGLE_SHOT;

	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Dac_Reset(struct DM35418_Board_Descriptor *handle,
		const struct DM35418_Function_Block *func_block)
{

	union dm35418_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = DM35418_DAC_MODE_RESET;

	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Dac_Pause(struct DM35418_Board_Descriptor *handle,
		const struct DM35418_Function_Block *func_block)
{

	union dm35418_ioctl_argument ioctl_request;


	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = DM35418_DAC_MODE_PAUSE;

	return DM35418_Write(handle, &ioctl_request);

}



DM35418LIB_API
int DM35418_Dac_Get_Mode_Status(struct DM35418_Board_Descriptor *handle,
				const struct DM35418_Function_Block *func_block,
				uint8_t *mode_status)
{

	union dm35418_ioctl_argument ioctl_request;
	int return_val;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_MODE_STATUS;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;

	return_val = DM35418_Read(handle, &ioctl_request);

	*mode_status = ioctl_request.readwrite.access.data.data8;

	return return_val;
}


DM35418LIB_API
int DM35418_Dac_Get_Last_Conversion(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					unsigned int channel,
					uint8_t *marker,
					int16_t *value)
{

	union dm35418_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35418_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35418_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35418_OFFSET_DAC_CHAN_LAST_CONVERSION;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;


	return_code = DM35418_Read(handle, &ioctl_request);

	*value = (int16_t) (ioctl_request.readwrite.access.data.data32 & 0x0000FFFF);
	*marker = (uint8_t) (ioctl_request.readwrite.access.data.data32 >> 24);

	return return_code;

}


DM35418LIB_API
int DM35418_Dac_Set_Last_Conversion(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					unsigned int channel,
					uint8_t marker,
					int16_t value)
{

	union dm35418_ioctl_argument ioctl_request;
	uint32_t value_to_write;

	value_to_write = (uint32_t) marker;

	value_to_write <<= 24;
	value_to_write |= (uint32_t) value;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35418_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35418_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35418_OFFSET_DAC_CHAN_LAST_CONVERSION;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = value_to_write;

	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Dac_Get_Conversion_Count(struct DM35418_Board_Descriptor *handle,
						const struct DM35418_Function_Block *func_block,
						uint32_t *value)
{

	union dm35418_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_CONV_COUNT;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;


	return_code = DM35418_Read(handle, &ioctl_request);

	*value = ioctl_request.readwrite.access.data.data32;

	return return_code;

}


DM35418LIB_API
int DM35418_Dac_Interrupt_Get_Status(struct DM35418_Board_Descriptor *handle,
						const struct DM35418_Function_Block *func_block,
						uint16_t *value)
{

	union dm35418_ioctl_argument ioctl_request;
	int return_code;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_INT_STAT;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_16;



	return_code = DM35418_Read(handle, &ioctl_request);

	*value = ioctl_request.readwrite.access.data.data16;

	return return_code;

}


DM35418LIB_API
int DM35418_Dac_Interrupt_Clear_Status(struct DM35418_Board_Descriptor *handle,
						const struct DM35418_Function_Block *func_block,
						uint16_t value)
{

	union dm35418_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_INT_STAT;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_16;
	ioctl_request.readwrite.access.data.data16 = value;


	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Dac_Set_Post_Stop_Conversion_Count(struct DM35418_Board_Descriptor *handle,
						const struct DM35418_Function_Block *func_block,
						uint32_t value)
{

	union dm35418_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_POST_STOP_CONV;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = value;


	return DM35418_Write(handle, &ioctl_request);

}

DM35418LIB_API
int DM35418_Dac_Get_Post_Stop_Conversion_Count(struct DM35418_Board_Descriptor *handle,
						const struct DM35418_Function_Block *func_block,
						uint32_t *value)
{

	union dm35418_ioctl_argument ioctl_request;
	int return_value;

	ioctl_request.readwrite.access.offset = func_block->control_offset + DM35418_OFFSET_DAC_POST_STOP_CONV;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;

	return_value = DM35418_Read(handle, &ioctl_request);

	*value = ioctl_request.readwrite.access.data.data32;

	return return_value;
}




DM35418LIB_API
int DM35418_Dac_Set_Clock_Source_Global(struct DM35418_Board_Descriptor *handle,
					const struct DM35418_Function_Block *func_block,
					enum DM35418_Clock_Sources clock,
					enum DM35418_Dac_Clock_Events clock_driver)
{

	union dm35418_ioctl_argument ioctl_request;
	int result = 0;
	unsigned int offset = 0;

	result = DM35418_Dac_Validate_Clock_Global_Src_Args(clock,
								clock_driver);

	if (result) {
		return result;
	}

	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;
	ioctl_request.readwrite.access.data.data8 = (uint8_t) clock_driver;

	switch (clock) {
	case DM35418_CLK_SRC_BUS2:
		offset = DM35418_OFFSET_DAC_CLK_BUS2;
		break;
	case DM35418_CLK_SRC_BUS3:
		offset = DM35418_OFFSET_DAC_CLK_BUS3;
		break;
	case DM35418_CLK_SRC_BUS4:
		offset = DM35418_OFFSET_DAC_CLK_BUS4;
		break;
	case DM35418_CLK_SRC_BUS5:
		offset = DM35418_OFFSET_DAC_CLK_BUS5;
		break;
	case DM35418_CLK_SRC_BUS6:
		offset = DM35418_OFFSET_DAC_CLK_BUS6;
		break;
	case DM35418_CLK_SRC_BUS7:
		offset = DM35418_OFFSET_DAC_CLK_BUS7;
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset + offset;


	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Dac_Channel_Setup(struct DM35418_Board_Descriptor *handle,
					struct DM35418_Function_Block *func_block,
					unsigned int channel,
					enum DM35418_Output_Ranges output_range)
{

	union dm35418_ioctl_argument ioctl_request;
	uint32_t fe_config;
	uint16_t fe_data;

	if (DM35418_Dac_Validate_Output_Range(output_range) ||
		channel >= DM35418_NUM_DAC_DMA_CHANNELS) {
		errno = EINVAL;
		return -1;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35418_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35418_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35418_OFFSET_DAC_CHAN_FRONT_END_CONFIG;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;

	switch (output_range) {
	case DM35418_DAC_RNG_BIPOLAR_2_5V:
		fe_data = DM35418_DAC_FE_CONFIG_GAIN_1;
		break;
	case DM35418_DAC_RNG_BIPOLAR_5V:
		fe_data = DM35418_DAC_FE_CONFIG_GAIN_2;
		break;
	default:
		fe_data = DM35418_DAC_FE_CONFIG_GAIN_4;
		break;
	}


	fe_config = DM35418_Get_Maskable(fe_data,
					     DM35418_DAC_FE_CONFIG_GAIN_MASK);

	ioctl_request.readwrite.access.data.data32 = fe_config;

	return DM35418_Write(handle, &ioctl_request);

}




DM35418LIB_API
int DM35418_Dac_Channel_Reset(struct DM35418_Board_Descriptor *handle,
				struct DM35418_Function_Block *func_block,
				unsigned int channel)
{

	union dm35418_ioctl_argument ioctl_request;

	if (channel >= DM35418_NUM_DAC_DMA_CHANNELS) {
		errno = EINVAL;
		return -1;
	}

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35418_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35418_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35418_OFFSET_DAC_CHAN_FRONT_END_CONFIG;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = DM35418_Get_Maskable(0,
										0xFFFF);

	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Dac_Channel_Set_Marker_Config(struct DM35418_Board_Descriptor *handle,
					struct DM35418_Function_Block *func_block,
					unsigned int channel,
					uint8_t marker_enable)
{

	union dm35418_ioctl_argument ioctl_request;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35418_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35418_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35418_OFFSET_DAC_CHAN_MARKER_ENABLE;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;

	ioctl_request.readwrite.access.data.data8 = marker_enable;

	return DM35418_Write(handle, &ioctl_request);

}



DM35418LIB_API
int DM35418_Dac_Channel_Get_Marker_Config(struct DM35418_Board_Descriptor *handle,
					struct DM35418_Function_Block *func_block,
					unsigned int channel,
					uint8_t *marker_enable)
{

	union dm35418_ioctl_argument ioctl_request;
	int return_value;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35418_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35418_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35418_OFFSET_DAC_CHAN_MARKER_ENABLE;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;

	return_value = DM35418_Read(handle, &ioctl_request);

	*marker_enable = ioctl_request.readwrite.access.data.data8;

	return return_value;

}




DM35418LIB_API
int DM35418_Dac_Channel_Get_Marker_Status(struct DM35418_Board_Descriptor *handle,
					struct DM35418_Function_Block *func_block,
					unsigned int channel,
					uint8_t *marker_status)
{

	union dm35418_ioctl_argument ioctl_request;
	int status = 0;

	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35418_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35418_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35418_OFFSET_DAC_CHAN_MARKER_STATUS;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;


	status = DM35418_Read(handle, &ioctl_request);

	*marker_status = ioctl_request.readwrite.access.data.data8;

	return status;

}




DM35418LIB_API
int DM35418_Dac_Channel_Clear_Marker_Status(struct DM35418_Board_Descriptor *handle,
					struct DM35418_Function_Block *func_block,
					unsigned int channel,
					uint8_t marker_to_clear)
{

	union dm35418_ioctl_argument ioctl_request;
	ioctl_request.readwrite.access.offset = func_block->control_offset +
		DM35418_OFFSET_DAC_CHAN_CTRL_BLK_START +
		(channel * DM35418_DAC_CHAN_CTRL_BLK_SIZE) +
		DM35418_OFFSET_DAC_CHAN_MARKER_STATUS;
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_8;

	ioctl_request.readwrite.access.data.data8 = marker_to_clear;

	return DM35418_Write(handle, &ioctl_request);

}


DM35418LIB_API
int DM35418_Dac_Fifo_Channel_Write(struct DM35418_Board_Descriptor *handle,
                            		const struct DM35418_Function_Block *func_block,
                            		unsigned int channel,
                            		int32_t value)
{
	union dm35418_ioctl_argument ioctl_request;
	int return_code = 0;

	if (channel >= func_block->num_dma_channels) {
		errno = EINVAL;
		return -1;
	}

	// Check that the function block revision ID is valid for direct
	// FIFO access.
	if(func_block->type_revision < DM35418_FIFO_ACCESS_FB_REVISION)
	{
	    errno = EPERM;
		return -1;
	}

	// Set up the IOCTL struct
	ioctl_request.readwrite.access.offset = func_block->fb_offset +
					    DM35418_OFFSET_DAC_FIFO_CTRL_BLK_START +
					    (channel * DM35418_OFFSET_DAC_FIFO_CTRL_BLK_SIZE);
	ioctl_request.readwrite.access.region = DM35418_PCI_REGION_FB;
	ioctl_request.readwrite.access.size = DM35418_PCI_REGION_ACCESS_32;
	ioctl_request.readwrite.access.data.data32 = value;

	// Perform the write
	return_code = DM35418_Write(handle, &ioctl_request);

	return return_code;
}


DM35418LIB_API
int DM35418_Dac_Volts_To_Conv(
				enum DM35418_Output_Ranges output_range,
				float volts,
				int16_t *dac_conversion)
{


	int32_t temp_value;
	float lsb;

	switch (output_range) {
	case DM35418_DAC_RNG_BIPOLAR_2_5V:
		lsb = DM35418_DAC_RNG_5_LSB;
		break;
	case DM35418_DAC_RNG_BIPOLAR_5V:
		lsb = DM35418_DAC_RNG_10_LSB;
		break;
	case DM35418_DAC_RNG_BIPOLAR_10V:
		lsb = DM35418_DAC_RNG_20_LSB;
		break;
	default:
		errno = EINVAL;
		return -1;
		break;
	}

	temp_value = (int32_t) (volts / lsb);

	if (temp_value > DM35418_DAC_BIPOLAR_MAX ||
	    temp_value < DM35418_DAC_BIPOLAR_MIN) {
		errno = ERANGE;
		return -1;
	}

    *dac_conversion = (int16_t) temp_value;

	return 0;
}


DM35418LIB_API
int DM35418_Dac_Conv_To_Volts(
				enum DM35418_Output_Ranges output_range,
				int16_t conversion,
				float *volts)
{

	float lsb;

	/* We would normally verify that the conversion value
	 * was within a valid range, but since the DAC is only
	 * 16-bits wide, all values are in range automatically
	 */
	switch (output_range) {
	case DM35418_DAC_RNG_BIPOLAR_2_5V:
		lsb = DM35418_DAC_RNG_5_LSB;
		break;
	case DM35418_DAC_RNG_BIPOLAR_5V:
		lsb = DM35418_DAC_RNG_10_LSB;
		break;
	case DM35418_DAC_RNG_BIPOLAR_10V:
		lsb = DM35418_DAC_RNG_20_LSB;
		break;
	default:
		errno = EINVAL;
		return 1;
		break;
	}
	*volts = lsb * ((float) conversion);


	return 0;
}

/**
 * @} DM35418_DAC_Library_Public_Functions
 */

/**
* @} DM35418_DAC_Library_Source
*/


