/**
    @file

    @brief
        Defines for the DM35418 (Device-specific values)

    @verbatim
    --------------------------------------------------------------------------
    This file and its contents are copyright (C) RTD Embedded Technologies,
    Inc.  All Rights Reserved.

    This software is licensed as described in the RTD End-User Software License
    Agreement.  For a copy of this agreement, refer to the file LICENSE.TXT
    (which should be included with this software) or contact RTD Embedded
    Technologies, Inc.
    --------------------------------------------------------------------------
    @endverbatim

    $Id: dm35418.h 96149 2016-01-20 21:01:00Z rgroner $
*/

#ifndef __DM35418_H__
#define __DM35418_H__

/**
 * @brief
 * DM35418 PCI vendor ID
 */

#define DM35418_PCI_VENDOR_ID	0x1435

/**
 * @brief
 *     DM35418 PCI device ID
 */
#define DM35418_PCI_DEVICE_ID	0x5418

/**
 * @brief
 *     Number of ADC on the DM35418
 */
#define DM35418_NUM_ADC_ON_BOARD 	8

/**
 * @brief
 *     Number of DAC on the DM35418
 */
#define DM35418_NUM_DAC_ON_BOARD 	8

/**
 * @brief
 *     Number of channels per ADC
 */
#define DM35418_NUM_ADC_DMA_CHANNELS	1

/**
 * @brief
 *     Number of buffers per ADC DMA channel
 */
#define DM35418_NUM_ADC_DMA_BUFFERS		7

/**
 * @brief
 *     Number of channels per DAC
 */
#define DM35418_NUM_DAC_DMA_CHANNELS	1

/**
 * @brief
 *     Number of buffers per DAC DMA channel
 */
#define DM35418_NUM_DAC_DMA_BUFFERS		7

/**
 * @brief
 *     Bit Mask for the gain bits of the FE Config
 */
#define DM35418_DAC_FE_CONFIG_GAIN_MASK		0x03

/**
 * @brief
 *     Sample size of the FIFO
 */
#define DM35418_FIFO_SAMPLE_SIZE		511

/**
 * @brief
 *     Max value of the DAC
 */
#define DM35418_DAC_MAX_VALUE	32767

/**
 * @brief
 *     Min value of the DAC
 */
#define DM35418_DAC_MIN_VALUE	-32768

#endif
