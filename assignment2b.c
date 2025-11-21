/*
    Assignment 2B: UART Part 2
    Edoardo Pilia - 70/91/00138
 */


/*
This code supports full arbitrary size of ppm images.
However, these images have to follow some specifications:
- Magic number should be always P6, since it is hard coded in this program
- Header's structure should follow this example: "P6\nWIDTH HEIGHT\nMAX_COLOR"; 
    other structures (like all spaces without newline or vice versa) can cause unexpected behaviours
*/

 #include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "xil_printf.h"
#include "xuartps.h"


int sizeof_numeral(int num, char* buf);
unsigned char* receive_image(int* min_addr, int* max_addr, int* len_final, int* width, int* height, int* len_header_);
void scale(int min, int max, int len_final, int width, int height, unsigned char* image_array, int len_header);


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
  Status_1 = XUartPs_CfgInitialize(&Uart_1_PS, Config_1, Config_1->BaseAddress); //initializing UART
  if (Status_1 != XST_SUCCESS) {
   return XST_FAILURE;
  }
  u32 BaudRate = (u32)115200;
  Status_1 = XUartPs_SetBaudRate(&Uart_1_PS, BaudRate);
  if (Status_1 != (s32)XST_SUCCESS) {
   return XST_FAILURE;
  }
  
  // Min (0) and max (255) are the min and max values for each pixel (color information in grayscale).
  // In this case, min is initialized as 255 and max as 0, because we use them to find the min and max
  // pixel value in the prototype image by comparing all pixels.
  int min = 255;
  int max = 0;
  int len_final = 0;
  int len_header = 0;
  int width=0;
  int height = 0;
  unsigned char* image_array = receive_image(&min, &max, &len_final, &width, &height, &len_header);
  scale(min, max, len_final, width, height, image_array, len_header);

  cleanup_platform();
  return 0;
}



unsigned char* receive_image(int* min_addr, int* max_addr, int* len_final, int* width, int* height, int* len_header_){

	
  unsigned char my_pixel = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
  int index = 0;         //index used to count header dimension
    
  int tmp; //temp var for convert ascii to dec

  int max_color_value=0;

  //receiving first two bytes of the header (P6) and "new line" byte
  //P6 is hard coded, so this bytes are being discarted
  while (my_pixel != 10) {
    //magic_number += my_pixel; //this didn't work, that's why it is hard coded
    index++;
    XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
  }

  //obtaining image width and space 
  index++;
  my_pixel = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR); //first byte of width
 
  while (my_pixel != 32) {         //while loop continues until we find a "space" byte
  	tmp = my_pixel - 48;
  	*width = *width*10 + tmp;
  	index ++;
  	my_pixel = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
  }

  
  //obtaining image height and newline
  index++;
  my_pixel = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR); //first byte of height
  
  while (my_pixel != 10) {      //while loop continues until we find a "new line" byte
  	tmp = my_pixel - 48;
  	*height = *height*10 + tmp;
  	index ++;
  	my_pixel = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
  }

  
  //receiving bytes of max color value and "new line" byte
  index++;
  my_pixel = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR); //first byte of max color value
  
  while (my_pixel != 10) {      //while loop continues until we find a "new line" byte
  	index ++;
  	tmp = my_pixel - 48;
  	max_color_value = max_color_value*10+tmp;
  	my_pixel = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
  }


  char* new_header_array = malloc(index);

  int len_header = sprintf(new_header_array, "P6\n%d %d\n%d\n",
              *width,
              *height,
              max_color_value);

  *len_header_ = len_header; //len_header (var) was used instead of len_header_ (ptr) to avoid ptr dereference in the rest of the code and possible errors

  int img_length = (*height)*(*width)*3; //allocating image array (w*h*3 -> color channels)
  
  unsigned char* image_array = malloc(img_length);

  //receiving image informations
  for (int j=len_header; j<img_length+len_header; j++){
    my_pixel = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
    if (my_pixel < *min_addr) *min_addr = my_pixel;
    if (my_pixel > *max_addr) *max_addr = my_pixel;
    image_array[j]=my_pixel;
  }

  unsigned char* final_array = malloc(len_header+img_length);
  memcpy(final_array, new_header_array, len_header);

  *len_final = img_length + len_header;

  free(new_header_array);
  free(image_array);

  return final_array;
}


// scale is the function which scales pixels values by stretching them between 0 and 255.
void scale(int min, int max, int len_final, int width, int height, unsigned char* image_array, int len_header){

	int scale = 255 / (max - min);
	for (int i=0; i<len_final; i++){
		if (i > len_header) {
			unsigned char scaled_pixel = (image_array[i] - min) * scale;
			XUartPs_SendByte(XPAR_PS7_UART_1_BASEADDR, scaled_pixel);
		} else {
		unsigned char header = image_array[i];
		XUartPs_SendByte(XPAR_PS7_UART_1_BASEADDR, header);
		}
	 }
}


