/**
    @file

    @brief
		Example program which demonstrates the use of the ADC and
		DMA.

    @verbatim

		This example program will collect data from the ADC(s)
		specified by the user, at the rate specified by the user, and will
		write the data to a file.  It will do this continuously until the
		user hits CTRL-C (or the filesystem becomes full).

		You can put any signal you want on the ADC input pins.
		However, for convenience, this example sets up the DACs to provide
		a signal for the ADC to measure.  In order for that to work, you
		must loopback the DAC outputs to the ADC inputs (DAC0.0 to ADC0.0,
		DAC1.0 to ADC1.0, etc).  The DAC signal is a 1 Hz sine wave.

		Maximum sustainable throughput is HIGHLY system dependent. Higher
		sample rates might be achievable through better buffer size
		selection or use of an operating system with realtime features.

    @endverbatim

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

    $Id: dm35418_adc_continuous_dma.c 141531 2024-03-06 21:05:25Z lfrankenfield $
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <getopt.h>

#include "dm35418_gbc_library.h"
#include "dm35418_adc_library.h"
#include "dm35418_dac_library.h"
#include "dm35418_ioctl.h"
#include "dm35418_examples.h"
#include "dm35418_dma_library.h"
#include "dm35418.h"
#include "dm35418_util_library.h"

/**
 * Default rate to use, if user does not enter one. (Hz)
 */
#define DEFAULT_RATE		10000

/**
 * Default number of ADC to use, if user does not provide a num
 */
#define DEFAULT_NUM_ADC		4

/**
 * Number of samples to play out DAC pins
 */
#define BUFFER_SIZE_SAMPLES	1000000

/**
 * Number of bytes in DAC sample buffer
 */
#define BUFFER_SIZE_BYTES	(BUFFER_SIZE_SAMPLES * sizeof(int))

/**
 * Rate for the DAC
 */
#define DAC_RATE	(BUFFER_SIZE_SAMPLES)

/**
 * Name of file when saving as ASCII
 */
#define ASCII_FILE_NAME "./adc_dma.txt"

/**
 * Name of file when saving as binary
 */
#define BIN_FILE_NAME "./adc_dma.bin"

/**
 * Name of the program as invoked on the command line
 */

static char *program_name;

/**
 * Boolean flag indicating if there was a DMA error.
 */
static int dma_has_error = 0;

/**
 * Pointer to board descriptor
 */
static struct DM35418_Board_Descriptor *board;

/**
 * Pointer to array of function blocks that will hold the ADC descriptors
 */
static struct DM35418_Function_Block my_adc[DM35418_NUM_ADC_ON_BOARD];

/**
 * Array of buffer counts, used to track progress of each ADC
 * as data is copied.
 */
static unsigned long buffer_count[DM35418_NUM_ADC_ON_BOARD];

/**
 * Pointer to local memory buffer where data is copied from the kernel buffers
 * when a DMA buffer becomes full.
 */
static int **local_buffer[DM35418_NUM_ADC_ON_BOARD];

/**
 * Boolean indicating the program should exit.
 */
static volatile int exit_program = 0;

/**
 * Size of the buffer allocated, in bytes.
 */
static unsigned long buffer_size_bytes = 0;

/**
 * Which buffer is next to be copied from DMA
 */
static unsigned int next_buffer[DM35418_NUM_ADC_ON_BOARD];

/**
*******************************************************************************
@brief
    Print information on stderr about how the program is to be used.  After
    doing so, the program is exited.
 *******************************************************************************
*/

