/**
    @file

    @brief
        Example program which demonstrates the use of the non DMA ADC 
	Interrupts. 

    @verbatim
	This program will demonstrate the useage of various non-DMA interrupts and how to
	handle interrupts in the user ISR. The program initializes the board and ADC0 and
	waits for the input voltage to exceed one volt. Once that Interrupt occurs, the
	program fills and outputs the post trigger buffer as samples are taken. The program
	waits for the Post buffer to trigger an interrupt indicating that it is full and 
	closes the program.

	The program counts the number of interrutpts occuring between the trigger and the 
	post buffer filled interrupts. Sometimes it counts the trigger interrupt and/or
	misses interrupts generated from a very fast sampleing adc. This shows the lag
        between moving data through kernel space to user space without usign a DMA.	


        The program will continue to run until the sampling threshold is crossed or until
	CTRL-C is pressed.

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

    $Id: dm35418_adc_basic_ints.c 141531 2024-03-06 21:05:25Z lfrankenfield $
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
#define DEFAULT_RATE	1000	

/**
 * Total list of interrupt sources used
 */
#define TOTAL_INT_SRC_MASK \
	DM35418_ADC_INT_SAMPLE_TAKEN_MASK | \
	DM35418_ADC_INT_POST_BUFF_FULL_MASK | \
	DM35418_ADC_INT_START_TRIG_MASK 

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
 * Board Structure that holds the information of specific board config
 */
static struct DM35418_Board_Descriptor *board;
/**
 * ADC structure that is used to hold data used by the ISR and user program
 */
//static struct DM35418_Function_Block &my_adc[DM35418_NUM_ADC_ON_BOARD];
static struct DM35418_Function_Block my_adc;

/**
 * Boolean indicating whether or not to exit the program.
 */
volatile int exit_program = 0;
/**
 * Int that tries to count the number of interrupts occuring. 
 */
volatile int sample_count = 0;
/* 
 * Volatile ISR Control Flags
 */
