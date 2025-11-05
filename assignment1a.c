/*
    Assignment 1A: Polling
    Edoardo Pilia - 70/91/00138
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"

// I/O interfaces (addresses)
volatile int * gpio_0_data = (volatile int*) 0x40000000; // LEDs
volatile int * gpio_1_data = (volatile int*) 0x40010000; // Switches

// AXI Interrupt Controller Register (addresses)
volatile int * IER = (volatile int*) 0x41200008; // Interrupt Enable Register
volatile int * MER = (volatile int*) 0x4120001C; // Master Enable Register
volatile int * IISR = (volatile int*) 0x41200000; // Interrupt Status Register
volatile int * IIAR = (volatile int*) 0x4120000C; // Interrupt Acknowledge Register

// Registers' addresses for GPIO 1 (Switches, connected to INTC[0])
volatile int * GGIER_1 = (volatile int*) 0x4001011C; // Global GPIO Interrupt Enable Register
volatile int * GIER_1 = (volatile int*)	0x40010128; // GPIO Interrupt Enable Register
volatile int * GISR_1 = (volatile int*)	0x40010120; // GPIO Interrupt Status Register


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

	// 3) Enable INTC lines
    *IER = 0x3; // 0b11 // enable two channels (11) -> IRQ0 and IRQ1 (use 0x2 if only GPIO2 is wired)
    *MER = 0x3; // 0b11 // ME | HIE

    microblaze_enable_interrupts(); // Enable processor interrupts

    while (1) {
        //empty loop to keep the processor going
        //interrupt triggered by GPIO => loop stopped and ISR is executed
    }
}

// Interrupt Service Routines
void myISR(void)
{
    unsigned p = *IISR;  // Snapshot
    if (p & 0b0001) { // p & 0x1 // GPIO1 on INTC[0]
    	*GISR_1 = 0x1;    // clear device first
    	*IIAR   = 0x1;    // then ack INTC

    	*gpio_0_data = *gpio_1_data; // LED matches the switches config
    }
}
