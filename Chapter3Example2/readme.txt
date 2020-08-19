
                        	USB Design By Example
                            	FX3 Example

Chapter 3: Example 1
--------------------

  I have added the Cypress Debug Console to provide progress messages on a terminal

  The device does not attach to USB.

  Files:

    1 cyfx_gcc_startup.S*	: Start-up code for the ARM-9 core on the FX3 device

    2 Application.h 		: Constant definitions for this application

    3 Application.c     	: C source file containing the example

    4 cyfxtx.c*				: ThreadX RTOS wrappers and utility functions required
      						  by the FX3 API library.

    5 makefile*				: GNU make compliant build script for compiling this example

    6 DebugConsole.c		: C source showing how to attach to a UART and CyU3PDebugPrint

    
* there should be no need to edit these files but please look what is there!

