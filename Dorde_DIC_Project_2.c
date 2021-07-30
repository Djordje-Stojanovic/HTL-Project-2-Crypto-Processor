/******************************************************************************
 
Author: Dorde Stojanovic
supervisor: Roland Lezuo
Class: 5BHEL
Last Update: 17.04.2021
Programm: This program is related to the dic-project of second semester of th 5BHE-DIC-class
Github-Repo: https://github.com/Djordje-Stojanovic/FSST/tree/main/Stojanovic_DIC_PROJECT_2

*******************************************************************************/


//------------------------------------------------------------------------------
//Disclaimer: All Variables were written in CamelCase format
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//Imports:
#include <stdlib.h>
#include <string.h>
#include <stdio.h> 
#include <time.h>  
#include <sys/printk.h>
#include <drivers/uart.h>
#include <unistd.h> 
#include <device.h>
#include <zephyr.h>
#include <drivers/uart.h>
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//Definition of constant variables:
//For the UART:
#define UART_DEVICE DT_NODELABEL(uart0) //Nodelable according to Zephyr Documentation is used for
                                        // "Get a node identifier for a node label."
#define sizeOfStack 1024 //Reporesents the Stack Size for the Thread
#define preference 0 //Represents the preference of the thread
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//Further Initialization:
enum {init, avail}; //initialize enum
int state = init; //initialize state
const struct device *uart_dev; //initialize the struct "device"
struct uart_config uartconf; ////initialize the struct "uart_config"

//Initialization for Uart
void uartInput(void *, void *, void *); //Input of the UART
void uartOutput(void*, void*, void*); //Output of the UART
void uartProcess(void*, void*, void*); //Process of the UART
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//Thread Initialization:
K_THREAD_DEFINE(uartInputThreadIdentifier, sizeOfStack, //Input Thread initialization
		uartInput, NULL, NULL, NULL,
		preference, 0, 0);
K_THREAD_DEFINE(uartOutputThreadIdentifier, sizeOfStack, //Output Thread initialization
		uartOutput, NULL, NULL, NULL,
		preference, 0, 0);
K_THREAD_DEFINE(uartProcessThreadIdentifier, sizeOfStack, //Process Thread initialization
		uartProcess, NULL, NULL, NULL,
		preference, 0, 0);
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Message Queue Initialization:
K_MSGQ_DEFINE(uartMessageQueue, 100*sizeof(char), 10, 1); //message Queue inizialization
K_MSGQ_DEFINE(uartProcessMessageQueue, 100*sizeof(char), 10, 1); //message Queue Process initialization
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//this function is responsible for the uart input 
//there are 3 possible cases which will be stated bellow
void uartInput(void *firstPointer, void *secondPointer, void *thirdPointer)
{
	
	// Initializations
	ARG_UNUSED(firstPointer);
	ARG_UNUSED(secondPointer);
	ARG_UNUSED(thirdPointer);
	unsigned char input;

    //next thing is an infinite loop
	for(;;)
    {
		switch(state)
        {
            //if state is init:
			case init:
				if(!uart_poll_in(uart_dev, &input))
                { //if data is recieved, the following is printed.
					printk("Data has been recieved: %c\n", input);
					switch(input)
                    {
                        //if "." is recieved we should print "." back
						case '.':
							k_msgq_put(&uartMessageQueue, ".\n", K_FOREVER);
							break;
                        //else if we got "P" then we should change the state to avail
						case 'P':
							printk("Sate is beeing changed to avail\n");
							state = avail;
							break;
                        //if we dont get P or . then it should break
						default: break;
					}
				}
            //if state is avail:
			case avail: break; //the function is breaked
			default: break;
		}
		k_msleep(1); //According to Zephyr documentation:
                     // "This routine puts the current thread to sleep for -duration- milliseconds."
	}
	return; 
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//this function is responsible for the uart output 
void uartOutput(void *firstPointer, void *secondPointer, void *thirdPointer)
{

	// Initializations
	ARG_UNUSED(firstPointer);
	ARG_UNUSED(secondPointer);
	ARG_UNUSED(thirdPointer);
	unsigned char *output=malloc(100*sizeof(char));

    //next thing is an infinite loop
	for(;;) 
    {
        //resets the memmory 
		memset(output, 0, strlen(output)); 
        //Scan the message queue, if it is 0 then
		if(k_msgq_get(&uartMessageQueue, output, K_NO_WAIT)==0)
        { 
            //send data
			printk("Sending the data: <%s>\n", output);

			for(int i=0; i<strlen(output); i++)
            {
				uart_poll_out(uart_dev, *(output+i));
                //prints out the sent data
				printk("Data that has been sent: <%x>\n", *(output+i));
			}
		}
		k_msleep(1); //According to Zephyr documentation:
                     // "This routine puts the current thread to sleep for -duration- milliseconds."
	}
	return;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//this function is responsible for the uart process in general 
void uartProcess(void *firstPointer, void *secondPointer, void *thirdPointer)
{
	// Initializations
	ARG_UNUSED(firstPointer);
	ARG_UNUSED(secondPointer);
	ARG_UNUSED(thirdPointer);

    //next thing is an infinite loop
	for(;;)
    {
		switch(state)
        {
            //here the process should break if it is in state init
			case init:
				break;
            //if it is in state avail it should make sure to tell that processing is now avaliable
			case avail:
				k_msgq_put(&uartMessageQueue, "PROCESSING AVAILABLE\n", K_FOREVER);
                //telling that its gonna change state
				printk("Changing state the state back to init\n");
                //changing state
				state = init; 
				break;
			default: break;
		}
		k_msleep(1); //According to Zephyr documentation:
                     // "This routine puts the current thread to sleep for -duration- milliseconds."
	}
	return;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//this is the main function
void main(void)
{

	// Initializations
	uart_dev = device_get_binding(DT_LABEL(UART_DEVICE));

    // If uart is not found
	if(!uart_dev)
    {
        //print that it couldnt be found
		printk("UART could not be found\n");
		return;
	}
    //else print that UART was found
	printk("UART has been found\n");

	//Configuration of the UART
	uartconf.baudrate = 9600; //initialization of a fixed baudrate
	uartconf.parity = UART_CFG_PARITY_NONE; //initialization of a fixed parity
	uartconf.stop_bits = UART_CFG_STOP_BITS_1; //initialization of stop bits
	uartconf.data_bits = UART_CFG_DATA_BITS_8; //initialization of data bits
	uartconf.flow_ctrl = UART_CFG_FLOW_CTRL_NONE; //initialization of flow control (none)

    //If UART couldnt be configured
	if(!uart_configure(uart_dev, &uartconf))
    {
        //Print that it could not be configured
		printk("Configuration of UART failed\n");
		return;
	}
    //else print that it could been configured
	printk("UART configured\n");

	for(;;)
    {
		printk("\nmain is waiting for death\n"); 
		k_msleep(10*1000); // 10s time sleep
	}
}
//------------------------------------------------------------------------------
