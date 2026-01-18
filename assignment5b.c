/*
    Assignment 5: DNN on MNIST
    Edoardo Pilia - 70/91/00138
 */


/*
This code supports only P6 28x28 images, with an header of 15 bytes.
*/

#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "xil_printf.h"
#include "xuartps.h"
#include "test_images.h"
#include <xtime_l.h>
#include <time.h>

#include <math.h>

#include "weights.h"

#define n_bias0 128
#define n_weights0 100352
#define n_bias1 64
#define n_weights1 8192
#define n_bias2 32
#define n_weights2 2048
#define n_bias3 10
#define n_weights3 320

typedef short int DATA;


DATA gemm0_bias[n_bias0] = {bias0};
DATA gemm0_weights[n_weights0] = {weights0} ;
DATA gemm1_bias[n_bias1] = {bias1};
DATA gemm1_weights[n_weights1] = {weights1};
DATA gemm2_bias[n_bias2] = {bias2};
DATA gemm2_weights[n_weights2] = {weights2};
DATA gemm3_bias[n_bias3] = {bias3};
DATA gemm3_weights[n_weights3] = {weights3};

#define FIXED2FLOAT(a, qf) (((float) (a)) / (1<<qf))
#define FLOAT2FIXED(a, qf) ((short int) round((a) * (1<<qf)))

#define _MAX_ (1 << (sizeof(DATA)*8-1))-1
#define _MIN_ -(_MAX_+1)

// DNN functions to compose your network

void FC_forward(DATA* input, DATA* output, int in_s, int out_s, DATA* weights, DATA* bias, int qf) ;
static inline long long int saturate(long long int mac);
static inline void relu_forward(DATA* input, DATA* output, int size);
int resultsProcessing(DATA* results, int size);

// implement your function receiving from UART
// DATA readfromUART(){ // reads a sequence of bytes and composes the DATA
// }

//DATA image_array[10][28*28] = {{imm_test_9},{imm_test_8},{imm_test_7},{imm_test_6},{imm_test_5},{imm_test_4},{imm_test_3},{imm_test_2},{imm_test_1},{imm_test_0}};

DATA image_array[28*28];

u32 t_start, receiving_time, classifying_time, t_end;

int main(){
  init_platform();

  //UART setup
  XUartPs Uart_1_PS;
  u16 DeviceId_1= XPAR_PS7_UART_1_DEVICE_ID;
  int Status_1;
  XUartPs_Config *Config_1;
  Config_1 = XUartPs_LookupConfig(DeviceId_1);
  if (NULL == Config_1) {
    return XST_FAILURE;
  }
  /*the default configuration is stored in Config and it can be used to initialize the controller */
  Status_1 = XUartPs_CfgInitialize(&Uart_1_PS, Config_1, Config_1->BaseAddress);
  if (Status_1 != XST_SUCCESS) {
    return XST_FAILURE;
  }
  // Set the BAUD rate
  u32 BaudRate = (u32)115200;
  Status_1 = XUartPs_SetBaudRate(&Uart_1_PS, BaudRate);
  if (Status_1 != (s32)XST_SUCCESS) {
    return XST_FAILURE;
  }
  //END UART SETUP
  xil_printf ("Started\r\n");


  while (1){
    
      //timer start
	  t_start = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);

	  receive_image();

	  receiving_time = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
	  u32 diff_ticks = receiving_time - t_start;
	  u32 duration_us = diff_ticks / (COUNTS_PER_SECOND / 1000 / 1000);

	  xil_printf("Receiving Duration [us]: %u\r\n", duration_us);

	  xil_printf("The image shows the number %i\r\n", classify_image());

	  t_end = Xil_In32(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET);
	  diff_ticks = t_end - receiving_time;
	  duration_us = diff_ticks / (COUNTS_PER_SECOND / 1000 / 1000);

	  xil_printf("Classifying Duration [us]: %u\r\n", duration_us);

	  diff_ticks = t_end - t_start;
	  duration_us = diff_ticks / (COUNTS_PER_SECOND / 1000 / 1000);

	  xil_printf("Total Duration [us]: %u\r\n", duration_us);
  }


  cleanup_platform();
  return 0;
}


void FC_forward(DATA* input, DATA* output, int in_s, int out_s, DATA* weights, DATA* bias, int qf) {
	// NOTE return W * x
	int hkern = 0;
	int wkern = 0;
	long long int mac = 0;
	DATA current = 0;

	/* foreach row in kernel */
	//	#pragma omp parallel for private (hkern, wkern, mac, current)
	for (hkern = 0; hkern < out_s; hkern++) {
		mac = ((long long int)bias[hkern]) << qf;
		for (wkern = 0; wkern < in_s; wkern++) {
			current = input[wkern];
			mac += current * weights[hkern*in_s + wkern];
		}
		// output[hkern] = (DATA)(mac >> qf);
		output[hkern] = (DATA)saturate(mac >> qf);
	}
}

