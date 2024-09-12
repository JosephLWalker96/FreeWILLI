#include <pico/stdlib.h>
//#include <iostream>
#include "CH9121.h"
#include "class/cdc/cdc_device.h"

int main(){
    stdio_init_all();
    CH9121 ethnet_device = CH9121();

    // Wait for USB connection
//    while (!tud_cdc_connected()) {
//        tight_loop_contents();
//    }
//
//    stdio_set_translate_crlf(&stdio_usb, true);
//    stdio_set_translate_crlf(&stdio_uart, false);

    ethnet_device.init_params();
    while (true){
        printf("Setup complete. \n");
    }

//    printf("Setup complete. Waiting for data...\n");
//    while (true){
//        if (uart_is_readable(UART_ID0)){
//            char c = uart_getc(UART_ID0);
//            printf("Received: %c\n", c);
//            if (uart_is_writable(UART_ID0))
//            {
//                uart_puts(UART_ID0, "Received!");
//                printf("Response Complete\n");
//            }
//        }
//    }
}