# Linux Software (DM35418)

> SWP-700010151 rev J
>
> Version v06.00.01.155701

Copyright (C) RTD Embedded Technologies, Inc.  All Rights Reserved.

This software package is dual-licensed.  Source code that is compiled for
kernel mode execution is licensed under the GNU General Public License
version 2.  For a copy of this license, refer to the file
LICENSE_GPLv2.TXT (which should be included with this software) or contact
the Free Software Foundation.  Source code that is compiled for user mode
execution is licensed under the RTD End-User Software License Agreement.
For a copy of this license, refer to LICENSE.TXT or contact RTD Embedded
Technologies, Inc.  Using this software indicates agreement with the
license terms listed above.

## Table of Contents

- [Supported Hardware](#supported-hardware)
- [Supported Kernel Versions](#supported-kernel-versions)
- [Supported CPU Architecture](#supported-cpu-architecture)
- [Supported Compilers](#supported-compilers)
- [Driver](#driver)
- [Library Interface](#library-interface)
- [Header Files](#header-files)
- [Example Programs](#example-programs)
- [Known Limitations](#known-limitations)
- [Getting Technical Support](#getting-technical-support)

## Supported Hardware


This software supports the RTD DM35418 (legacy) and [DM35419](https://www.rtd.com/PC104/DM/analog%20IO/DM35x19.htm).


## Supported Kernel Versions

This software has been tested with the following Linux distributions and kernel
versions:

* Ubuntu 20.04 LTS (5.4 Kernel unmodified)
* Ubuntu 22.04 LTS (6.5 Kernel unmodified)

 Due to API differences between kernel versions, RTD cannot guarantee compatibility
 with kernels and distributions not listed above.  If a user wishes to use an 
 unsupported kernel/distribution, it may be necessary to modify the driver module 
 code and/or Makefiles for the specific Linux environment.
 
## Supported CPU Architecture
  
 This software has been validated on the following CPU architectures.
 
* Intel x86 (32-bit) dual-core
* Intel x86 (64-bit) single-core
* Intel x86 (64-bit) multi-core
* ARM64

## Supported Compilers

The driver software and example programs were compiled using the GNU gcc 
compiler, though porting to other compilers such as LLVM is possible.

## Driver

The directory `driver/` contains source code related to the driver.

In order to use a driver, one must first compile it, load it into the kernel,
and create device files for the board(s).  To do this, issue the following
commands while sitting in the `driver/` directory:

* `make`
* `sudo make load`

The driver module must be loaded before running any program which accesses a
DM35418 device.

## Library Interface

The directory `lib/` contains source code related to the user library. 
The DM35418 library is created with a file name of librtd-dm35418.a and is
statically linked.

Please refer to the software manual for details on using the user level library
functions.  These functions are prototyped in the file `include/dm35418_library.h`;
this header file must be included in any code which wishes to call library
functions. The library must be built before compiling the example programs or your
application. To build the library, issue the command `make` within `lib/`.


## Header Files

The directory `include/` contains all header files needed by the driver, example
programs, library, and user applications.


## Example Programs

The directory `examples/` contains source code related to the example programs,
which demonstrate how to use features of the DM35418 boards, test the driver, or
test the library.  In addition to source files, `examples/` holds other files as
well; the purpose of these files will be explained below.


To build the example programs, issue the command `make` within `examples/`.


The following files are provided in `examples/`:

### Makefile

Make description file for building example programs.

### [dm35418_adc.c](examples/dm35418_adc.c)

This example program demonstrates the use of the ADC and interrupt
handling.  An interrupt is generated each time an ADC has taken a 
sample.  When that interrupt happens, the programs gets the value
of the last sample and displays it on the screen.
            
Setup: Connect the signal of interest to the ADCx.0 pin and GND for
each ADC.
            
Usage: Display the command syntax by executing: 

```sh
./dm35418_adc --help
```

Hit CTRL-C to exit.

### [dm35418_adc_basic_ints.c](examples/dm35418_adc_basic_ints.c)

This example program demonstrates the use of the ADC and non-dma
interrupt handling.  An interrupt is generated when ADC0 recieves a
rising edge above 1 volt. The program will then take samples and exit. 
            
Setup: Connect the voltage source to the ADC0 pin and GND
            
Usage: Display the command syntax by executing:

```sh
./dm35418_adc_basic_ints --help
```
       
Hit CTRL-C to exit. Program will automatically exit when the user buffer is full.

### [dm35418_adc_continuous_dma.c](examples/dm35418_adc_continuous_dma.c)

This example program demonstrates the use of the ADC and DMA.  The 
example will collect data from the ADC via DMA, and then write the
data out to a file on disk.  The data can then be plotted using 
gnuplot and the plot_adc_dma file.
            
Setup: Connect the signal of interest to the ADCx.0 pin and GND for
each ADC.
            
Usage: Display the command syntax by executing: 

```sh
./dm35418_adc_continuous --help.
```

Hit CTRL-C to exit.

### [dm35418_adc_sync_trig_dma.c](examples/dm35418_adc_sync_trig_dma.c)

Demonstrates using the Syncbus input as an external trigger to sample
for the ADC.  "External" trigger is supplied by the DIO, and a signal
is produced by the DAC for sampling.
            
Setup:
DIO 0.0 (CN3 Pin 1) to Sync0+ (CN5 Pin 1) with a 100 Ohm resistor

DIO 0.1 (CN3 Pin 2) to Sync0- (CN5 Pin 2) with a 100 Ohm resistor

DAC 0.0 (CN4 Pin 33) to ADC0.0 (CN4 Pin 1), or signal of interest
               
Usage: Display the command syntax by executing:
                           
```sh
./dm35418_adc_sync_trig_dma --help
```

Hit CTRL-C to exit the example.
   
### [dm35418_adc_thresh_dma.c](examples/dm35418_adc_thresh_dma.c)

This example samples data using the ADC Channel 0 input.  It will start
collecting data when the threshold is exceeded and then collect
10000 samples.  By default, it will start when the high threshold is
exceeded, but you can select the low threshold instead with a command
line argument.

For convenience, a 5V to -5V sine wave is produced at the DAC Channel 0
output. 

Samples taken are output to a data file (adc_thresh_dma.dat) and can 
be viewed with gnuplot using the plot_adc_thresh_dma file.
            
Setup:
               
DAC 0.0 (CN4 Pin 33) to ADC0.0 (CN4 Pin 1), or signal of interest
    
Usage:  

```sh
./dm35418_adc_sync_trig_dma 
```                        
Hit CTRL-C to exit the example.   
   
### [dm35418_adc_to_dac_dma.c](examples/dm35418_adc_to_dac_dma.c) 
            
Demonstrates the ADC and DAC DMA working together.  A signal is sampled by the
ADC and then, after some transformation, is sent to the associated DAC DMA.
            
Setup:
               
Input signal to ADC x.0 (CN4 Pin 1, Pin 5, etc)
Oscilloscope to output signal on DAC x.0 (CN4 Pin 33, Pin 35, etc)
               
Usage: Display the command syntax by executing:

```sh
./dm35418_adc_to_dac_dma --help
```

Hit CTRL-C to exit the example.
            
### [dm35418_dac.c](examples/dm35418_dac.c)
This example program demonstrates the use of the DAC.  A voltage is
put out on the pin corresponding to the input from the user.  The voltage
can be easily changed to cover the full range of -5V to 5V.
            
Setup: Connect an oscilloscope to the DACx.0 pin associated with
the DAC in use.
            
Usage: Display the command syntax by executing:

```sh
./dm35418_dac --help
```

Follow the prompts on screen for changing the voltage.

### [dm35418_dac_dma.c](examples/dm35418_dac_dma.c)

This example program demonstrates the use of the DAC and DMA.  A 
wave pattern data is generated in the program, written to a DMA buffer,
and then sent to the DAC in a repeating loop, thus provding a
continuous cycling signal.
            
Setup: Connect an oscilloscope to the DACx.0 pin associated with
the DAC in use.
            
Usage: Display the command syntax by executing:

```sh
./dm35418_dac_dma --help.
```

### [dm35418_dac_syncbus.c](examples/dm35418_dac_syncbus.c)

This example program demonstrates the use of the DAC and DMA and
the SyncBus.  A wave pattern data is generated in the program, 
written to a DMA buffer, and then sent to the DAC in a repeating 
loop, thus provding a continuous cycling signal.  Using the SyncBus
you can synchronize the output waveforms across multiple DM35418 
boards.
            
Setup: This example requires more than 1 DM35418 in the stack.  
Connect all DM35418 boards SyncBus connector together, Pin 1 to
Pin 1, etc.  Connect an oscilloscope to the DACx.0 pin associated with
the DACs in use.
            
Usage: Display the command syntax by executing:

```sh
./dm35418_dac_syncbus --help.
```

You will have to designate one board to be master and the other boards
to be slaves.  You will execute a separate example program per board,
and give them the correct command lines to designate them as master or
slave.  Both boards at the end of the SyncBus connection should have 
their termination enabled via command line, and boards in the middle 
should have their termination disabled.
            
### [dm35418_dio.c](examples/dm35418_dio.c)

This example program demonstrates the use of the DIO.  It configures
half of the pins as output, and the other half as input.  Then, 
using a loopback connector, data is written to the output pins and 
read from the input pins, and then verified to be correct.
            
Setup: Use a loopback to connect Port 0 Pins 0 - 15 to Port 1 Pins
0 - 15.  Reference the board manual to determine correct pins on
connector CN3.

Usage: Display the command syntax by executing: 

```sh
./dm35418_dio --help
```

### [dm35418_list_fb.c](examples/dm35418_list_fb.c)

This example program demonstrates accessing the board-level 
registers to access the function blocks on the board.  The 
program will query every function block location to see if a 
valid function block type exists there, and if it does, it will
display that type on the screen.  In this way, it will give an
inventory of the function blocks on the board.
            
Setup: No setup required.

Usage: Display the command syntax by executing: 

```sh
./dm35418_list_fb --help
```

### [dm35418_temperature.c](examples/dm35418_temperature.c)

This example program demonstrates the temperature sensor on the board.
It will continually display the temperature on the screen until
stopped.
            
Press CTRL-C to exit.
            
Setup: No setup required.
            
Usage: Display the command syntax by executive: 

```sh
./dm35418_temperature --help
```



## Known Limitations

 1. This software was tested only on little-endian processors.  If you are using
    a big-endian CPU, you will need to examine the driver, example, and library
    source code for endianness issues and resolve them.

 2. Many conditions affect board throughput and interrupt performance.  For a
    discussion of these issues, please see the Application Note SWM-640000021
    (Linux Interrupt Performance) available on our web site.

 3. If you are using the interrupt wait mechanism, be aware that signals
    delivered to the application can cause the sleep to awaken prematurely.
    Interrupts may be missed if signals are delivered rapidly enough or at
    inopportune times.
    
 4. Boards with Rev A of the FPGA may have issues with DMA on 64-bit systems
    with more than 2 GB of RAM.



## Getting Technical Support

If you require additional support with this product, or any other products from
RTD Embedded Technologies, contact us using the information below:

RTD Embedded Technologies, Inc. \
103 Innovation Boulevard \
State College, PA 16803 USA

Telephone: (814) 234-8087 \
Fax: (814) 234-5218 \
Sales Information and Quotes: sales@rtd.com \
Technical Assistance: techsupport@rtd.com \
Web Site: [http://www.rtd.com](http://www.rtd.com)