static void usage(void)
{

	fprintf(stderr, "\n");
	fprintf(stderr, "NAME\n\n\t%s\n\n", program_name);
	fprintf(stderr, "USAGE\n\n\t%s [OPTIONS]\n\n", program_name);

	fprintf(stderr, "OPTIONS\n\n");
	fprintf(stderr, "\t--help\n");
	fprintf(stderr, "\t\tShow this help screen and exit.\n");

	fprintf(stderr, "\t--minor NUM\n");
	fprintf(stderr, "\t\tSpecify the minor number (>= 0) of the board to open.  When not specified,\n");
	fprintf(stderr, "\t\tthe device file with minor 0 is opened.\n");

	fprintf(stderr, "\t--rate RATE\n");
	fprintf(stderr, "\t\tSample at the specified rate (Hz).  The default is %d.\n", DEFAULT_RATE);

	fprintf(stderr, "\t--num_adc NUM\n");
	fprintf(stderr, "\t\tThe number of ADC to collect from.  The default is 1.\n");

	fprintf(stderr, "\t--samples NUM\n");
	fprintf(stderr, "\t\tThe number of samples to collect before stopping.  Note that the\n");
	fprintf(stderr, "\t\tactual number collected may be higher due to buffer size.\n");

	fprintf(stderr, "\t--binary\n");
	fprintf(stderr, "\t\tWrite data to file in binary format, instead of default ASCII.\n");
	fprintf(stderr, "\t\tData is stored as [ADC0 Buff0][ADC1 Buff0]....[ADC0 Buff1][ADC1 Buff1],etc\n");

	fprintf(stderr, "\t--bin2txt\n");
	fprintf(stderr, "\t\tThe program will convert the %s file to\n", BIN_FILE_NAME);
	fprintf(stderr, "\t\t%s and exit.\n\n", ASCII_FILE_NAME);
	fprintf(stderr, "\t\tNote: Because the rate affects the buffer size, and the\n");
	fprintf(stderr, "\t\tnumber of ADC affects data layout, you must include the\n");
	fprintf(stderr, "\t\t--rate and --num_adc arguments as well, IF they were used\n");
	fprintf(stderr, "\t\tto create the binary file in the first place.\n");
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

/**
*******************************************************************************
@brief
    Signal handler for SIGINT Control-C keyboard interrupt.

@param
    signal_number

    Signal number passed in from the kernel.

@warning
    One must be extremely careful about what functions are called from a signal
    handler.
 *******************************************************************************
*/

static void sigint_handler(int signal_number)
{
	exit_program = 0xff;
}

/**
*******************************************************************************
@brief
    Output the status of a DMA channel.  This is a helper function to determine
    the cause of an error when it occurs.

@param
    handle

    Pointer to the board handle.

@param
    func_block

    Pointer to the function block containing the DMA channel

@param
    channel

    The DMA channel we want the status of.

 @retval
    None
 *******************************************************************************
*/

void output_channel_status(struct DM35418_Board_Descriptor *handle,
			   const struct DM35418_Function_Block *func_block,
			   unsigned int channel)
{
	int result;
	unsigned int current_buffer;
	uint32_t current_count;
	int current_action;
	int status_overflow;
	int status_underflow;
	int status_used;
	int status_invalid;
	int status_complete;

	result = DM35418_Dma_Status(handle,
				    func_block,
				    channel,
				    &current_buffer,
				    &current_count,
				    &current_action,
				    &status_overflow,
				    &status_underflow,
				    &status_used,
				    &status_invalid, &status_complete);

	DM35418_Check_Result(result, "Error getting DMA status");

	printf
	    ("FB%d Ch%d DMA Status: Current Buffer: %u  Count: %u  Action: 0x%x  Status: "
	     "Ov: %d  Un: %d  Used: %d  Inv: %d  Comp: %d\n",
	     func_block->fb_num, channel, current_buffer, current_count,
	     current_action, status_overflow, status_underflow, status_used,
	     status_invalid, status_complete);
}

/**
*******************************************************************************
@brief
    The interrupt subroutine that will execute when a DMA interrupt occurs.
    This function will read from the DMA, copying data from the kernel buffers
    to the user buffers so that we can access the data.

@param
    int_info

    A structure containing information about the interrupt.

 @retval
    None.
 *******************************************************************************
*/
void ISR(struct dm35418_ioctl_interrupt_info_request int_info)
{

	int result = 0;
	int adc_num = -1;
	int adc_index = 0;
	uint32_t function_block;
	int buffer_full = 0;
	int dma_error = 0;
	char message[200];

	function_block = int_info.interrupt_fb & 0x7FFFFFFF;

	if (int_info.valid_interrupt) {

		while (adc_num < 0 && adc_index < DM35418_NUM_ADC_ON_BOARD) {
			if (my_adc[adc_index].fb_num == function_block) {
				adc_num = adc_index;
			}

			adc_index++;
		}

		sprintf(message,
			"ISR: Could not match function block number (%d) to an ADC.",
			function_block);

		DM35418_Check_Result(adc_num < 0, message);

		// It's a DMA interrupt
		if (int_info.interrupt_fb < 0) {

			result = DM35418_Dma_Check_For_Error(board,
							     &my_adc[adc_num],
							     CHANNEL_0,
							     &dma_error);

			DM35418_Check_Result(result, "Error checking for DMA error.");

			if (dma_error) {
				dma_has_error = 1;
				exit_program = 1;
				return;
			}
			result = DM35418_Dma_Check_Buffer_Used(board,
							       &my_adc[adc_num],
							       CHANNEL_0,
							       next_buffer
							       [adc_num],
							       &buffer_full);

			DM35418_Check_Result(result, "Error finding used buffer.");

			while (buffer_full) {
				result = DM35418_Dma_Read(board,
							  &my_adc[adc_num],
							  CHANNEL_0,
							  next_buffer[adc_num],
							  buffer_size_bytes,
							  local_buffer[adc_num]
							  [next_buffer
							   [adc_num]]);

				buffer_count[adc_num]++;
				DM35418_Check_Result(result,
					     "Error getting DMA buffer");

				result = DM35418_Dma_Reset_Buffer(board,
								  &my_adc
								  [adc_num],
								  CHANNEL_0,
								  next_buffer
								  [adc_num]);

				DM35418_Check_Result(result, "Error resetting buffer");

				next_buffer[adc_num] =
				    (next_buffer[adc_num] +
				     1) % my_adc[adc_num].num_dma_buffers;

				result = DM35418_Dma_Check_Buffer_Used(board,
								       &my_adc
								       [adc_num],
								       CHANNEL_0,
								       next_buffer
								       [adc_num],
								       &buffer_full);

				DM35418_Check_Result(result,
					     "Error finding used buffer.");

			}

			result = DM35418_Dma_Clear_Interrupt(board,
							     &my_adc[adc_num],
							     CHANNEL_0,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     CLEAR_INTERRUPT);

		} else {
			printf("*** Process non-DMA interrupt for FB 0x%x.\n",
			       int_info.interrupt_fb);
		}

		result = DM35418_Gbc_Ack_Interrupt(board);

		DM35418_Check_Result(result, "Error calling ACK interrupt.");

	}

}


/**
*******************************************************************************
@brief
    Setup the DACs to produce a sine wave as a signal to sample

@param
    my_dac

    Pointer to the DAC function block structure.

 @retval
    None.
 *******************************************************************************
*/
void setup_dacs(struct DM35418_Function_Block *my_dac)
{
	unsigned int dac_num;
	uint32_t actual_rate;
	int result;
	int32_t *dac_buffer;
	int16_t dac_min_value, dac_max_value;

	for (dac_num = 0; dac_num < DM35418_NUM_DAC_ON_BOARD; dac_num++) {
		result = DM35418_Dac_Open(board, dac_num, &my_dac[dac_num]);

		DM35418_Check_Result(result, "Could not open DAC");

		printf("Found DAC%u, with %d DMA channels (%d buffers each)\n",
		       dac_num, my_dac[dac_num].num_dma_channels,
		       my_dac[dac_num].num_dma_buffers);

		result = DM35418_Dac_Set_Clock_Src(board,
						   &my_dac[dac_num],
						   DM35418_CLK_SRC_IMMEDIATE);

		DM35418_Check_Result(result, "Error setting DAC clock");

		result = DM35418_Dac_Set_Conversion_Rate(board,
							 &my_dac[dac_num],
							 DAC_RATE,
							 &actual_rate);

		fprintf(stdout,
			"Rate requested: %d  Actual Rate Achieved: %d\n",
			DAC_RATE, actual_rate);
		DM35418_Check_Result(result, "Error setting sample rate");

		result = DM35418_Dac_Channel_Setup(board,
						   &my_dac[dac_num],
						   CHANNEL_0,
						   DM35418_DAC_RNG_BIPOLAR_5V);

		DM35418_Check_Result(result, "Error setting DAC output range");

		dac_buffer = (int *)malloc(BUFFER_SIZE_BYTES);

		DM35418_Check_Result(dac_buffer == NULL,
			     "Error allocating space for buffer.");

		result = DM35418_Dac_Volts_To_Conv(DM35418_DAC_RNG_BIPOLAR_5V,
						   4.9f, &dac_max_value);

		DM35418_Check_Result(result,
			     "Error converting value to conversion counts.");

		result = DM35418_Dac_Volts_To_Conv(DM35418_DAC_RNG_BIPOLAR_5V,
						   -5.0f, &dac_min_value);

		DM35418_Check_Result(result,
			     "Error converting value to conversion counts.");

		result = DM35418_Generate_Signal_Data(DM35418_SINE_WAVE,
						      dac_buffer,
						      BUFFER_SIZE_SAMPLES,
						      dac_max_value,
						      dac_min_value,
						      0, 0x0000FFFF);

		DM35418_Check_Result(result,
			     "Error trying to generate data for the DAC.");

		fprintf(stdout, "Initializing and configuring DAC DMA.");

		result =
		    DM35418_Dma_Initialize(board, &my_dac[dac_num],
					   CHANNEL_0, 1, BUFFER_SIZE_BYTES);

		DM35418_Check_Result(result, "Error initializing DMA");

		result = DM35418_Dma_Setup(board,
					   &my_dac[dac_num],
					   CHANNEL_0,
					   DM35418_DMA_SETUP_DIRECTION_WRITE,
					   IGNORE_USED);

		DM35418_Check_Result(result, "Error configuring DMA");

		fprintf(stdout, "success!\n");

		result = DM35418_Dma_Buffer_Setup(board,
						  &my_dac[dac_num],
						  CHANNEL_0,
						  BUFFER_0,
						  DM35418_DMA_BUFFER_CTRL_VALID |
						  DM35418_DMA_BUFFER_CTRL_LOOP);

		DM35418_Check_Result(result, "Error setting up buffer control.");

		result = DM35418_Dma_Write(board,
					   &my_dac[dac_num],
					   CHANNEL_0,
					   BUFFER_0,
					   BUFFER_SIZE_BYTES, dac_buffer);

		DM35418_Check_Result(result, "Writing to DMA buffer failed");

		fprintf(stdout, "Starting DMA Channel %d......", CHANNEL_0);
		result = DM35418_Dma_Start(board, &my_dac[dac_num], CHANNEL_0);

		DM35418_Check_Result(result, "Error starting DMA");

		printf("success.\n");

		free(dac_buffer);

		fprintf(stdout, "Starting DAC.\n");

		result = DM35418_Dac_Set_Start_Trigger(board,
						       &my_dac[dac_num],
						       DM35418_CLK_SRC_IMMEDIATE);

		DM35418_Check_Result(result, "Error setting start trigger for DAC.");

		result = DM35418_Dac_Set_Stop_Trigger(board,
						      &my_dac[dac_num],
						      DM35418_CLK_SRC_NEVER);

		DM35418_Check_Result(result, "Error setting stop trigger for DAC.");

		result = DM35418_Dac_Start(board, &my_dac[dac_num]);

		DM35418_Check_Result(result, "Error starting DAC");

	}


}


/**
*******************************************************************************
@brief
    Setup the ADCs to sample.

@param
    num_adc_to_use

    The number of ADCs that will be setup for sampling.

 @retval
    None.
 *******************************************************************************
*/
void setup_adcs(unsigned int num_adc_to_use)
{
	unsigned int adc_num;
	int buff;
	uint8_t buff_status, buff_control;
	uint32_t buff_size;
	int result;

	for (adc_num = 0; adc_num < num_adc_to_use; adc_num++) {
		result = DM35418_Adc_Open(board, adc_num, &my_adc[adc_num]);

		DM35418_Check_Result(result, "Could not open ADC");

		printf("Found ADC%d, with %d DMA channels (%d buffers each)\n",
		       adc_num, my_adc[adc_num].num_dma_channels,
		       my_adc[adc_num].num_dma_buffers);

		result = DM35418_Adc_Set_Clock_Src(board,
						   &my_adc[adc_num],
						   DM35418_CLK_SRC_IMMEDIATE);

		DM35418_Check_Result(result, "Error setting ADC clock");

		fprintf(stdout, "Initializing DMA Channel 0....");
		result = DM35418_Dma_Initialize(board,
						&my_adc[adc_num],
						CHANNEL_0,
						my_adc[adc_num].num_dma_buffers,
						buffer_size_bytes);
		printf("Errno: %d\n", errno);
		DM35418_Check_Result(result, "Error initializing DMA");

		result = DM35418_Dma_Setup(board,
					   &my_adc[adc_num],
					   CHANNEL_0,
					   DM35418_DMA_SETUP_DIRECTION_READ,
					   NOT_IGNORE_USED);

		DM35418_Check_Result(result, "Error configuring DMA");

		fprintf(stdout, "Setting DMA Interrupts......");
		result = DM35418_Dma_Configure_Interrupts(board,
							  &my_adc[adc_num],
							  CHANNEL_0,
							  INTERRUPT_ENABLE,
							  ERROR_INTR_ENABLE);

		DM35418_Check_Result(result, "Error setting DMA Interrupts");
		fprintf(stdout, "success!\n");

		next_buffer[adc_num] = 0;

		for (buff = 0; buff < my_adc[adc_num].num_dma_buffers; buff++) {

			buff_control = DM35418_DMA_BUFFER_CTRL_VALID |
					DM35418_DMA_BUFFER_CTRL_INTR;

			if (buff == (DM35418_NUM_ADC_DMA_BUFFERS - 1)) {
				buff_control |= DM35418_DMA_BUFFER_CTRL_LOOP;

			}

			result = DM35418_Dma_Buffer_Setup(board,
							  &my_adc[adc_num],
							  CHANNEL_0,
							  buff,
							  buff_control);

			DM35418_Check_Result(result, "Error setting buffer control.");

			result = DM35418_Dma_Buffer_Status(board,
							   &my_adc[adc_num],
							   CHANNEL_0,
							   buff,
							   &buff_status,
							   &buff_control,
							   &buff_size);

			DM35418_Check_Result(result, "Error getting buffer status.");

			fprintf(stdout,
				"    Buffer %d: Stat: 0x%x  Ctrl: 0x%x  Size: %d\n",
				buff, buff_status, buff_control, buff_size);
		}

		result = DM35418_Adc_Channel_Setup(board,
						   &my_adc[adc_num],
						   CHANNEL_0,
						   DM35418_ADC_RNG_BIPOLAR_5V,
						   DM35418_ADC_INPUT_SINGLE_ENDED);

		DM35418_Check_Result(result, "Error setting up channel.");

	}


}


/**
*******************************************************************************
@brief
    Setup the ADCs that will be triggered (started) off of the first ADC.
    This causes all of the ADCs to start sampling at the same time.

@param
    rate

    Sampling rate for all ADCs

@param
	num_adc_to_use

	Number of ADCs that will be used.

 @retval
    None.
 *******************************************************************************
*/
void setup_slave_adcs(uint32_t rate, unsigned int num_adc_to_use)
{
	unsigned int adc_num;
	uint32_t actual_rate;
	int result;

	for (adc_num = 1; adc_num < num_adc_to_use; adc_num++) {

		fprintf(stdout, "Starting ADC %d DMA ......", adc_num);
		result = DM35418_Dma_Start(board, &my_adc[adc_num], CHANNEL_0);

		DM35418_Check_Result(result, "Error starting DMA");

		printf("success.\nInitializing ADC......");
		result = DM35418_Adc_Set_Start_Trigger(board,
						       &my_adc[adc_num],
						       DM35418_CLK_SRC_BUS2);
		DM35418_Check_Result(result, "Error setting start trigger.");

		result = DM35418_Adc_Set_Stop_Trigger(board,
						      &my_adc[adc_num],
						      DM35418_CLK_SRC_NEVER);
		DM35418_Check_Result(result, "Error setting stop trigger.");

		result = DM35418_Adc_Set_Sample_Rate(board,
						     &(my_adc[adc_num]),
						     rate, &actual_rate);

		DM35418_Check_Result(result, "Failed to set sample rate for ADC.");
		fprintf(stdout,
			"success.\nFB:%d Rate requested: %d  Actual Rate Achieved: %d\n",
			adc_num, rate, actual_rate);

		result = DM35418_Adc_Initialize(board, &(my_adc[adc_num]));

		DM35418_Check_Result(result, "Failed or timed out initializing ADC.");

		printf("Starting ADC %d\n", adc_num);

		result = DM35418_Adc_Start(board, &my_adc[adc_num]);

		DM35418_Check_Result(result, "Error starting ADC");
	}

}


/**
*******************************************************************************
@brief
    Handler to detect when user hits Ctrl-C

 @retval
    None.
 *******************************************************************************
*/
void setup_ctrlc_handler()
{
	struct sigaction signal_action;

	signal_action.sa_handler = sigint_handler;
	sigfillset(&(signal_action.sa_mask));
	signal_action.sa_flags = 0;

	if (sigaction(SIGINT, &signal_action, NULL) < 0) {
		error(EXIT_FAILURE, errno, "ERROR: sigaction() FAILED");
	}

}


/**
*******************************************************************************
@brief
    Convert a binary data file to ASCII values.  The format will be the same
    as the data file produced without the --binary argument.  The example
    program will exit after finishing.


 @retval
    None.
 *******************************************************************************
*/
void convert_bin_to_txt(unsigned int num_adc, unsigned int samples_in_buff)
{

	FILE *fp_in, *fp_out;
	unsigned long adc_num, sample_num, output_index = 0;
	int num_read = 0;

	int *buff[DM35418_NUM_ADC_ON_BOARD];

	for (adc_num = 0; adc_num < num_adc; adc_num++) {
		buff[adc_num] = (int *) malloc(samples_in_buff * sizeof(int));
		if (buff[adc_num] == NULL) {
			error(EXIT_FAILURE, errno,
			"Error allocating memory to read binary file contents.\n");
		}
	}

	fp_in = fopen(BIN_FILE_NAME, "rb");

	if (fp_in == NULL) {
		error(EXIT_FAILURE, errno,
		"open() FAILED to open binary input file %s.\n", BIN_FILE_NAME);
	}

	fp_out = fopen(ASCII_FILE_NAME, "w");

	if (fp_out == NULL) {
		error(EXIT_FAILURE, errno,
		"open() FAILED to open ASCII output file %s.\n", ASCII_FILE_NAME);
	}

	adc_num = 0;
	num_read = 1;
	while (adc_num < num_adc && num_read > 0) {
		num_read = fread(buff[adc_num], sizeof(int), samples_in_buff, fp_in);
		adc_num ++;
	}

	while (num_read > 0) {

		for (sample_num = 0; sample_num < samples_in_buff; sample_num ++) {
			fprintf(fp_out, "%lu", output_index);
			for (adc_num = 0; adc_num < num_adc; adc_num ++) {

				fprintf(fp_out, "\t%d", buff[adc_num][sample_num]);
			}

			fprintf(fp_out, "\n");
			output_index ++;
		}

		adc_num = 0;
		while (adc_num < num_adc && num_read > 0) {
			num_read = fread(buff[adc_num], sizeof(int), samples_in_buff, fp_in);
			adc_num ++;
		}

	}

	for (adc_num = 0; adc_num < num_adc; adc_num++) {
		free(buff[adc_num]);

	}

	fclose(fp_in);
	fclose(fp_out);

}


/**
*******************************************************************************
@brief
    The main program.

@param
    argument_count

    Number of args passed on the command line, including the executable name

@param
    arguments

    Pointer to array of character strings, which are the args themselves.

@retval
    0

    Success

@retval
    Non-Zero

    Failure.

 *******************************************************************************
*/

int main(int argument_count, char **arguments)
{
	struct DM35418_Function_Block my_dac[DM35418_NUM_DAC_ON_BOARD];
	unsigned long int minor = 0;
	int result;
	int index;
	FILE *fp;

	unsigned int buffer_to_get, buff;
	unsigned int buffers_copied;
	unsigned long local_buffer_count[DM35418_NUM_ADC_ON_BOARD];
	unsigned long bytes_written = 0;
	unsigned long output_index = 0;

	// Initialize this to its largest possible value.
	unsigned long samples_to_collect = 0, samples_taken = 0;
	int store_in_binary = 0;
	unsigned int timeout_count = 0, samples_in_buffer;

	unsigned int adc_num = 0;
	unsigned int num_adc_to_use = DEFAULT_NUM_ADC;
	uint32_t rate = DEFAULT_RATE, actual_rate = 0;

	int help_option_given = 0, convert_bin_file = 0;
	int status;
	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, HELP_OPTION},
		{"minor", 1, 0, MINOR_OPTION},
		{"num_adc", 1, 0, ADC_NUM_OPTION},
		{"rate", 1, 0, RATE_OPTION},
		{"samples", 1, 0, SAMPLES_OPTION},
		{"binary", 0, 0, BINARY_OPTION},
		{"bin2txt", 0, 0, BIN2TXT_OPTION},
		{0, 0, 0, 0}
	};

	program_name = arguments[0];

	// Show usage, parse arguments
	while (1) {
		/*
		 * Parse the next command line option and any arguments it may require
		 */
		status = getopt_long(argument_count,
				     arguments, "", options, NULL);

		/*
		 * If getopt_long() returned -1, then all options have been processed
		 */
		if (status == -1) {
			break;
		}

		/*
		 * Figure out what getopt_long() found
		 */
		switch (status) {

		/*#################################################################
			  User entered '--help'
		################################################################# */
		case HELP_OPTION:
			help_option_given = 0xFF;
			break;

		/*#################################################################
			User entered '--minor'
		################################################################# */
		case MINOR_OPTION:
			/*
			 * Convert option argument string to unsigned long integer
			 */
			errno = 0;
			minor = strtoul(optarg, &invalid_char_p, 10);

			/*
			 * Catch unsigned long int overflow
			 */
			if ((minor == ULONG_MAX)
			    && (errno == ERANGE)) {
				error(0, 0,
				      "ERROR: Device minor number caused numeric overflow");
				usage();
			}

			/*
			 * Catch argument strings with valid decimal prefixes, for
			 * example "1q", and argument strings which cannot be converted,
			 * for example "abc1"
			 */
			if ((*invalid_char_p != '\0')
			    || (invalid_char_p == optarg)) {
				error(0, 0,
				      "ERROR: Non-decimal device minor number");
				usage();
			}

			break;

		/*#################################################################
			  User entered rate
		################################################################# */
		case RATE_OPTION:
			/*
			 * Convert option argument string to unsigned long integer
			 */
			errno = 0;
			rate = strtoul(optarg, &invalid_char_p, 10);

			/*
			 * Catch unsigned long int overflow
			 */
			if ((rate == ULONG_MAX)
			    && (errno == ERANGE)) {
				error(0, 0,
				      "ERROR: Rate number caused numeric overflow");
				usage();
			}

			/*
			 * Catch argument strings with valid decimal prefixes, for
			 * example "1q", and argument strings which cannot be converted,
			 * for example "abc1"
			 */
			if ((*invalid_char_p != '\0')
			    || (invalid_char_p == optarg)) {
				error(0, 0,
				      "ERROR: Non-decimal rate value entered");
				usage();
			}
			break;

		/*#################################################################
		   User entered '--num_adc'
		 ################################################################# */
		case ADC_NUM_OPTION:
			/*
			 * Convert option argument string to unsigned long integer
			 */
			errno = 0;
			num_adc_to_use = strtoul(optarg, &invalid_char_p, 10);

			/*
			 * Catch unsigned long int overflow
			 */
			if ((num_adc_to_use == ULONG_MAX)
			    && (errno == ERANGE)) {
				error(0, 0,
				      "ERROR: Num ADC caused numeric overflow");
				usage();
			}

			/*
			 * Catch argument strings with valid decimal prefixes, for
			 * example "1q", and argument strings which cannot be converted,
			 * for example "abc1"
			 */
			if ((*invalid_char_p != '\0')
			    || (invalid_char_p == optarg)) {
				error(0, 0, "ERROR: Non-decimal num ADC");
				usage();
			}

			break;

		/*#################################################################
		   User entered number of samples
		 ################################################################# */
		case SAMPLES_OPTION:
			/*
			 * Convert option argument string to unsigned long integer
			 */
			errno = 0;
			samples_to_collect =
			    strtoul(optarg, &invalid_char_p, 10);

			/*
			 * Catch unsigned long int overflow
			 */
			if ((samples_to_collect == ULONG_MAX)
			    && (errno == ERANGE)) {
				error(0, 0,
				      "ERROR: Samples number caused numeric overflow");
				usage();
			}

			/*
			 * Catch argument strings with valid decimal prefixes, for
			 * example "1q", and argument strings which cannot be converted,
			 * for example "abc1"
			 */
			if ((*invalid_char_p != '\0')
			    || (invalid_char_p == optarg)) {
				error(0, 0,
				      "ERROR: Non-decimal samples value entered");
				usage();
			}

			break;

		/*#################################################################
		   User entered '--binary'
		 ################################################################# */
		case BINARY_OPTION:
			/*
			 * '--binary' option has been seen
			 */
			store_in_binary = 0xFF;
			break;

		/*#################################################################
		   User entered '--bin2txt'
		 ################################################################# */
		case BIN2TXT_OPTION:
			convert_bin_file = 1;
			break;

		/*#################################################################
		   User entered unsupported option
		   ################################################################# */
		case '?':
			usage();
			break;

		/*#################################################################
		   getopt_long() returned unexpected value
		   ################################################################# */
		default:
			error(EXIT_FAILURE,
			      0,
			      "ERROR: getopt_long() returned unexpected value %#x",
			      status);
			break;
		}
	}

	/*
	 * Recognize '--help' option before any others
	 */

	if (help_option_given) {
		usage();
	}

	if (num_adc_to_use > DM35418_NUM_ADC_ON_BOARD) {
		error(0, 0,
		      "Error: Number of ADC to use exceeds number on board.");
		usage();
	}

	if (rate < 1 || rate > DM35418_ADC_MAX_RATE) {
		error(0, 0, "Error: Rate given not within range of board.");
		usage();
	}

	/*
	 * Trying to come up with a reasonable size for buffers that doesn't
	 * take forever to fill up with slower rates, but also makes it possible
	 * to run at higher rates.
	 */
	buffer_size_bytes = (rate * num_adc_to_use) / 50;
	if (buffer_size_bytes < (20 * sizeof(int))) {
		buffer_size_bytes = 20 * sizeof(int);
	}
	buffer_size_bytes &= ~0x03;
	samples_in_buffer = buffer_size_bytes / sizeof(int);

	if (convert_bin_file) {
		convert_bin_to_txt(num_adc_to_use, samples_in_buffer);
		return 0;
	}

	setup_ctrlc_handler();

	if (store_in_binary) {
		fp = fopen(BIN_FILE_NAME, "wb");
	} else {
		fp = fopen(ASCII_FILE_NAME, "w");
	}

	if (fp == NULL) {
		error(EXIT_FAILURE, errno,
		      "open() FAILED on output data file.\n");
	}

	printf("Opening board.....");
	result = DM35418_Board_Open(minor, &board);

	DM35418_Check_Result(result, "Could not open board");
	printf("success.\nResetting board.....");
	result = DM35418_Gbc_Board_Reset(board);

	DM35418_Check_Result(result, "Could not reset board");
	printf("success.\n");

	/**
	 * First, setup the DACS.  They will produce a sine wave that needs to
	 * be looped back to the ADC inputs.
	 */
	printf("success.\nOpening DACs......\n");
	setup_dacs(my_dac);

	setup_adcs(num_adc_to_use);


	// Allocate local memory for data.
	for (adc_num = 0; adc_num < num_adc_to_use; adc_num++) {
		buffer_count[adc_num] = 0;
		local_buffer_count[adc_num] = 0;

		local_buffer[adc_num] =
		    malloc(sizeof(int *) * my_adc[adc_num].num_dma_buffers);
		DM35418_Check_Result(local_buffer[adc_num] == NULL,
			     "Could not allocate for local buffer");

		for (buff = 0; buff < my_adc[adc_num].num_dma_buffers; buff++) {

			local_buffer[adc_num][buff] = malloc(buffer_size_bytes);

			DM35418_Check_Result(local_buffer[adc_num][buff] == NULL,
				     "Could not allocate for local buffer");

		}

	}

	fprintf(stdout, "success.\nInstalling user ISR .....");
	result = DM35418_General_InstallISR(board, ISR);
	DM35418_Check_Result(result, "DM35418_General_InstallISR()");

	fprintf(stdout, "success.\n");

	fprintf(stdout, "Starting ADC %d DMA ......", ADC_0);
	result = DM35418_Dma_Start(board, &my_adc[ADC_0], CHANNEL_0);

	DM35418_Check_Result(result, "Error starting DMA");

	printf("success.\nInitializing ADC......");
	result = DM35418_Adc_Set_Start_Trigger(board,
					       &my_adc[ADC_0],
					       DM35418_CLK_SRC_IMMEDIATE);
	DM35418_Check_Result(result, "Error setting start trigger.");

	result = DM35418_Adc_Set_Stop_Trigger(board,
					      &my_adc[ADC_0],
					      DM35418_CLK_SRC_NEVER);
	DM35418_Check_Result(result, "Error setting stop trigger.");

	/*
	 * Set the clock source global 2 to be driven by the ADC 0 start
	 * trigger.  All other ADC will then trigger off CSG2, thus
	 * causing all ADC to start sampling at the same time.
	 */
	result = DM35418_Adc_Set_Clock_Source_Global(board,
						     &my_adc[ADC_0],
						     DM35418_CLK_SRC_BUS2,
						     DM35418_ADC_CLK_BUS_SRC_START_TRIG);

	DM35418_Check_Result(result,
		     "Error setting clock source global to start trigger.");

	result = DM35418_Adc_Set_Sample_Rate(board,
					     &(my_adc[ADC_0]),
					     rate, &actual_rate);

	DM35418_Check_Result(result, "Failed to set sample rate for ADC.");
	fprintf(stdout,
		"success.\nFB:%d Rate requested: %d  Actual Rate Achieved: %d\n",
		ADC_0, rate, actual_rate);

	result = DM35418_Adc_Initialize(board, &(my_adc[ADC_0]));

	DM35418_Check_Result(result, "Failed or timed out initializing ADC.");

	setup_slave_adcs(rate, num_adc_to_use);

	printf("Starting ADC %d\n", ADC_0);

	result = DM35418_Adc_Start(board, &my_adc[ADC_0]);

	DM35418_Check_Result(result, "Error starting ADC");

	buffers_copied = 0;

	/*
	 * Loop here until an error occurs, or the user hits CTRL-C, or we've
	 * collected the requested number of samples.  Loop through the ADCs
	 * and see if a buffer has been copied from kernel space.  If so, then
	 * write it out to disk.
	 */
	while (!exit_program) {

		for (adc_num = 0; adc_num < num_adc_to_use; adc_num++) {
			if ((buffer_count[adc_num] -
			     local_buffer_count[adc_num]) >
			    my_adc[adc_num].num_dma_buffers) {
				fprintf(stdout,
					"Local buffer for ADC %d was overrun.\n",
					adc_num);
				exit_program = 1;
			} else {

				timeout_count = 0;
				while ((local_buffer_count[adc_num] ==
					buffer_count[adc_num])
				       && !exit_program
				       && (timeout_count < 5000)) {
					DM35418_Micro_Sleep(1000);
					timeout_count++;
				}

				if (timeout_count == 5000) {
					exit_program = 1;
				}

				if (exit_program) {
					break;
				}
			}
		}

		if (exit_program) {
			// Exit out of the while loop
			break;
		}

		// 1 buffer from each ADC is now complete.
		buffer_to_get =
		    local_buffer_count[ADC_0] % my_adc[ADC_0].num_dma_buffers;

		if (store_in_binary) {
			for (adc_num = 0; adc_num < num_adc_to_use; adc_num++) {

				fwrite(local_buffer[adc_num]
				       [buffer_to_get], sizeof(int),
				       samples_in_buffer, fp);

				bytes_written += buffer_size_bytes;

			}

		} else {
			for (index = 0; index < samples_in_buffer; index++) {

				fprintf(fp, "%lu\t", output_index);
				for (adc_num = 0; adc_num < num_adc_to_use;
				     adc_num++) {
					fprintf(fp, "%d\t",
						local_buffer[adc_num]
						[buffer_to_get][index]);
				}

				output_index++;
				fprintf(fp, "\n");
			}
		}

		buffers_copied++;
		if (buffers_copied % 10 == 0) {
			fprintf(stdout, "Copied %d buffers.\n", buffers_copied);
		}
		for (adc_num = 0; adc_num < num_adc_to_use; adc_num++) {
			local_buffer_count[adc_num]++;
		}

		if (samples_to_collect > 0) {

			samples_taken += samples_in_buffer;
			if (samples_taken >= samples_to_collect) {
				exit_program = 1;
			}
		}

		DM35418_Micro_Sleep(100);

	}


	for (adc_num = 0; adc_num < num_adc_to_use; adc_num++) {

		if (dma_has_error) {
			output_channel_status(board,
					      &my_adc[adc_num], CHANNEL_0);
		}

		result = DM35418_Dma_Configure_Interrupts(board,
							  &my_adc[adc_num],
							  CHANNEL_0,
							  INTERRUPT_DISABLE,
							  ERROR_INTR_DISABLE);

		DM35418_Check_Result(result, "Error setting DMA Interrupts");

		for (buff = 0; buff < my_adc[adc_num].num_dma_buffers; buff++) {

			free(local_buffer[adc_num][buff]);

		}

	}

	if (samples_to_collect && samples_taken >= samples_to_collect) {
		fprintf(stdout, "Reached number of samples (%lu)\n",
			samples_to_collect);
	}

	if (store_in_binary) {
		fprintf(stdout, "Wrote %lu bytes to file.\n", bytes_written);
	}

	fclose(fp);

	printf("Removing ISR\n");
	result = DM35418_General_RemoveISR(board);

	DM35418_Check_Result(result, "Error removing ISR.");

	result = DM35418_Gbc_Board_Reset(board);
	printf("Closing Board\n");
	result = DM35418_Board_Close(board);

	DM35418_Check_Result(result, "Error closing board.");

	printf("Example program successfully completed.\n");
	return 0;

}
