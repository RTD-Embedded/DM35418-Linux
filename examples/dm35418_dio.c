/**
    @file

    @brief
        Example program which demonstrates the use of the DIO.

    @verbatim

        This example program sets the lower 16-bits of DIO to
        output, and the upper 16-bits to input.  We'll then
        write every possible 16-bit value to the output
        and verify the same value on the input pins.

        This example requires a loopback from DIO Pins 0-15
        to DIO Pins 16-31.  Check the manual for which pins
        on connector CN3 correspond to those.

        Port 0, Pins 0-15: Output
        Port 0, Pins 16-31: Input

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

    $Id: dm35418_dio.c 141531 2024-03-06 21:05:25Z lfrankenfield $
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>

#include "dm35418_gbc_library.h"
#include "dm35418_dio_library.h"
#include "dm35418_ioctl.h"
#include "dm35418_util_library.h"
#include "dm35418_examples.h"

#define DM35418_DIO_DIRECTION		0x0000FFFF

/**
 * Name of the program as invoked on the command line
 */

static char *program_name;

struct DM35418_Board_Descriptor *board;
struct DM35418_Function_Block my_dio;

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
	fprintf(stderr, "\t\tShow this help screen and exit.\n\n");
	fprintf(stderr, "\t--minor NUM\n");
	fprintf(stderr, "\t\tSpecify the minor number (>= 0) of the board to open.  When not specified,\n");
	fprintf(stderr, "\t\tthe device file with minor 0 is opened.\n");
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
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

	int dio_num = 0;

	int help_option_given = 0;

	int status;

	uint32_t input_value;
	uint32_t output_value;

	char *invalid_char_p;
	struct option options[] = {
		{"help", 0, 0, 1},
		{"minor", 1, 0, 2},
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

	printf("Opening board.....");
	result = DM35418_Board_Open(minor, &board);

	DM35418_Check_Result(result, "Could not open board");
	printf("success.\nResetting board.....");
	result = DM35418_Gbc_Board_Reset(board);

	DM35418_Check_Result(result, "Could not reset board");
	printf("success.\nOpening DIO......");

	result = DM35418_Dio_Open(board, dio_num, &my_dio);

	DM35418_Check_Result(result, "Could not open DIO");

	printf("Found DIO%d\n", dio_num);

	/*
	 * Set the direction of bits 0-15 to output and bits 16-31 to input
	 */
	result = DM35418_Dio_Set_Direction(board,
					   &my_dio, DM35418_DIO_DIRECTION);

	DM35418_Check_Result(result, "Could not set direction of DIO pins.");

	output_value = 0;
	input_value = 0;

	for (output_value = 0; output_value <= 0xFFFF; output_value++) {

		result = DM35418_Dio_Set_Output_Value(board,
						      &my_dio, output_value);

		DM35418_Check_Result(result, "Could not set output value.");

		result = DM35418_Dio_Get_Input_Value(board,
						     &my_dio, &input_value);

		input_value >>= 16;

		printf("Output: %u\t\tInput: %u\n", output_value, input_value);

		DM35418_Check_Result(!(output_value == input_value),
			     "Values do not match!");

	}

	result = DM35418_Gbc_Board_Reset(board);
	printf("Closing Board\n");
	result = DM35418_Board_Close(board);

	DM35418_Check_Result(result, "Error closing board.");
	printf("Example program successfully completed.\n");
	return 0;

}
