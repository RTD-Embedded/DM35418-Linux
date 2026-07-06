/**
    @file

    @brief
        Example program which demonstrates the use of the ADC, setting
        and responding to interrupts.

    @verbatim

        This example program uses an ADC to collect data.  An interrupt
        is generated every time data is collected by the ADC.
        After acknowledging the interrupt, the program queries the value
        last taken by the ADC, and the sample counter, and prints them
        to the screen.

	You can put any signal you want on the ADC input pins.
	However, for convenience, this example sets up the DACs to provide
	a signal for the ADC to measure.  In order for that to work, you
	must loopback the DAC outputs to the ADC inputs (DAC0.0 to ADC0.0,
	DAC1.0 to ADC1.0, etc)

        The program will continue to run until CTRL-C is pressed.

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

    $Id: dm35418_adc.c 141531 2024-03-06 21:05:25Z lfrankenfield $
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>

#include "dm35418_gbc_library.h"
#include "dm35418_adc_library.h"
#include "dm35418_dac_library.h"
#include "dm35418_dma_library.h"
#include "dm35418_ioctl.h"
#include "dm35418_examples.h"
#include "dm35418_util_library.h"
#include "dm35418_board_access.h"
#include "dm35418_types.h"
#include "dm35418.h"

/**
 * Rate to run at, if the user does not provide one. (Hz)
 */
#define DEFAULT_RATE	6400

/**
 * DAC rate to use. (Hz)
 */
#define DAC_RATE	25

/**
 * Number of samples to play out DAC pins
 */
#define BUFFER_SIZE_SAMPLES	100

/**
 * Number of bytes in DAC sample buffer
 */
#define BUFFER_SIZE_BYTES	(BUFFER_SIZE_SAMPLES * sizeof(int))

/**
 * Define a default range to use, if the user does not
 * provide one.
 */
#define DEFAULT_RANGE		DM35418_ADC_RNG_BIPOLAR_5V

/**
 * Name of the program as invoked on the command line
 */
static char *program_name;

/**
 * Count of interrupts that have happened.
 */
volatile int interrupt_count = 0;

/**
 * Boolean indicating whether or not to exit the program.
 */
volatile int exit_program = 0;

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
	fprintf(stderr, "\t\tUse the specified sampling rate (Hz).  The default is %d.\n", DEFAULT_RATE);
	fprintf(stderr, "\t--range RNG\n");
	fprintf(stderr, "\t\tUse the specified range and mode of the ADC.\n");
	fprintf(stderr, "\t\t\t10B = 10V, Bipolar\n");
	fprintf(stderr, "\t\t\t10U = 10V, Unipolar\n");
	fprintf(stderr, "\t\t\t5B = 5V, Bipolar (Default)\n");
	fprintf(stderr, "\t\t\t5U = 5V, Unipolar\n");
	fprintf(stderr, "\t\t\t2.5B = 2.5V, Bipolar\n");
	fprintf(stderr, "\t\t\t2.5U = 2.5V, Unipolar\n");
	fprintf(stderr, "\t\t\t1.25B = 1.25V, Bipolar\n");
	fprintf(stderr, "\t\t\t1.25U = 1.25V, Unipolar\n");
	fprintf(stderr, "\t\t\t.625B = 0.625V, Bipolar\n");
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

/**
*******************************************************************************
@brief
    The interrupt subroutine that will execute when an interrupt occurs.  It will
    simply increment a count, which the main program
 *******************************************************************************
*/

