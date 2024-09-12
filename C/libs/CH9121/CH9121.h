#ifndef _CH9121_H_
#define _CH9121_H_

#include <stdlib.h> // malloc() free()
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

/// \tag::uart_advanced[]
#define UART_ID0 uart0
#define UART_ID1 uart1
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN0 0
#define UART_RX_PIN0 1
#define UART_TX_PIN1 4
#define UART_RX_PIN1 5

#define CFG_PIN 14
#define RES_PIN 17
#define GPIO_OUT 1
#define GPIO_IN 0

#define UCHAR unsigned char
#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t

#define TCP_SERVER 0
#define TCP_CLIENT 1
#define UDP_SERVER 2
#define UDP_CLIENT 3

#define Mode1 0x10               //Port 1: Setup Mode   0x00:TCP Server 0x01:TCP Client 0x02:UDP Server 0x03:UDP Client
#define LOCAL_IP 0x11            //Local IP
#define SUBNET_MASK 0x12         //Subnet Mask
#define GATEWAY 0x13             //Gateway
#define LOCAL_PORT1 0X14         //Port 1:Local Port
#define TARGET_IP1 0x15          //Port 1:Target IP
#define TARGET_PORT1 0x16        //Port 1:Target Port
#define PORT_RANDOM_ENABLE1 0x17 //Port 1:Port Random Enable
#define UART1_BAUD1 0x21         //Port 1:Baud rate of serial port 1

#define ON_OFF_UART2 0X39        //Port 2: Enable/disable port 2
#define Mode2 0x40               //Port 2: Setup Mode   0x00:TCP Server 0x01:TCP Client 0x02:UDP Server 0x03:UDP Client
#define LOCAL_PORT2 0X41         //Port 2:Local Port
#define TARGET_IP2 0x42          //Port 2:Target IP
#define TARGET_PORT2 0x43        //Port 2:Target Port
#define UART1_BAUD2 0x44         //Port 2:Baud rate of serial port 2
#define PORT_RANDOM_ENABLE2 0x17 //Port 2:Port Random Enable

void CH9121_init(void);

class CH9121{
private:
    UCHAR local_ip[4] = {169, 254, 88, 90};
    UWORD local_port  = 3000;
    UCHAR target_ip[4] = {255, 255, 255, 255}; // listen for everyone
    UWORD target_port = 8080; // port that client to access
    UDOUBLE baud_rate = 115200;
    UCHAR mode = 0x02; // 0x00:TCP Server 0x01:TCP Client 0x02:UDP Server 0x03:UDP Client

    // The following should be set properly based on the ethernet/wifi router
    UCHAR gateway[4] = {169, 254, 88, 1};
    UCHAR subnet_mask[4] = {255, 255, 255, 0};
public:
    void init_params();
};

#endif
