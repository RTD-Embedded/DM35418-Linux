/**
    @file

    @brief
	Demonstrates use of threshold values to control when data is collected.

    @verbatim
	This example samples data using the ADC Channel 0 input.  It will start
	collecting data when the threshold is exceeded and then collect
	10000 samples.  By default, it will start when the high threshold is
	exceeded, but you can select the low threshold instead with a command
	line argument.

	For convenience, a 5V to -5V sine wave is produced at the DAC Channel 0
	output. Simply connect CN4 Pin 33 to CN4 Pin 1 to execute this example
	without the need for external equipment.

	Samples taken are output to a data file (adc_thresh_dma.dat) and can be viewed
	with gnuplot using the plot_adc_thresh_dma file.

        Hit Ctrl-C to exit.

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

    $Id: dm35418_adc_thresh_dma.c 141531 2024-03-06 21:05:25Z lfrankenfield $
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
#include <time.h>
#include <sys/time.h>

#include "dm35418_gbc_library.h"
#include "dm35418_adc_library.h"
#include "dm35418_dac_library.h"

#include "dm35418_ioctl.h"
#include "dm35418_examples.h"
#include "dm35418_dma_library.h"
#include "dm35418.h"
#include "dm35418_util_library.h"

/**
 * Default rate to use (Hz)
 */
#define DEFAULT_RATE		2000

/**
 * Rate of the DAC, for its sine wave
 */
#define DAC_RATE		1000

/**
 * Upper voltage threshold
 */
#define ADC_THRESH_HIGH_VOLTS	3.0f

/**
 * Lower voltage threshold
 */
#define ADC_THRESH_LOW_VOLTS	-3.0f

/**
 * Number of samples to play out DAC pins
 */
#define DAC_BUFF_NUM_SAMPLES	1000

/**
 * Number of samples to take on the ADC when the trigger occurs.
 */
#define NUM_SAMPLES_PER_TRIGGER		(DAC_BUFF_NUM_SAMPLES * 2)

/**
 * Number of bytes in DAC sample buffer
 */
#define DAC_BUFF_SIZE_BYTES	(DAC_BUFF_NUM_SAMPLES * sizeof(int))

/**
 * Name of file when saving as ASCII
 */
#define ASCII_FILE_NAME "./adc_thresh_dma.txt"

/**
 * Name of file when saving as binary
 */
#define BIN_FILE_NAME "./adc_thresh_dma.bin"

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
static struct DM35418_Function_Block my_adc;

/**
 * Array of buffer counts, used to track progress of each ADC
 * as data is copied.
 */
static unsigned long buffer_count;

/**
 * Pointer to local memory buffer where data is copied from the kernel buffers
 * when a DMA buffer becomes full.
 */
static int **local_buffer;

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
static unsigned int next_buffer;

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

	fprintf(stderr, "\t--low\n");
	fprintf(stderr, "\t\tTrigger data collect off of the low threshold instead of high.\n");

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
    the cause of an error if it occurs.

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
	uint32_t function_block;
	int buffer_full = 0;
	int dma_error = 0;
	char message[200];

	function_block = int_info.interrupt_fb & 0x7FFFFFFF;

	if (int_info.valid_interrupt) {

		sprintf(message,
			"ISR: Could not match function block number (%d) to an ADC.",
			function_block);

		DM35418_Check_Result(my_adc.fb_num != function_block, message);

		// It's a DMA interrupt
		if (int_info.interrupt_fb < 0) {

			result = DM35418_Dma_Check_For_Error(board,
							     &my_adc,
							     CHANNEL_0,
							     &dma_error);

			DM35418_Check_Result(result, "Error checking for DMA error.");

			if (dma_error) {
				dma_has_error = 1;
				exit_program = 1;
				return;
			}
			result = DM35418_Dma_Check_Buffer_Used(board,
							      &my_adc,
							      CHANNEL_0,
							      next_buffer,
							      &buffer_full);

			DM35418_Check_Result(result, "Error finding used buffer.");

			while (buffer_full) {
				result = DM35418_Dma_Read(board,
							  &my_adc,
							  CHANNEL_0,
							  next_buffer,
							  buffer_size_bytes,
							  local_buffer
							  [next_buffer]);

				buffer_count++;
				DM35418_Check_Result(result,
					     "Error getting DMA buffer");

				result = DM35418_Dma_Reset_Buffer(board,
								  &my_adc,
								  CHANNEL_0,
								  next_buffer);

				DM35418_Check_Result(result, "Error resetting buffer");

				next_buffer = (next_buffer + 1) % my_adc.num_dma_buffers;
				result = DM35418_Dma_Check_Buffer_Used(board,
							      &my_adc,
							      CHANNEL_0,
							      next_buffer,
							      &buffer_full);

				DM35418_Check_Result(result,
					     "Error finding used buffer.");

			}

			result = DM35418_Dma_Clear_Interrupt(board,
							     &my_adc,
							     CHANNEL_0,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     NO_CLEAR_INTERRUPT,
							     CLEAR_INTERRUPT);

		}

	}

}


