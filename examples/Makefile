#
#	FILE NAME: Makefile
#
#	FILE DESCRIPTION: Make description file for building example programs
#
#	PROJECT NAME: Linux Software (DM35418)
#
#	PROJECT VERSION: (Defined in README.TXT)
#
#	This file and its contents are copyright (C) RTD Embedded Technologies,
#	Inc.  All Rights Reserved.
#
#	This software is licensed as described in the RTD End-User Software
#	License Agreement.  For a copy of this agreement, refer to the file
#	LICENSE.TXT (which should be included with this software) or contact RTD
#	Embedded Technologies, Inc.
#

CC=gcc
DEBUG_FLAGS=-g
INCLUDE_FLAGS=-I../include
LIBRARY_FLAGS=-L../lib -lrtd-dm35418 -lpthread -lm
OPTIMIZE_FLAGS=-O6
WARNING_FLAGS=-Wall
CFLAGS:=$(DEBUG_FLAGS) $(INCLUDE_FLAGS) $(OPTIMIZE_FLAGS) $(WARNING_FLAGS)

EXAMPLES = \
	dm35418_adc \
	dm35418_adc_basic_ints \
	dm35418_adc_continuous_dma \
	dm35418_adc_thresh_dma \
	dm35418_dac \
	dm35418_dac_dma \
	dm35418_dio \
	dm35418_dac_syncbus \
	dm35418_adc_sync_trig_dma \
	dm35418_adc_to_dac_dma \
	dm35418_temperature \
	dm35418_list_fb 


all:	$(EXAMPLES)

dm35418_adc:	dm35418_adc.c
	$(CC) $(CFLAGS) -o dm35418_adc dm35418_adc.c \
		$(LIBRARY_FLAGS)

dm35418_adc_basic_ints:	dm35418_adc_basic_ints.c
	$(CC) $(CFLAGS) -o dm35418_adc_basic_ints dm35418_adc_basic_ints.c \
		$(LIBRARY_FLAGS)

dm35418_adc_continuous_dma:	dm35418_adc_continuous_dma.c
	$(CC) $(CFLAGS) -o dm35418_adc_continuous_dma dm35418_adc_continuous_dma.c \
		$(LIBRARY_FLAGS)
		
dm35418_adc_thresh_dma:	dm35418_adc_thresh_dma.c
	$(CC) $(CFLAGS) -o dm35418_adc_thresh_dma dm35418_adc_thresh_dma.c \
		$(LIBRARY_FLAGS)
		
dm35418_adc_to_dac_dma:	dm35418_adc_to_dac_dma.c
	$(CC) $(CFLAGS) -o dm35418_adc_to_dac_dma dm35418_adc_to_dac_dma.c \
		$(LIBRARY_FLAGS)
		
dm35418_adc_sync_trig_dma:	dm35418_adc_sync_trig_dma.c
	$(CC) $(CFLAGS) -o dm35418_adc_sync_trig_dma dm35418_adc_sync_trig_dma.c \
		$(LIBRARY_FLAGS)

dm35418_dac:	dm35418_dac.c
	$(CC) $(CFLAGS) -o dm35418_dac dm35418_dac.c \
		$(LIBRARY_FLAGS)

dm35418_dac_dma:	dm35418_dac_dma.c
	$(CC) $(CFLAGS) -o dm35418_dac_dma dm35418_dac_dma.c \
		$(LIBRARY_FLAGS)

dm35418_dac_syncbus:	dm35418_dac_syncbus.c
	$(CC) $(CFLAGS) -o dm35418_dac_syncbus dm35418_dac_syncbus.c \
		$(LIBRARY_FLAGS)

dm35418_temperature:	dm35418_temperature.c
	$(CC) $(CFLAGS) -o dm35418_temperature dm35418_temperature.c \
		$(LIBRARY_FLAGS)

dm35418_dio:	dm35418_dio.c
	$(CC) $(CFLAGS) -o dm35418_dio	dm35418_dio.c \
		$(LIBRARY_FLAGS)


dm35418_list_fb:	dm35418_list_fb.c
	$(CC) $(CFLAGS) -o dm35418_list_fb dm35418_list_fb.c \
		$(LIBRARY_FLAGS)

clean:
	rm -f *.o *.dat *~ $(EXAMPLES) $(TESTS)