static inline long long int saturate(long long int mac)
{

	if(mac > _MAX_) {
		printf("[WARNING] Saturation.mac: %lld -> %llx _MAX_: %d  _MIN_: %d  res: %d\r\n",  mac, mac, _MAX_, _MIN_, _MAX_);
		return _MAX_;
	}

	if(mac < _MIN_){
		printf( "[WARNING] Saturation. mac: %lld -> %llx _MAX_: %d  _MIN_: %d  res: %d\r\n",  mac, mac, _MAX_, _MIN_, _MIN_);
		return _MIN_;
	}

	//printf("mac: %lld -> %llx _MAX_: %lld  _MIN_: %lld  res: %lld\r\n", mac, mac, _MAX_, _MIN_, mac);
    return mac;

}

static inline void relu_forward(DATA* input, DATA* output, int size) {
	int i = 0;
	for(i = 0; i < size; i++) {
		DATA v = input[i];
		v = v > 0 ? v : 0;
		output[i] = v;
	}
}

#define SIZEWA 10
int resultsProcessing(DATA* results, int size){
//What do you want to do with the results of the CNN? Here is the place where you should put the classifier or the detection (see YOLO detection for example)
//The simplest classifier is a maximum search for the results which returns the index value of the maximum

 char *labels[10]={"digit 0", "digit 1", "digit 2", "digit 3", "digit 4", "digit 5", "digit 6", "digit 7", "digit 8", "digit 9"};

// TODO: check the size parameter
  int size_wa = SIZEWA;
  float  r[SIZEWA];
  int  c[SIZEWA];
  float results_float[SIZEWA];
  float sum=0.0;
  DATA max=0;
  int max_i;
  for (int i =0;i<size_wa;i++){
      results_float[i] = FIXED2FLOAT(results[i],8);
    int n;
    if (results[i]>0)
      n=results[i];
    else
      n=-results[i];
    if (n>max){
      max=n;
      max_i=i;
    }
  }
  for (int i =0;i<size_wa;i++)
    sum+=exp(results_float[i]);

  for (int i =0;i<size_wa;i++){
    r[i]=exp(results_float[i]) / sum;
    c[i]=i;
  }
  for (int i =0;i<size_wa;i++){
    for (int j =i;j<size_wa;j++){
      if (r[j]>r[i]){
        float t= r[j];
        r[j]=r[i];
        r[i]=t;
        int tc= c[j];
        c[j]=c[i];
        c[i]=tc;
      }
    }
  }
  int top0=0;
  float topval=results_float[0];
  for (int i =1;i<size_wa;i++){
    if (results_float[i]>topval){
      top0=i;
      topval=results_float[i];
    }
  }
  //xil_printf("\r\n\r\n");
  for (int i =0;i<5;i++){
//	  xil_printf("            TOP %d: [%d] %s   \r\n",i, c[i], labels[c[i]]);
  }
  // xil_printf("max= %x \r\n",top0);
  return top0;
}

void receive_image(){
  //Optimization
  //This will work only if the image is converted to do not have the MSB (00) or if the sending script is adapted
  //to send only the LSB
	for (int i=0; i<28*28; i++){
		u8 data1 = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR); 
		u8 data2 = 0x00;
		DATA data_16 = (data2<<8) + data1;
		image_array[i] = data_16;
	}
}

int classify_image(){
	xil_printf("Classification started...\r\n");

	DATA output_gemm0[128];
	DATA input_gemm1[128];
	DATA output_gemm1[64];
	DATA input_gemm2[64];
	DATA output_gemm2[32];
	DATA input_gemm3[32];
	DATA output_gemm3[10];

    //DNN chain
	FC_forward(image_array, output_gemm0, 784, 128, gemm0_weights, gemm0_bias, 8);

	relu_forward(output_gemm0, input_gemm1, 128);

	FC_forward(input_gemm1, output_gemm1, 128, 64, gemm1_weights, gemm1_bias, 8);

	relu_forward(output_gemm1, input_gemm2, 64);

	FC_forward(input_gemm2, output_gemm2, 64, 32, gemm2_weights, gemm2_bias, 8);

	relu_forward(output_gemm2, input_gemm3, 32);

    xil_printf("Classification ended\r\n");

	return resultsProcessing(output_gemm3, 10);
}