/**
*******************************************************************************
@brief
    Setup DAC 0 to provide a sine wave signal.  This is purely for convenience
    as a signal for the ADC to sample.

@param
    my_dac

    Pointer to the DAC function block structure

 @retval
    None.
 *******************************************************************************
*/
void setup_dac(struct DM35418_Function_Block *my_dac)
{
	int result;
	uint32_t actual_rate;
	int *dac_buffer;
	int16_t dac_min_value, dac_max_value;

	printf("Opening DACs......\n");

	result = DM35418_Dac_Open(board, DAC_0, my_dac);

	DM35418_Check_Result(result, "Could not open DAC");

	printf("Found DAC0, with %d DMA channels (%d buffers each)\n",
	       my_dac->num_dma_channels, my_dac->num_dma_buffers);

	result = DM35418_Dac_Set_Clock_Src(board,
						my_dac,
						DM35418_CLK_SRC_IMMEDIATE);

	DM35418_Check_Result(result, "Error setting DAC clock");

	result = DM35418_Dac_Set_Conversion_Rate(board,
						 my_dac, DAC_RATE, &actual_rate);

	fprintf(stdout,
		"Rate requested: %d  Actual Rate Achieved: %d\n",
		DAC_RATE, actual_rate);
	DM35418_Check_Result(result, "Error setting sample rate");

	result = DM35418_Dac_Channel_Setup(board,
					   my_dac,
					   CHANNEL_0,
					   DM35418_DAC_RNG_BIPOLAR_5V);

	DM35418_Check_Result(result, "Error setting DAC output range");

	dac_buffer = (int *)malloc(DAC_BUFF_SIZE_BYTES);

	DM35418_Check_Result(dac_buffer == NULL, "Error allocating space for buffer.");

	result = DM35418_Dac_Volts_To_Conv(DM35418_DAC_RNG_BIPOLAR_5V,
					   4.9f, &dac_max_value);

	DM35418_Check_Result(result, "Error converting value to conversion counts.");

	result = DM35418_Dac_Volts_To_Conv(DM35418_DAC_RNG_BIPOLAR_5V,
					   -5.0f, &dac_min_value);

	DM35418_Check_Result(result, "Error converting value to conversion counts.");

	result = DM35418_Generate_Signal_Data(DM35418_SINE_WAVE,
					      dac_buffer,
					      DAC_BUFF_NUM_SAMPLES,
					      dac_max_value,
					      dac_min_value, 0, 0x0000FFFF);

	DM35418_Check_Result(result, "Error trying to generate data for the DAC.");

	fprintf(stdout, "Initializing and configuring DAC DMA.");

	result =
	    DM35418_Dma_Initialize(board, my_dac,
				   CHANNEL_0, 1, DAC_BUFF_SIZE_BYTES);

	DM35418_Check_Result(result, "Error initializing DMA");

	result = DM35418_Dma_Setup(board,
				   my_dac,
				   CHANNEL_0,
				   DM35418_DMA_SETUP_DIRECTION_WRITE,
				   IGNORE_USED);

	DM35418_Check_Result(result, "Error configuring DMA");

	fprintf(stdout, "success!\n");

	result = DM35418_Dma_Buffer_Setup(board,
					  my_dac,
					  CHANNEL_0,
					  BUFFER_0,
					  DM35418_DMA_BUFFER_CTRL_VALID |
					  DM35418_DMA_BUFFER_CTRL_LOOP);

	DM35418_Check_Result(result, "Error setting up buffer control.");

	result = DM35418_Dma_Write(board,
				   my_dac,
				   CHANNEL_0,
				   BUFFER_0, DAC_BUFF_SIZE_BYTES, dac_buffer);

	DM35418_Check_Result(result, "Writing to DMA buffer failed");

	fprintf(stdout, "Starting DMA Channel %d......", CHANNEL_0);
	result = DM35418_Dma_Start(board, my_dac, CHANNEL_0);

	DM35418_Check_Result(result, "Error starting DMA");

	printf("success.\n");

	free(dac_buffer);

	fprintf(stdout, "Starting DAC.\n");

	result = DM35418_Dac_Set_Start_Trigger(board,
					       my_dac,
					       DM35418_CLK_SRC_IMMEDIATE);

	DM35418_Check_Result(result, "Error setting start trigger for DAC.");

	result = DM35418_Dac_Set_Stop_Trigger(board,
					      my_dac, DM35418_CLK_SRC_NEVER);

	DM35418_Check_Result(result, "Error setting stop trigger for DAC.");


}