volatile int buff_full_flag = 0;
volatile int trigger_flag = 0;


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
	uint16_t int_status;
	int block;
	if (int_info.error_occurred) {

		printf("ISR: Error received.\n");
		return;

	}
	// Get the function block and ignore the most signfificant bit.
	block = int_info.interrupt_fb & 0x7FFFFFFFF;
	// verify that the interrupt is valid -- verify that it came from the expected function block
	if (int_info.valid_interrupt && block == my_adc.fb_num) {
		// Store the adc in volatile memory for usage
		DM35418_Adc_Interrupt_Get_Status(board, &my_adc, &int_status);

		// Set buffer flag if buffer is full
		if(int_status & DM35418_ADC_INT_POST_BUFF_FULL_MASK)
		{
			buff_full_flag = 1;	
		}
		// Set trigger flag if the interrupt was flagged 
		if(int_status & DM35418_ADC_INT_START_TRIG_MASK)
		{
			trigger_flag = 1;
		}
		if(trigger_flag && (int_status & DM35418_ADC_INT_SAMPLE_TAKEN_MASK)) sample_count++;
	}
	// Clear all potential interrupt Sources	
	DM35418_Adc_Interrupt_Clear_Status(board,&my_adc,TOTAL_INT_SRC_MASK);
	// Clear the channel that is generating the interrupt as well for a threshold cross -- required
	DM35418_Adc_Channel_Interrupt_Clear_Status(board,&my_adc,CHANNEL_0,DM35418_ADC_CHAN_INTR_HIGH_THRESHOLD_MASK);
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
	unsigned long int minor = 0;
	int result;
	uint8_t mode_status;
	int prior_int = 0;
	uint8_t chanintval;	

	unsigned int adc_num = 0;
	unsigned int rate = DEFAULT_RATE;
	unsigned int actual_rate;
	unsigned int range = DEFAULT_RANGE;

	int help_option_given = 0;
	int status;
	struct sigaction signal_action;
	uint16_t interrupt_ena = 0;
	int32_t samp, samp2;

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
			} else if (strcmp(optarg, "10U") == 0) {
				range = DM35418_ADC_RNG_UNIPOLAR_10V;
			} else if (strcmp(optarg, "5B") == 0) {
				range = DM35418_ADC_RNG_BIPOLAR_5V;
			} else if (strcmp(optarg, "5U") == 0) {
				range = DM35418_ADC_RNG_UNIPOLAR_5V;
			} else if (strcmp(optarg, "2.5B") == 0) {
				range = DM35418_ADC_RNG_BIPOLAR_2_5V;
			} else if (strcmp(optarg, "2.5U") == 0) {
				range = DM35418_ADC_RNG_UNIPOLAR_2_5V;
			} else if (strcmp(optarg, "1.25U") == 0) {
				range = DM35418_ADC_RNG_UNIPOLAR_1_25V;
			} else if (strcmp(optarg, "1.25B") == 0) {
				range = DM35418_ADC_RNG_BIPOLAR_1_25V;
			} else if (strcmp(optarg, ".625B") == 0) {
				range = DM35418_ADC_RNG_BIPOLAR_625mV;
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

	// Begin Opening all of the ADC channels
	printf("Opening ADC......\n");
	result = DM35418_Adc_Open(board, adc_num, &my_adc);
	DM35418_Check_Result(result, "Could not open ADC");

	printf("Found ADC%d, with %d DMA channels (%d buffers each)\n",
		       adc_num, my_adc.num_dma_channels,
		       my_adc.num_dma_buffers);

	result = DM35418_Adc_Set_Clock_Src(board,
					   &my_adc,
					   DM35418_CLK_SRC_IMMEDIATE);

	DM35418_Check_Result(result, "Error setting ADC clock");
	/**
	 * Must have a pre trigger sample count > 0 in 
	 * order to trigger a threshold trigger or interrupt otherwise
	 * it is impossible to detect a sample higher than a threshold
	 */
	result = DM35418_Adc_Set_Pre_Trigger_Samples(board,&my_adc,1);
	DM35418_Check_Result(result, "Error setting pre-capture samples.");

	// Set number of post buffer samples taken to 10
	result = DM35418_Adc_Set_Post_Stop_Samples(board,&my_adc,10);
	DM35418_Check_Result(result, "Error setting post-capture samples.");

	fprintf(stdout, "Installing user ISR ...\n");
	result = DM35418_General_InstallISR(board, ISR);
	DM35418_Check_Result(result, "DM35418_General_InstallISR()");

	// Set interrupt for ADC 0
	result = DM35418_Adc_Volts_To_Sample(range, 1.0f, &samp2);
	DM35418_Check_Result(result, "Error getting a sample to voltage");

	result = DM35418_Adc_Channel_Set_High_Threshold(board,
                                        &my_adc,
                                        CHANNEL_0,
                                        samp2);
	DM35418_Check_Result(result, "Error setting high threshold");
	
	result = DM35418_Adc_Channel_Get_Thresholds(board,
                                        &my_adc,
                                        CHANNEL_0,
					&samp,
					&samp2);
	
	DM35418_Check_Result(result, "Error getting thresholds");
	printf("LOW  THRESHOLD [adc]: %d\n",samp);
	printf("HIGH THRESHOLD [adc]: %d\n",samp2);
	
	// configure interrupts
	result = DM35418_Adc_Interrupt_Set_Config(board,&my_adc,TOTAL_INT_SRC_MASK,INTERRUPT_ENABLE);
	DM35418_Check_Result(result, "Error setting interrupt.");
	
	result = DM35418_Adc_Channel_Interrupt_Set_Config(board,
							&my_adc,
							CHANNEL_0,
							DM35418_ADC_CHAN_INTR_HIGH_THRESHOLD_MASK,
							0xFF);
	DM35418_Check_Result(result, "Error setting interrupt.");
	
	result = DM35418_Adc_Channel_Interrupt_Get_Config(board, &my_adc,CHANNEL_0,&chanintval);
	printf("Channel Interrupt ENA: %d\n",chanintval);	
	// configure and start adcs 
	result = DM35418_Adc_Channel_Setup(board,
					   &my_adc,
					   CHANNEL_0,
					   range,
					   DM35418_ADC_INPUT_SINGLE_ENDED);
	DM35418_Check_Result(result, "Error setting up channel.");

	printf("Initializing ADC......\n");
	result = DM35418_Adc_Set_Start_Trigger(board,
					       &my_adc,
					       DM35418_CLK_SRC_CHAN_THRESH);
	DM35418_Check_Result(result, "Error setting start trigger.");

	result = DM35418_Adc_Set_Stop_Trigger(board,
					      &my_adc,
					      DM35418_CLK_SRC_CHAN_THRESH);
	DM35418_Check_Result(result, "Error setting stop trigger.");

	result = DM35418_Adc_Set_Sample_Rate(board,
					     &my_adc,
					     rate, &actual_rate);
	DM35418_Check_Result(result, "Failed to set sample rate for ADC.");
	fprintf(stdout,
		"FB:%d; Rate requested: %d  Actual Rate Achieved: %d\n",
		adc_num, rate, actual_rate);

	result = DM35418_Adc_Initialize(board, &my_adc);
	DM35418_Check_Result(result, "Failed or timed out initializing ADC.");

	result = DM35418_Adc_Start(board, &my_adc);
	DM35418_Check_Result(result, "Error starting ADC");
	
	result = DM35418_Adc_Interrupt_Get_Config(board,
						  &my_adc,
						  &interrupt_ena);
	DM35418_Check_Result(result, "Error getting interrupt value");

	printf("Interrupt enable flags set to: 0x%x\n", interrupt_ena);

	DM35418_Micro_Sleep(100);

	// Start Body
	printf("Begin Program. Wating for ADC0 voltage to exceed 1.0 Volts...\n");

	result = DM35418_Adc_Get_Mode_Status(board, &my_adc, &mode_status);
	DM35418_Check_Result(result, "Error getting Mode_status");

	if ((mode_status>>4) != DM35418_ADC_STAT_WAITING_START_TRIG) {
                printf("\nADC0 should be waiting for start trigger, but is not.\n");
                printf("\nStatus: 0x%x\n\n", mode_status);
		printf("interrupts that occured before main loop: %d\n",sample_count);
		printf("\nIs your voltage source on already?\n");
        }
	
	DM35418_Micro_Sleep(10);		
	
	
	// Begin top of program loop
	//Begin waiting for treshold trigger or exit signal
	while(!exit_program && !trigger_flag);
	if(!exit_program) printf("Trigger detected Sampling...\n");
	
	// Wait for buffer to be for or exit signal
	while(!exit_program && !buff_full_flag)
	{
		// Display any sample interrupt that the while loop can catch
		if(prior_int < sample_count)	
		{
			int my_value = 0;
			float volts = 0.0f;
			printf("Sample[%d]\n",sample_count);
			result = DM35418_Adc_Channel_Get_Last_Sample(board,
					&my_adc,
					CHANNEL_0,
					&my_value);
			
			result = DM35418_Adc_Sample_To_Volts(range,
						my_value, &volts);
			printf("  adc:%d\n volt: %f\n", my_value,volts);
			prior_int = sample_count;
		}
	}
	
	printf("Buffer Is Full... closing\n");		

	// Stop all the adcs and disable interrupts
	printf("\n\nStopping Adc %d............", adc_num);

	result = DM35418_Adc_Reset(board, &my_adc);
	DM35418_Check_Result(result, "Error restarting ADC");
	
	printf("success!\nDisabling interrupt.....");
	result = DM35418_Adc_Interrupt_Set_Config(board, &my_adc, 
			DM35418_ADC_INT_ALL_MASK,
		       	INTERRUPT_DISABLE);
	DM35418_Check_Result(result, "Error removing interrupt.");
		
	result = DM35418_Adc_Channel_Interrupt_Set_Config(board,
						&my_adc,
						CHANNEL_0,
						DM35418_ADC_CHAN_INTR_HIGH_THRESHOLD_MASK,
						INTERRUPT_DISABLE);	
	DM35418_Check_Result(result, "Error removing interrupt.");

	// Clean up the program -- must be done every non-error exit.
	printf("success!\nRemoving ISR......");
	result = DM35418_General_RemoveISR(board);

	DM35418_Check_Result(result, "Error removing ISR.");

	printf("Success.\n");

	result = DM35418_Gbc_Board_Reset(board);
	printf("Closing Board\n");
	result = DM35418_Board_Close(board);

	DM35418_Check_Result(result, "Error closing board.");
	printf("Example program successfully completed.\n");
	return 0;

}
