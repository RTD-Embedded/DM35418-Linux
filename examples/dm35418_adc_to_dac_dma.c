/**
    @file

    @brief
        Demonstrates transferring data from ADC DMA to DAC DMA.

    @verbatim

        This example program will collect data from the ADCs at an
        arbitrary rate and be transferred via DMA.  Once a buffer is
        completed, it will undergo a quick transformation and then be
        written to the corresponding DAC DMA buffer (ADC0 to DAC0, etc).
        The DAC will then write the data out at the same arbitrary rate.

        In this way, a signal can be fed into ADC0 and it will come out
        of DAC0.  DAC0 can be looped to ADC1 and the result will come out
        DAC1, etc.

        The example will run continuously until the user hits CTRL-C.

	Maximum sustainable throughput is system dependent. Higher
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

    $Id: dm35418_adc_to_dac_dma.c 141531 2024-03-06 21:05:25Z lfrankenfield $
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
#define DEFAULT_RATE		100000

/**
 * Default number of ADC to use, if user does not provide a num
 */
#define NUM_FB_TO_USE		4

/**
 * Number of samples to play out DAC pins
 */
#define BUFFER_SIZE_SAMPLES	5000

/**
 * Number of bytes in DAC sample buffer
 */
#define BUFFER_SIZE_BYTES	(BUFFER_SIZE_SAMPLES * sizeof(int))

/**
 * Name of the program as invoked on the command line
 */

static char *program_name;

/**
 * Boolean flag indicating if there was an ADC DMA error.
 */
static int adc_dma_error = 0;

/**
 * Pointer to board descriptor
 */
static struct DM35418_Board_Descriptor *board;

/**
 * Pointer to array of function blocks that will hold the descriptors
 */
static struct DM35418_Function_Block my_adc[NUM_FB_TO_USE];
static struct DM35418_Function_Block my_dac[NUM_FB_TO_USE];

/**
 * Array of buffer counts, used to track progress of each ADC
 * as data is copied.
 */
static unsigned long buff_cnt[NUM_FB_TO_USE];

/**
 * Pointer to local memory buffer where data is copied from the
 * kernel buffers when a DMA buffer becomes full.
 */
static int **local_buffer[NUM_FB_TO_USE];

/**
 * Boolean indicating the program should exit.
 */
static volatile int exit_program = 0;

 /**
 * Which buffer is next to be copied from DMA
 */
static unsigned int next_buffer[NUM_FB_TO_USE];

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
	fprintf(stderr, "USAGE\n\n\t%s BOARD [OPTIONS]\n\n", program_name);

	fprintf(stderr, "OPTIONS\n\n");
	fprintf(stderr, "\t--help\n");
	fprintf(stderr, "\t\tShow this help screen and exit.\n");
	fprintf(stderr, "\t--minor NUM\n");
	fprintf(stderr, "\t\tSpecify the minor number (>= 0) of the board to open.  When not specified,\n");
	fprintf(stderr, "\t\tthe device file with minor 0 is opened.\n");
	fprintf(stderr, "\t--rate RATE\n");
	fprintf(stderr, "\t\tUse the specified rate (Hz).  The default is %d.\n", DEFAULT_RATE);
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

		while (adc_num < 0 && adc_index < NUM_FB_TO_USE) {
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
				adc_dma_error = 1;
				exit_program = 1;
				return;
			}

			result = DM35418_Dma_Check_Buffer_Used(board,
							       &my_adc[adc_num],
							       CHANNEL_0,
							       next_buffer
							       [adc_num],
							       &buffer_full);

			DM35418_Check_Result(result,
				     "Error checking if buffer is used.");

			while (buffer_full) {
				result = DM35418_Dma_Read(board,
							  &my_adc[adc_num],
							  CHANNEL_0,
							  next_buffer[adc_num],
							  BUFFER_SIZE_BYTES,
							  local_buffer[adc_num]
							  [next_buffer
							   [adc_num]]);

				buff_cnt[adc_num]++;
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
					     "Error checking if buffer is used.");

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
    Setup the DACs to output continuously via DMA

