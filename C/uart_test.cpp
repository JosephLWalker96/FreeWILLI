#include <iostream>
#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <hardware/gpio.h>

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

int main(){
    stdio_init_all();
    uart_init(UART_ID, BAUD_RATE);

    // Set up the UART pins
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    while (true) {
        // Wait for data to be available
        if (uart_is_readable(UART_ID)) {
            std::cout << "Waiting" << std::endl;
//            uart_read_blocking(UART_ID, buf, sizeof(buf));
            char c = uart_getc(UART_ID);
            // Print the received message
            std::cout << "Received: " << c << std::endl;
            // Send the message back to the Mac
//            uart_write_blocking(UART_ID, buf, sizeof(buf));
            uart_putc(UART_ID, c);
        }
//        else{
//            std::cout << "UART is not readable" << std::endl;
//        }
    }
    return 0;
}