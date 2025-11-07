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

int main(void)
{

    // 1) (Optional) set inputs if these are input channels (TRI=1s)
    *(volatile int*)(0x40010000 + 0x4) = 0xFFFFFFFF; // GPIO1 TRI

    while (1) {
       *gpio_0_data = *gpio_1_data; //this line set the LEDs' pattern equal to switches' pattern
    }
}