@param
    rate

    Rate of data output.  This should be the same as the rate of data input.

 @retval
    None.
 *******************************************************************************
*/
void setup_dacs(uint32_t rate)
{
	unsigned int dac_num, buff;
	int result = 0;
	uint32_t actual_rate;
	uint8_t buff_status, buff_control;
	uint32_t buff_size;

	for (dac_num = 0; dac_num < NUM_FB_TO_USE; dac_num++) {
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
							 rate, &actual_rate);

		fprintf(stdout,
			"Rate requested: %d  Actual Rate Achieved: %d\n",
			rate, actual_rate);
		DM35418_Check_Result(result, "Error setting sample rate");

		result = DM35418_Dac_Channel_Setup(board,
						   &my_dac[dac_num],
						   CHANNEL_0,
						   DM35418_DAC_RNG_BIPOLAR_5V);

		DM35418_Check_Result(result, "Error setting DAC output range");

		fprintf(stdout, "Initializing and configuring DAC DMA.");

		result = DM35418_Dma_Initialize(board, &my_dac[dac_num],
					   CHANNEL_0,
					   my_dac[dac_num].num_dma_buffers,
					   BUFFER_SIZE_BYTES);

		DM35418_Check_Result(result, "Error initializing DMA");

		result = DM35418_Dma_Setup(board,
					   &my_dac[dac_num],
					   CHANNEL_0,
					   DM35418_DMA_SETUP_DIRECTION_WRITE,
					   NOT_IGNORE_USED);

		DM35418_Check_Result(result, "Error configuring DMA");

		fprintf(stdout, "success!\nSetting up DAC DMA buffers....\n");

		for (buff = 0; buff < my_dac[dac_num].num_dma_buffers; buff++) {

			buff_control = DM35418_DMA_BUFFER_CTRL_VALID;

			if (buff == (DM35418_NUM_ADC_DMA_BUFFERS - 1)) {
				buff_control |= DM35418_DMA_BUFFER_CTRL_LOOP;

			}

			result = DM35418_Dma_Buffer_Setup(board,
							  &my_dac[dac_num],
							  CHANNEL_0,
							  buff,
							  buff_control);

			DM35418_Check_Result(result,
				     "Error setting up buffer control.");

			result = DM35418_Dma_Buffer_Status(board,
							&my_dac[dac_num],
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


		if (dac_num == DAC_0) {
			result = DM35418_Dac_Set_Clock_Source_Global(board,
								     &my_dac
								     [dac_num],
								     DM35418_CLK_SRC_BUS3,
								     DM35418_DAC_CLK_BUS_SRC_START_TRIG);

			DM35418_Check_Result(result,
				     "Error setting clock source global to start trigger.");

			result = DM35418_Dac_Set_Start_Trigger(board,
							       &my_dac[dac_num],
							       DM35418_CLK_SRC_IMMEDIATE);

			DM35418_Check_Result(result,
				     "Error setting start trigger for DAC0.");
		} else {
			result = DM35418_Dac_Set_Start_Trigger(board,
							       &my_dac[dac_num],
							       DM35418_CLK_SRC_BUS3);

			DM35418_Check_Result(result,
				     "Error setting start trigger for DAC.");

			DM35418_Dac_Start(board, &my_dac[dac_num]);

			DM35418_Check_Result(result, "Error starting DAC");
		}

		result = DM35418_Dac_Set_Stop_Trigger(board,
						      &my_dac[dac_num],
						      DM35418_CLK_SRC_NEVER);

		DM35418_Check_Result(result, "Error setting stop trigger for DAC.");

		printf("success.\n");

	}


}


/**
*******************************************************************************
@brief
    Set the ADCs to continuously sample data into the DMA buffer.


 @retval
    None.
 *******************************************************************************
*/
void setup_adcs()
{
	unsigned int adc_num, buff;
	int result;
	uint8_t buff_status, buff_control;
	uint32_t buff_size;

	for (adc_num = 0; adc_num < NUM_FB_TO_USE; adc_num++) {
		result = DM35418_Adc_Open(board, adc_num, &my_adc[adc_num]);

		DM35418_Check_Result(result, "Could not open ADC");

		printf("Found ADC%d, with %d DMA channels (%d buffers each)\n",
		       adc_num, my_adc[adc_num].num_dma_channels,
		       my_adc[adc_num].num_dma_buffers);

		result = DM35418_Adc_Set_Clock_Src(board,
						   &my_adc[adc_num],
						   DM35418_CLK_SRC_IMMEDIATE);

		DM35418_Check_Result(result, "Error setting ADC clock");

		buff_cnt[adc_num] = 0;

		fprintf(stdout, "Initializing DMA Channel 0....");
		result = DM35418_Dma_Initialize(board,
						&my_adc[adc_num],
						CHANNEL_0,
						my_adc[adc_num].num_dma_buffers,
						BUFFER_SIZE_BYTES);
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
    Install the interrupt subroutine.

@retval
    None.
 *******************************************************************************
*/
void setup_isr()
{

	int result;

	fprintf(stdout, "Installing user ISR .....");
	result = DM35418_General_InstallISR(board, ISR);
	DM35418_Check_Result(result, "DM35418_General_InstallISR()");

	fprintf(stdout, "success.\n");

}


/**
*******************************************************************************
@brief
    Start all of the "slave" ADCs.  These started ADCs will wait until the
    start trigger, which is the starting of ADC0.  In this way, all ADCs will
    start together.

@param
    rate

    Rate at which to sample data.

 @retval
    None.
 *******************************************************************************
*/
void start_slave_adcs(uint32_t rate)
{
	unsigned int adc_num;
	int result;
	uint32_t actual_rate;

	for (adc_num = 1; adc_num < NUM_FB_TO_USE; adc_num++) {

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

	unsigned long int minor = 0;
	int result;
	int index;

	int buff;

	unsigned int buffer_to_get;
	unsigned int buffers_copied;
	unsigned long loc_buff_ct[NUM_FB_TO_USE];
	unsigned long output_index = 0;

	// Initialize this to its largest possible value.
	unsigned long samples_to_collect = -1;
	unsigned int timeout_count = 0;

	struct sigaction signal_action;

	unsigned int adc_num = 0, dac_num = 0;
	uint32_t rate = DEFAULT_RATE, actual_rate = 0;
	int dac_dma_error = 0;

	int help_option_given = 0;
	int status;
	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, HELP_OPTION},
		{"minor", 1, 0, MINOR_OPTION},
		{"rate", 1, 0, RATE_OPTION},
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
		   User entered unsupported option
		################################################################# */
		case '?':
			usage();
			break;

		/*#################################################################
		   get_opt_long() returned unexpected value
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

	if (rate < 1 || rate > DM35418_ADC_MAX_RATE) {
		error(0, 0, "Error: Rate given not within range of board.");
		usage();
	}

	signal_action.sa_handler = sigint_handler;
	sigfillset(&(signal_action.sa_mask));
	signal_action.sa_flags = 0;

	if (sigaction(SIGINT, &signal_action, NULL) < 0) {
		error(EXIT_FAILURE, errno, "ERROR: sigaction() FAILED");
	}

	printf("Opening board.....");
	result = DM35418_Board_Open(minor, &board);

	DM35418_Check_Result(result, "Could not open board");
	printf("success.\nResetting board.....");
	result = DM35418_Gbc_Board_Reset(board);

	DM35418_Check_Result(result, "Could not reset board");
	printf("success.\n");

	setup_dacs(rate);

	setup_adcs();

	// Allocate local memory for data.
	for (adc_num = 0; adc_num < NUM_FB_TO_USE; adc_num++) {

		local_buffer[adc_num] =
		    malloc(sizeof(int *) * my_adc[adc_num].num_dma_buffers);
		DM35418_Check_Result(local_buffer[adc_num] == NULL,
			     "Could not allocate for local buffer");

		for (buff = 0; buff < my_adc[adc_num].num_dma_buffers; buff++) {

			local_buffer[adc_num][buff] = malloc(BUFFER_SIZE_BYTES);

			DM35418_Check_Result(local_buffer[adc_num][buff] == NULL,
				     "Could not allocate for local buffer");

		}

		loc_buff_ct[adc_num] = 0;
	}

	setup_isr();

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

	start_slave_adcs(rate);

	printf("Starting ADC %d\n", ADC_0);

	result = DM35418_Adc_Start(board, &my_adc[ADC_0]);

	DM35418_Check_Result(result, "Error starting ADC");

	buffers_copied = 0;

	/*
	 * Loop here until an error occurs, or the user hits CTRL-C, or we've
	 * collected the requested number of samples.  Loop through the ADCs
	 * and see if a buffer has been copied from kernel space.  If so, then
	 * convert it to DAC scale and write it to DAC DMA.
	 */
	while (!exit_program && (output_index < samples_to_collect)) {

		for (adc_num = 0; adc_num < NUM_FB_TO_USE; adc_num++) {
			int buff_diff =
			    buff_cnt[adc_num] - loc_buff_ct[adc_num];
			int overrun =
			    buff_diff > my_adc[adc_num].num_dma_buffers;
			if (overrun) {
				fprintf(stdout,
					"Local buffer for ADC %d was overrun.\n",
					adc_num);
				exit_program = 1;
			} else {

				timeout_count = 0;

				/* Wait for buffer to be updated by the ISR */
				while ((loc_buff_ct[adc_num] ==
					buff_cnt[adc_num])
				       && !exit_program
				       && (timeout_count < 5000)) {
					DM35418_Micro_Sleep(100);
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
		    loc_buff_ct[ADC_0] % my_adc[ADC_0].num_dma_buffers;

		for (adc_num = 0; adc_num < NUM_FB_TO_USE; adc_num++) {
			for (index = 0; index < BUFFER_SIZE_SAMPLES; index++) {
				local_buffer[adc_num][buffer_to_get][index] *=
				    0.25;

			}

			result = DM35418_Dma_Write(board,
						   &my_dac[adc_num],
						   CHANNEL_0,
						   buffer_to_get,
						   BUFFER_SIZE_BYTES,
						   local_buffer[adc_num]
						   [buffer_to_get]);

			DM35418_Check_Result(result, "Writing to DMA buffer failed");

			result = DM35418_Dma_Reset_Buffer(board,
							  &my_dac[adc_num],
							  CHANNEL_0,
							  buffer_to_get);

			DM35418_Check_Result(result, "Error clearing buffer for DAC.");

			result = DM35418_Dma_Check_For_Error(board,
							     &my_dac[adc_num],
							     CHANNEL_0,
							     &dac_dma_error);

			DM35418_Check_Result(result,
				     "Error checking for DMA error from DAC.");

			if (dac_dma_error) {
				exit_program = 1;
				printf("DMA error detected for DAC.\n");
			}

			loc_buff_ct[adc_num]++;
		}

		buffers_copied++;
		if (buffers_copied % 100 == 0) {
			fprintf(stdout, "Copied %d buffers.\n",
				(buffers_copied * NUM_FB_TO_USE));
		}

		if (buff_cnt[ADC_0] == 2) {

			/* Give the ADC a 2 buffer head start */
			for (dac_num = 0; dac_num < NUM_FB_TO_USE; dac_num++) {

				result =
				    DM35418_Dma_Start(board, &my_dac[dac_num],
						      CHANNEL_0);

			}

			result = DM35418_Dac_Start(board, &my_dac[DAC_0]);

			DM35418_Check_Result(result, "Error starting DAC");

			fprintf(stdout, "Started DACs.\n");
		}

		DM35418_Micro_Sleep(100);

	}


	for (adc_num = 0; adc_num < NUM_FB_TO_USE; adc_num++) {

		if (adc_dma_error) {
			output_channel_status(board,
					      &my_adc[adc_num], CHANNEL_0);
		}
		if (dac_dma_error) {
			output_channel_status(board,
					      &my_dac[adc_num], CHANNEL_0);
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
