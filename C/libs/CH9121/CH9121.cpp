#include "CH9121.h"

// command header for network setup
UCHAR tx[8] = {0x57, 0xAB};
void uart_gpio_init(void)
{

    uart_init(UART_ID0, BAUD_RATE);
    uart_init(UART_ID1, BAUD_RATE);
    gpio_set_function(UART_TX_PIN0, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN0, GPIO_FUNC_UART);
    gpio_set_function(UART_TX_PIN1, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN1, GPIO_FUNC_UART);

    gpio_init(CFG_PIN);
    gpio_init(RES_PIN);
    gpio_set_dir(CFG_PIN, GPIO_OUT);
    gpio_set_dir(RES_PIN, GPIO_OUT);
    gpio_put(CFG_PIN, 1);
    gpio_put(RES_PIN, 1);
}

void resetTX(){
    for (int i=2; i<8; i++)
        tx[i] = 0;
}

void waitReadable(){
    while (!uart_is_readable(UART_ID0)){
        tight_loop_contents();
    }
}

void enterSetUpMode(){
    UCHAR c = 0;
    while (true){
        uart_putc(UART_ID0, 0x55);
        uart_putc(UART_ID0, 0xaa);
        uart_putc(UART_ID0, 0x5a);
        printf("Sending setup mode init command. \n");

//        waitReadable();
        c = uart_getc(UART_ID0);
        printf("Getting %02X from setup command. \n", c);
        if (c == 0xa5){
            uart_putc(UART_ID0, c);
//            waitReadable();
            c = uart_getc(UART_ID0);
            printf("Getting %02X from confirm command. \n", c);
            if (c == 0xa5) {
                printf("Entering CH9121 setup mode. \n");
                return;
            } else{
                printf("Failed to enter CH9121 setup mode. \n");
            }
        } else{
            printf("Failed to enter CH9121 setup mode. \n");
        }
    }
    printf("Entering CH9121 setup mode. \n");
}

void CH9121::init_params() {
    printf("Initializing CH9121 network settings. \n");

    uart_gpio_init();

    // entering CH9121 setup mode
    enterSetUpMode();

    // set up network mode (UDP or TCP)
    tx[2] = static_cast<UCHAR>((this->mode >> 8) & 0xFF); // upper byte
    tx[3] = static_cast<UCHAR>(this->mode & 0xFF); // lower byte
    sleep_ms(10);
    for (int o = 0; o < 4; o++)
        uart_putc(UART_ID0, tx[o]);
    sleep_ms(10);
    resetTX();
    printf("Set up UDP.\n");

    // set up local IP
    for (int i=2; i<6; i++)
        tx[i] = this->local_ip[i-2];
    sleep_ms(10);
    for (int o = 0; o < 6; o++)
        uart_putc(UART_ID0, tx[o]);
    sleep_ms(10);
    resetTX();
    printf("Local IP setup Complete.\n");

    // set up local port
    tx[2] = static_cast<UCHAR>((this->local_port >> 8) & 0xFF); // upper byte
    tx[3] = static_cast<UCHAR>(this->local_port & 0xFF); // lower byte
    sleep_ms(10);
    for (int o = 0; o < 4; o++)
        uart_putc(UART_ID0, tx[o]);
    sleep_ms(10);
    resetTX();
    printf("Local Port setup Complete.\n");

    // set up target IP
    for (int i=2; i<6; i++)
        tx[i] = this->target_ip[i-2];
    sleep_ms(10);
    for (int o = 0; o < 6; o++)
        uart_putc(UART_ID0, tx[o]);
    sleep_ms(10);
    resetTX();
    printf("Target IP setup Complete.\n");

    // set up local port
    tx[2] = static_cast<UCHAR>((this->target_port >> 8) & 0xFF); // upper byte
    tx[3] = static_cast<UCHAR>(this->target_port & 0xFF); // lower byte
    sleep_ms(10);
    for (int o = 0; o < 4; o++)
        uart_putc(UART_ID0, tx[o]);
    sleep_ms(10);
    resetTX();
    printf("Target Port setup Complete.\n");

    // set up baud rate
    tx[2] = static_cast<UCHAR>((this->baud_rate >> 24) & 0xFF); // highest byte
    tx[3] = static_cast<UCHAR>((this->baud_rate >> 16) & 0xFF); // high byte
    tx[4] = static_cast<UCHAR>((this->baud_rate >> 8) & 0xFF);  // low byte
    tx[5] = static_cast<UCHAR>(this->baud_rate & 0xFF);         // lowest byte
    sleep_ms(10);
    for (int o = 0; o < 6; o++)
        uart_putc(UART_ID0, tx[o]);
    sleep_ms(10);
    resetTX();
    printf("Baud Rate setup Complete.\n");

    // set up gateway
    for (int i=2; i<6; i++)
        tx[i] = this->gateway[i-2];
    sleep_ms(10);
    for (int o = 0; o < 6; o++)
        uart_putc(UART_ID0, tx[o]);
    sleep_ms(10);
    resetTX();
    printf("Gateway setup Complete.\n");

    // set up subnet mask
    for (int i=2; i<6; i++)
        tx[i] = this->subnet_mask[i-2];
    sleep_ms(10);
    for (int o = 0; o < 6; o++)
        uart_putc(UART_ID0, tx[o]);
    sleep_ms(10);
    resetTX();
    printf("Subnet Mask setup Complete.\n");

    // update params
    tx[2] = 0x0d;
    for (int o=0; 0<3; o++)
        uart_putc(UART_ID0, tx[o]);
    sleep_ms(200);
    resetTX();
    printf("Updated Params.\n");

    // execute with new params
    tx[2] = 0x0e;
    for (int o=0; 0<3; o++)
        uart_putc(UART_ID0, tx[o]);
    sleep_ms(200);
    resetTX();
    printf("Recovered CH9121.\n");

    // exit
    tx[2] = 0x5e;
    for (int o=0; 0<3; o++)
        uart_putc(UART_ID0, tx[o]);
    sleep_ms(200);
    resetTX();
    printf("Exit set-up.\n");
};