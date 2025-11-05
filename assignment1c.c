/*
    Assignment 1B: Interrupt Part 1
    Edoardo Pilia - 70/91/00138
 */
 
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"

// I/O interfaces (addresses)
volatile int * gpio_0_data = (volatile int*) 0x40000000; // LEDs
volatile int * gpio_1_data = (volatile int*) 0x40010000; // Switches
volatile int * gpio_2_data = (volatile int*) 0x40020000; // BTN1

// AXI Interrupt Controller Register (addresses)
volatile int * IER = (volatile int*) 0x41200008; // Interrupt Enable Register
volatile int * MER = (volatile int*) 0x4120001C; // Master Enable Register
volatile int * IISR = (volatile int*) 0x41200000; // Interrupt Status Register
volatile int * IIAR = (volatile int*) 0x4120000C; // Interrupt Acknowledge Register

// Registers' addresses for GPIO 1 (Switches, connected to INTC[0])
volatile int * GGIER_1 = (volatile int*) 0x4001011C; // Global GPIO Interrupt Enable Register
volatile int * GIER_1 = (volatile int*)	0x40010128; // GPIO Interrupt Enable Register
volatile int * GISR_1 = (volatile int*)	0x40010120; // GPIO Interrupt Status Register

// Registers' addresses for GPIO 2 (BTN1, connected to INTC[1])
volatile int * GGIER_2 = (volatile int*) 0x4002011C;
volatile int * GIER_2 = (volatile int*)	0x40020128;
volatile int * GISR_2 = (volatile int*)	0x40020120;


// MyISR handles interrupt
void myISR(void) __attribute__((interrupt_handler));

int main(void)
{

    // 1) (Optional) set inputs if these are input channels (TRI=1s)
    *(volatile int*)(0x40010000 + 0x4) = 0xFFFFFFFF; // GPIO1 TRI
    *(volatile int*)(0x40020000 + 0x4) = 0xFFFFFFFF; // GPIO2 TRI

    // 2) Enable device interrupts (GPIO)
	*GIER_1  = 0x1; // Enable interrupt for GPIO 1
	*GGIER_1 = 0x80000000; // Master enable for GPIO 1

	*GIER_2  = 0x1; // Enable interrupt for GPIO 2
	*GGIER_2 = 0x80000000; // Master enable for GPIO 2

	// 3) Enable INTC lines
    *IER = 0x3; // 0b11 // enable two channels (11) -> IRQ0 and IRQ1 (use 0x2 if only GPIO2 is wired)
    *MER = 0x3; // 0b11 // ME | HIE

    microblaze_enable_interrupts(); // Enable processor interrupts

	while (1) {
        //empty loop to keep the processor going
        //interrupt triggered by GPIO => loop stopped and ISR is executed
    }
}

int last_digit = 8;

// Interrupt Service Routines
void myISR(void)
{
    unsigned p = *IISR;  // Snapshot

    if (p & 0b0001) { 
    	*GISR_1 = 0x1;    // clear device first
    	*IIAR   = 0x1;    // then ack INTC

    	int sw_conf = *gpio_1_data; 	// Save the switches input configuration
    	int msb_weight = -1; 			// initialize MSB weight to -1 (MSB of the switch configuration)

    	// Obtain the MSB weight
    	while (sw_conf) {
    		sw_conf = sw_conf>>1;
    		msb_weight += 1;
    	}

    	if (msb_weight == -1) msb_weight = 0; // Needed if all switches are off

    	*gpio_0_data = msb_weight; // LEDs show MSB weight
    }

    if ((p & 0b0010)) { // p & 0x2 // GPIO2 on INTC[1]
    	if (*gpio_2_data) {
			*GISR_2 = 0x1;
			*IIAR   = 0x2;

			//Add the last digit of the student ID and then negate the current LED configuration
			*gpio_0_data = ~(*gpio_0_data + last_digit);
    	}
    }
}
