/*
    Assignment 2A: UART Part 1
    Edoardo Pilia - 70/91/00138
 */


/*
This code supports only 128x128 images, with an header of 15 bytes.
*/


#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "xil_printf.h"
#include "xuartps.h"

unsigned char image_array[128*128*3+15];

void receive_image(int*min, int* max);
void negative();


int main()
{
    init_platform();
    XUartPs Uart_1_PS;
    u16 DeviceId_1= XPAR_PS7_UART_1_DEVICE_ID;
    int Status_1;
    XUartPs_Config *Config_1;
    Config_1 = XUartPs_LookupConfig(DeviceId_1);
    if (NULL == Config_1) {
      return XST_FAILURE;
    }
    Status_1 = XUartPs_CfgInitialize(&Uart_1_PS, Config_1, Config_1->BaseAddress); //inizializzo la UART
    if (Status_1 != XST_SUCCESS) {
      return XST_FAILURE;
    }
    u32 BaudRate = (u32)115200;
    Status_1 = XUartPs_SetBaudRate(&Uart_1_PS, BaudRate); //setto il BaudRate = 115200
    if (Status_1 != (s32)XST_SUCCESS) {
      return XST_FAILURE;
    }
    int min = 255;
    int max = 0;
    receive_image(&min, &max);
    negative();


    cleanup_platform();
    return 0;
}




void receive_image(int* min_addr, int* max_addr){

	unsigned char my_pixel;
	for (int i=0; i<3*128*128+15; i++){
		my_pixel=XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
		if (my_pixel < *min_addr && i > 14) *min_addr = my_pixel;
		if (my_pixel > *max_addr && i > 14) *max_addr = my_pixel;
	    image_array[i]=my_pixel;
	}
}

void negative(){
	 for (int i=0; i<3*128*128+15; i++){
		if (i > 14) {
			unsigned char negated_pixel = 255-image_array[i];
			XUartPs_SendByte(XPAR_PS7_UART_1_BASEADDR, negated_pixel);
		} else {
			unsigned char header = image_array[i];
			XUartPs_SendByte(XPAR_PS7_UART_1_BASEADDR, header);
		}
	 }
}