/**
*******************************************************************************
@brief
    Setup ADC 0 to sample.  It will start sampling based on the trigger from the
    threshold crossing.

 @retval
    None.
 *******************************************************************************
*/
void setup_adc()
{
	int result;
	int buff;
	uint8_t buff_status, buff_control;
	uint32_t buff_size;
	uint32_t actual_rate;
	int32_t threshold_counts;


	result = DM35418_Adc_Open(board, ADC_0, &my_adc);

	DM35418_Check_Result(result, "Could not open ADC");

	printf("Found ADC0, with %d DMA channels (%d buffers each)\n",
	       my_adc.num_dma_channels, my_adc.num_dma_buffers);

	result = DM35418_Adc_Set_Clock_Src(board,
					   &my_adc, DM35418_CLK_SRC_IMMEDIATE);

	DM35418_Check_Result(result, "Error setting ADC clock");

    /* This setting is a requirement, as it forces sampling in order
       to know when we've crossed a threshold. */
	result = DM35418_Adc_Set_Pre_Trigger_Samples(board, &my_adc, 1);

	DM35418_Check_Result(result, "Error setting pre-capture samples.");

	result = DM35418_Adc_Set_Post_Stop_Samples(board,
						   &my_adc, NUM_SAMPLES_PER_TRIGGER);

	DM35418_Check_Result(result, "Error setting post-capture samples.");

	fprintf(stdout, "Initializing DMA Channel 0....");
	result = DM35418_Dma_Initialize(board,
					&my_adc,
					CHANNEL_0,
					my_adc.num_dma_buffers,
					buffer_size_bytes);

	DM35418_Check_Result(result, "Error initializing DMA");

	result = DM35418_Dma_Setup(board,
				   &my_adc,
				   CHANNEL_0,
				   DM35418_DMA_SETUP_DIRECTION_READ,
				   NOT_IGNORE_USED);

	DM35418_Check_Result(result, "Error configuring DMA");

	fprintf(stdout, "Setting DMA Interrupts......");
	result = DM35418_Dma_Configure_Interrupts(board,
						  &my_adc,
						  CHANNEL_0,
						  INTERRUPT_ENABLE,
						  ERROR_INTR_ENABLE);

	DM35418_Check_Result(result, "Error setting DMA Interrupts");
	fprintf(stdout, "success!\n");

	for (buff = 0; buff < my_adc.num_dma_buffers; buff++) {

		buff_control = DM35418_DMA_BUFFER_CTRL_VALID |
				DM35418_DMA_BUFFER_CTRL_INTR;

		if (buff == (DM35418_NUM_ADC_DMA_BUFFERS - 1)) {
			buff_control |= DM35418_DMA_BUFFER_CTRL_LOOP;

		}

		result = DM35418_Dma_Buffer_Setup(board,
						  &my_adc,
						  CHANNEL_0,
						  buff,
						  buff_control);

		DM35418_Check_Result(result, "Error setting buffer control.");

		result = DM35418_Dma_Buffer_Status(board,
						   &my_adc,
						   CHANNEL_0,
						   buff,
						   &buff_status,
						   &buff_control, &buff_size);

		DM35418_Check_Result(result, "Error getting buffer status.");

		fprintf(stdout,
			"    Buffer %d: Stat: 0x%x  Ctrl: 0x%x  Size: %d\n",
			buff, buff_status, buff_control, buff_size);
	}

	result = DM35418_Adc_Channel_Setup(board,
					   &my_adc,
					   CHANNEL_0,
					   DM35418_ADC_RNG_BIPOLAR_5V,
					   DM35418_ADC_INPUT_SINGLE_ENDED);


	DM35418_Check_Result(result, "Error setting up channel.");

	result = DM35418_Adc_Volts_To_Sample(DM35418_ADC_RNG_BIPOLAR_5V,
						ADC_THRESH_HIGH_VOLTS,
						&threshold_counts);

	DM35418_Check_Result(result, "Error converting volts to sample.");

	result = DM35418_Adc_Channel_Set_High_Threshold(board,
								&my_adc,
								CHANNEL_0,
								threshold_counts);

	DM35418_Check_Result(result, "Error setting high threshold.");

	result = DM35418_Adc_Volts_To_Sample(DM35418_ADC_RNG_BIPOLAR_5V,
						ADC_THRESH_LOW_VOLTS,
						&threshold_counts);

	DM35418_Check_Result(result, "Error converting volts to sample.");

	result = DM35418_Adc_Channel_Set_Low_Threshold(board,
								&my_adc,
								CHANNEL_0,
								threshold_counts);

	DM35418_Check_Result(result, "Error setting high threshold.");

	result = DM35418_Adc_Set_Start_Trigger(board,
					       &my_adc,
					       DM35418_CLK_SRC_CHAN_THRESH);

	DM35418_Check_Result(result, "Error setting start trigger.");

	result = DM35418_Adc_Set_Stop_Trigger(board,
					      &my_adc,
					      DM35418_CLK_SRC_CHAN_THRESH);
	DM35418_Check_Result(result, "Error setting stop trigger.");


	result = DM35418_Adc_Set_Sample_Rate(board,
					     &my_adc, DEFAULT_RATE, &actual_rate);

	DM35418_Check_Result(result, "Failed to set sample rate for ADC.");
	fprintf(stdout,
		"ADC Rate requested: %d  Actual Rate Achieved: %d\n",
		DEFAULT_RATE, actual_rate);

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
	struct DM35418_Function_Block my_dac;

	unsigned long int minor = 0, buff;
	int result;
	int index;
	FILE *fp;

	unsigned int buffer_to_get;
	unsigned int buffers_copied;
	unsigned long local_buffer_count;
	unsigned long output_index = 0;

	// Initialize this to its largest possible value.
	int store_in_binary = 0;

	struct sigaction signal_action;
	uint8_t mode_status;

	int use_low_threshold = 0;
	int help_option_given = 0;
	int status;
	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, HELP_OPTION},
		{"minor", 1, 0, MINOR_OPTION},
		{"low", 0, 0, LOW_THRESHOLD_OPTION},
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
		   User entered '--low'
		 ################################################################# */
		case LOW_THRESHOLD_OPTION:
			use_low_threshold = 0xFF;
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

	buffer_size_bytes = (100 * sizeof(int));

	signal_action.sa_handler = sigint_handler;
	sigfillset(&(signal_action.sa_mask));
	signal_action.sa_flags = 0;

	if (sigaction(SIGINT, &signal_action, NULL) < 0) {
		error(EXIT_FAILURE, errno, "ERROR: sigaction() FAILED");
	}

	if (store_in_binary) {
		fp = fopen(BIN_FILE_NAME, "wb");
	} else {
		fp = fopen(ASCII_FILE_NAME, "w");
	}

	if (fp == NULL) {
		error(EXIT_FAILURE, errno, "open() FAILED to open output file\n");
	}

	printf("Opening board.....");
	result = DM35418_Board_Open(minor, &board);

	DM35418_Check_Result(result, "Could not open board");
	printf("success.\nResetting board.....");
	result = DM35418_Gbc_Board_Reset(board);

	DM35418_Check_Result(result, "Could not reset board");
	printf("success.\n");

	/**
	 * Setup DAC 0 to produce a sine wave.  This can be looped to the ADC
	 * input to have a signal to measure.
	 */
	setup_dac(&my_dac);

	/* Setup the ADCs for sampling */
	setup_adc();

	buffer_count = 0;
	local_buffer_count = 0;
	next_buffer = 0;
	// Allocate local memory for data.

	local_buffer = malloc(sizeof(int *) * my_adc.num_dma_buffers);
	DM35418_Check_Result(local_buffer == NULL,
		     "Could not allocate for local buffer");

	for (buff = 0; buff < my_adc.num_dma_buffers; buff++) {

		local_buffer[buff] = malloc(buffer_size_bytes);

		DM35418_Check_Result(local_buffer[buff] == NULL,
			     "Could not allocate for local buffer");

	}

	printf("success.\n");

	fprintf(stdout, "Installing user ISR .....");
	result = DM35418_General_InstallISR(board, ISR);
	DM35418_Check_Result(result, "DM35418_General_InstallISR()");

	fprintf(stdout, "success.\n");

	fprintf(stdout, "Starting ADC0 DMA ......");
	result = DM35418_Dma_Start(board, &my_adc, CHANNEL_0);

	DM35418_Check_Result(result, "Error starting DMA");

	printf("success.\nInitializing ADC.\n");

	result = DM35418_Adc_Initialize(board, &my_adc);

	DM35418_Check_Result(result, "Failed or timed out initializing ADC.");

	if (use_low_threshold) {
		result = DM35418_Adc_Channel_Interrupt_Set_Config(board,
			&my_adc,
			CHANNEL_0,
			DM35418_ADC_CHAN_INTR_LOW_THRESHOLD_MASK,
			0xFF);

		DM35418_Check_Result(result, "Error setting channel interrupt configuration");
	}
	else {

		result = DM35418_Adc_Channel_Interrupt_Set_Config(board,
			&my_adc,
			CHANNEL_0,
			DM35418_ADC_CHAN_INTR_HIGH_THRESHOLD_MASK,
			0xFF);

		DM35418_Check_Result(result, "Error setting channel interrupt configuration");
	}


	printf("Starting ADC0\n");

	result = DM35418_Adc_Start(board, &my_adc);

	DM35418_Check_Result(result, "Error starting ADC");

	buffers_copied = 0;

	DM35418_Micro_Sleep(2000000);

	/* A quick check that the ADC is waiting in the correct mode */
	result = DM35418_Adc_Get_Mode_Status(board,
						 &my_adc,
						 &mode_status);

	DM35418_Check_Result(result, "Error getting mode/status from ADC");

	mode_status >>= 4;

	if (mode_status != DM35418_ADC_STAT_WAITING_START_TRIG) {
		printf("\nADC0 should be waiting for start trigger, but is not.\n");
		printf("\nStatus: 0x%x\n\n", mode_status);
		DM35418_Check_Result(1, "ADC had wrong status.");
	}

	printf("Starting DAC.  Sampling on ADC should begin when signal rises past threshold.\n");
	printf("Waiting for ADC to start....\n\n");

	result = DM35418_Dac_Start(board, &my_dac);

	DM35418_Check_Result(result, "Error starting DAC");

	while (mode_status == DM35418_ADC_STAT_WAITING_START_TRIG && !exit_program) {

		/* A quick check that the ADC is waiting in the correct mode */
		result = DM35418_Adc_Get_Mode_Status(board,
							 &my_adc,
							 &mode_status);

		DM35418_Check_Result(result, "Error getting mode/status from ADC");

		mode_status >>= 4;

		DM35418_Micro_Sleep(100);

	}

	if (!exit_program) {
		printf("ADC has started sampling...\n");
	}

	/*
	 * Loop here until an error occurs, or the user hits CTRL-C.  When a buffer
	 * has been copied to user space by the interrupt handler, then write it out
	 * to disk.
	 */
	while (!exit_program && mode_status != DM35418_ADC_STAT_DONE) {

		if ((buffer_count - local_buffer_count) >
		    my_adc.num_dma_buffers) {
			fprintf(stdout, "Local buffer for ADC0 was overrun.\n");
			exit_program = 1;
		} else {
			while (local_buffer_count < buffer_count
			       && !exit_program) {

				buffer_to_get =
				    local_buffer_count % my_adc.num_dma_buffers;

				if (store_in_binary) {
					fwrite(local_buffer
					       [buffer_to_get], sizeof(int),
					       (buffer_size_bytes / sizeof(int)), fp);

				} else {
					for (index = 0;
					     index <
					     (buffer_size_bytes / sizeof(int));
					     index++) {

						fprintf(fp, "%lu\t%d\n",
							output_index,
							local_buffer
							[buffer_to_get][index]);

						output_index++;

					}
				}

				buffers_copied++;
				if (buffers_copied % 10 == 0) {
					fprintf(stdout, "Copied %d buffers.\n",
						buffers_copied);
				}
				local_buffer_count++;
			}

			if (exit_program) {
				// Exit out of the while loop
				break;
			}
		}

		result = DM35418_Adc_Get_Mode_Status(board,
			&my_adc,
			&mode_status);

		DM35418_Check_Result(result, "Error getting mode/status from ADC");

		mode_status >>= 4;

		DM35418_Micro_Sleep(10);

	}

	if (mode_status == DM35418_ADC_STAT_DONE) {

		printf("\nSampling completed.\n");

	}


	result = DM35418_Adc_Channel_Interrupt_Clear_Status(board,
			&my_adc,
			CHANNEL_0,
			0xFF);


	DM35418_Check_Result(result, "Error trying to clear channel interrupt status.");

	if (dma_has_error) {
		printf("\n\nDMA Error detected.\n");
		output_channel_status(board, &my_adc, CHANNEL_0);
	}

	result = DM35418_Dma_Configure_Interrupts(board,
						  &my_adc,
						  CHANNEL_0,
						  INTERRUPT_DISABLE,
						  ERROR_INTR_DISABLE);

	DM35418_Check_Result(result, "Error setting DMA Interrupts");

	for (buff = 0; buff < my_adc.num_dma_buffers; buff++) {

		free(local_buffer[buff]);

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