void ISR(struct dm35418_ioctl_interrupt_info_request int_info)
{

	if (int_info.error_occurred) {

		printf("ISR: Error received.\n");
		return;

	}

	if (int_info.valid_interrupt) {

		interrupt_count++;

	}

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
	struct DM35418_Board_Descriptor *board;
	struct DM35418_Function_Block my_adc[DM35418_NUM_ADC_ON_BOARD];
	struct DM35418_Function_Block my_dac[DM35418_NUM_DAC_ON_BOARD];
	unsigned long int minor = 0;
	int result;
	int my_value[DM35418_NUM_ADC_ON_BOARD];
	uint32_t sample_counts;
	int last_int_count = 0;
	uint16_t int_status;

	unsigned int adc_num = 0, dac_num = 0;
	unsigned int rate = DEFAULT_RATE;
	unsigned int actual_rate;
	unsigned int range = DEFAULT_RANGE;

	int help_option_given = 0;
	int status;
	struct sigaction signal_action;
	float dac_max_volts = 5.0f, dac_min_volts = -4.9f;
	uint16_t interrupt_ena = 0;
	float volts = 0;

	int16_t max_value, min_value;
	int32_t *buffer;

	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, HELP_OPTION},
		{"minor", 1, 0, MINOR_OPTION},
		{"rate", 1, 0, RATE_OPTION},
		{"range", 1, 0, RANGE_OPTION},
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
		   User entered --range option
		   ################################################################# */
		case RANGE_OPTION:
			if (strcmp(optarg, "10B") == 0) {
				range = DM35418_ADC_RNG_BIPOLAR_10V;
				dac_max_volts = 9.9f;
				dac_min_volts = -9.9f;
			} else if (strcmp(optarg, "10U") == 0) {
				range = DM35418_ADC_RNG_UNIPOLAR_10V;
				dac_max_volts = 9.9f;
				dac_min_volts = 0;
			} else if (strcmp(optarg, "5B") == 0) {
				range = DM35418_ADC_RNG_BIPOLAR_5V;
				dac_max_volts = 5.0f;
				dac_min_volts = -5.0f;
			} else if (strcmp(optarg, "5U") == 0) {
				range = DM35418_ADC_RNG_UNIPOLAR_5V;
				dac_max_volts = 5.0f;
				dac_min_volts = 0;
			} else if (strcmp(optarg, "2.5B") == 0) {
				range = DM35418_ADC_RNG_BIPOLAR_2_5V;
				dac_max_volts = 2.5f;
				dac_min_volts = -2.5f;
			} else if (strcmp(optarg, "2.5U") == 0) {
				range = DM35418_ADC_RNG_UNIPOLAR_2_5V;
				dac_max_volts = 2.5f;
				dac_min_volts = 0;
			} else if (strcmp(optarg, "1.25U") == 0) {
				range = DM35418_ADC_RNG_UNIPOLAR_1_25V;
				dac_max_volts = 1.25f;
				dac_min_volts = 0;
			} else if (strcmp(optarg, "1.25B") == 0) {
				range = DM35418_ADC_RNG_BIPOLAR_1_25V;
				dac_max_volts = 1.25f;
				dac_min_volts = -1.25f;
			} else if (strcmp(optarg, ".625B") == 0) {
				range = DM35418_ADC_RNG_BIPOLAR_625mV;
				dac_max_volts = 0.625f;
				dac_min_volts = -0.625f;
			} else {
				error(0, 0,
				      "ERROR: Range and mode entered did not match available options.");
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

	/**
	 * First, setup the DACS.  They will produce a sine wave that needs to
	 * be looped back to the ADC inputs.
	 */
	printf("success.\nOpening DACs......\n");

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
						   DM35418_DAC_RNG_BIPOLAR_10V);

		DM35418_Check_Result(result, "Error setting DAC output range");

		buffer = (int *)malloc(BUFFER_SIZE_BYTES);

		DM35418_Check_Result(buffer == NULL,
			     "Error allocating space for buffer.");

		result = DM35418_Dac_Volts_To_Conv(DM35418_DAC_RNG_BIPOLAR_10V,
						   dac_max_volts, &max_value);

		DM35418_Check_Result(result,
			     "Error converting value to conversion counts.");

		result = DM35418_Dac_Volts_To_Conv(DM35418_DAC_RNG_BIPOLAR_10V,
						   dac_min_volts, &min_value);

		DM35418_Check_Result(result,
			     "Error converting value to conversion counts.");

		result = DM35418_Generate_Signal_Data(DM35418_SINE_WAVE,
						      buffer,
						      BUFFER_SIZE_SAMPLES,
						      max_value,
						      min_value, 0, 0x0000FFFF);

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
					   BUFFER_0, BUFFER_SIZE_BYTES, buffer);

		DM35418_Check_Result(result, "Writing to DMA buffer failed");

		fprintf(stdout, "Starting DMA Channel %d......", CHANNEL_0);
		result = DM35418_Dma_Start(board, &my_dac[dac_num], CHANNEL_0);

		DM35418_Check_Result(result, "Error starting DMA");

		printf("success.\n");

		free(buffer);

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

	printf("Opening ADC......\n");
	for (adc_num = 0; adc_num < DM35418_NUM_ADC_ON_BOARD; adc_num++) {
		result = DM35418_Adc_Open(board, adc_num, &(my_adc[adc_num]));

		if (result == -1 && errno == ENODEV) {
			break;
		}
		DM35418_Check_Result(result, "Could not open ADC");

		printf("Found ADC%d, with %d DMA channels (%d buffers each)\n",
		       adc_num, my_adc[adc_num].num_dma_channels,
		       my_adc[adc_num].num_dma_buffers);

		result = DM35418_Adc_Set_Clock_Src(board,
						   &(my_adc[adc_num]),
						   DM35418_CLK_SRC_IMMEDIATE);

		DM35418_Check_Result(result, "Error setting ADC clock");

		result = DM35418_Adc_Set_Pre_Trigger_Samples(board,
							     &(my_adc[adc_num]),
							     0);

		DM35418_Check_Result(result, "Error setting pre-capture samples.");

		result = DM35418_Adc_Set_Post_Stop_Samples(board,
							   &(my_adc[adc_num]),
							   0);

		DM35418_Check_Result(result, "Error setting post-capture samples.");

	}

	fprintf(stdout, "Installing user ISR ...\n");
	result = DM35418_General_InstallISR(board, ISR);
	DM35418_Check_Result(result, "DM35418_General_InstallISR()");

	result = DM35418_Adc_Interrupt_Set_Config(board,
						  &(my_adc[0]),
						  DM35418_ADC_INT_SAMPLE_TAKEN_MASK,
						  INTERRUPT_ENABLE);

	DM35418_Check_Result(result, "Error setting interrupt.");

	for (adc_num = 0; adc_num < DM35418_NUM_ADC_ON_BOARD; adc_num++) {

		result = DM35418_Adc_Channel_Setup(board,
						   &(my_adc[adc_num]),
						   CHANNEL_0,
						   range,
						   DM35418_ADC_INPUT_SINGLE_ENDED);

		DM35418_Check_Result(result, "Error setting up channel.");

		printf("Initializing ADC......\n");
		result = DM35418_Adc_Set_Start_Trigger(board,
						       &(my_adc[adc_num]),
						       DM35418_CLK_SRC_IMMEDIATE);
		DM35418_Check_Result(result, "Error setting start trigger.");

		result = DM35418_Adc_Set_Stop_Trigger(board,
						      &(my_adc[adc_num]),
						      DM35418_CLK_SRC_NEVER);
		DM35418_Check_Result(result, "Error setting stop trigger.");

		result = DM35418_Adc_Set_Sample_Rate(board,
						     &(my_adc[adc_num]),
						     rate, &actual_rate);

		DM35418_Check_Result(result, "Failed to set sample rate for ADC.");
		fprintf(stdout,
			"FB:%d; Rate requested: %d  Actual Rate Achieved: %d\n",
			adc_num, rate, actual_rate);

		result = DM35418_Adc_Initialize(board, &(my_adc[adc_num]));

		DM35418_Check_Result(result, "Failed or timed out initializing ADC.");

		result = DM35418_Adc_Start(board, &(my_adc[adc_num]));

		DM35418_Check_Result(result, "Error starting ADC");

		result = DM35418_Adc_Interrupt_Get_Config(board,
							  &(my_adc[adc_num]),
							  &interrupt_ena);

		DM35418_Check_Result(result, "Error getting interrupt value");

		printf("Interrupt enable set to: 0x%x\n", interrupt_ena);
	}

	printf("Sample Count");
	for (adc_num = 0; adc_num < DM35418_NUM_ADC_ON_BOARD; adc_num++) {

		printf("\tVoltage %d.", adc_num);

	}
	printf("\n");
	printf("============");
	for (adc_num = 0; adc_num < DM35418_NUM_ADC_ON_BOARD; adc_num++) {

		printf("\t==========");

	}
	printf("\n");

	while (!exit_program) {
		if (last_int_count < interrupt_count) {

			result = DM35418_Adc_Interrupt_Get_Status(board,
								  &(my_adc[0]),
								  &int_status);
			DM35418_Check_Result(result, "Error getting interrupt status");

			result = DM35418_Adc_Get_Sample_Count(board,
							      &(my_adc[0]),
							      &sample_counts);

			DM35418_Check_Result(result, "Error getting sample count.");

			printf("%12d", sample_counts);

			for (adc_num = 0; adc_num < DM35418_NUM_ADC_ON_BOARD;
			     adc_num++) {

				result =
				    DM35418_Adc_Channel_Get_Last_Sample(board,
							&(my_adc[adc_num]),
							CHANNEL_0,
							&(my_value[adc_num]));

				DM35418_Check_Result(result,
					     "Error getting ADC value.");

				result =
				    DM35418_Adc_Sample_To_Volts
				    (range, my_value[adc_num], &volts);

				DM35418_Check_Result(result,
					     "Error converting ADC sample to volts.");

				printf("\t%+2.5f", volts);

			}
			printf("\n");

			result = DM35418_Adc_Interrupt_Clear_Status(board,
								    &(my_adc[0]),
								    int_status);
			DM35418_Check_Result(result, "Error clearing interrupt status");

			last_int_count++;
		}

		DM35418_Micro_Sleep(100);

	}

	for (adc_num = 0; adc_num < DM35418_NUM_ADC_ON_BOARD; adc_num++) {
		printf("\n\nStopping Adc %d............", adc_num);

		result = DM35418_Adc_Reset(board, &(my_adc[adc_num]));

		DM35418_Check_Result(result, "Error starting ADC");

		printf("success!\nDisabling interrupt.....");

		result = DM35418_Adc_Interrupt_Set_Config(board,
							  &(my_adc[adc_num]),
							  DM35418_ADC_INT_SAMPLE_TAKEN_MASK,
							  INTERRUPT_DISABLE);
	}

	DM35418_Check_Result(result, "Error removing interrupt.");

	printf("success!\nRemoving ISR......");
	result = DM35418_General_RemoveISR(board);

	DM35418_Check_Result(result, "Error removing ISR.");

	printf("success.\n");

	result = DM35418_Gbc_Board_Reset(board);
	printf("Closing Board\n");
	result = DM35418_Board_Close(board);

	DM35418_Check_Result(result, "Error closing board.");
	printf("Example program successfully completed.\n");
	return 0;

}
