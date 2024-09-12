#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/lwip_freertos.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lwip/sockets.h"
#include "unistd.h"

#define MAIN_TASK_PRIORITY     ( tskIDLE_PRIORITY + 3 )
#define LISTEN_TASK_PRIORITY   ( tskIDLE_PRIORITY + 2 )
#define BLINK_TASK_PRIORITY    ( tskIDLE_PRIORITY + 1 )
#define PING_TASK_PRIORITY     ( tskIDLE_PRIORITY + 1 )

void blink_task(__unused void *params){
    // Use FreeRTOS delay so that the OS can context switch to other tasks
    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(250));
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void listen_task(__unused void *params){
    // Socket setup code
    int udpSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    char buffer[128];

    // Create socket
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        printf("Error creating socket\n");
        vTaskDelete(NULL);
    }

    // Set server address and port
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9876); // Listening on port 9876
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket
    if (bind(udpSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Bind failed!\n");
        close(udpSocket);
        vTaskDelete(NULL);
    }

    // Print server IP address and port
    char server_ip[16];
    ip4addr_ntoa_r(netif_ip4_addr(&cyw43_state.netif[CYW43_ITF_STA]), server_ip, sizeof(server_ip));
    printf("Server IP Address: %s\n", server_ip);
    printf("Server Port: %d\n", ntohs(serverAddr.sin_port));
    printf("Listening\n");

    while (true) {
        ssize_t recvLen = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0,
                                   (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (recvLen > 0) {
            buffer[recvLen] = '\0';
            printf("Received: %s\n", buffer);
        } else {
            // Use FreeRTOS delay so that the OS can context switch to other tasks
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    // Cleanup resources if task ever exits
    close(udpSocket);
    vTaskDelete(NULL);
}

void ping_task(__unused void *params){
    // a near idle task just for being pinged
    while (true) {
        // Delay between pings
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void main_task(__unused void *params) {

    // Initialize the Wi-Fi driver
    if (cyw43_arch_init()) {
        while (true)
            printf("Failed to initialize Wi-Fi driver\n");
    }

    cyw43_arch_enable_sta_mode();

    const char* ssid = "SpectrumSetup-D8";
    const char* password = "urbanfarmer157";
    while (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        printf("try again. \n");
    }
    printf("Connected to Wi-Fi!\n");

    int link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    printf("Link status: %d\n", link_status);

    BaseType_t result;
    TaskHandle_t blinkTaskHandle, listenTaskHandle, pingTaskHandle;

    // Create Blink Task
    result = xTaskCreate(blink_task, "BlinkTask", 1024, NULL, BLINK_TASK_PRIORITY, &blinkTaskHandle);
    if (result != pdPASS) {
        printf("Error creating BlinkTask: %ld\n", result);
        for(;;); // Loop forever to halt the system
    }

    // Create Listen Task
    result = xTaskCreate(listen_task, "ListenTask", 2048, NULL, LISTEN_TASK_PRIORITY, &listenTaskHandle);
    if (result != pdPASS) {
        printf("Error creating ListenTask: %ld\n", result);
        for(;;); // Loop forever to halt the system
    }

    // Create Ping Task
    result = xTaskCreate(ping_task, "PingTask", 1024, NULL, PING_TASK_PRIORITY, &pingTaskHandle);
    if (result != pdPASS) {
        printf("Error creating PingTask: %ld\n", result);
        for(;;); // Loop forever to halt the system
    }

    // Delete main_task if it's no longer needed
    vTaskDelete(NULL);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    // Handle stack overflow (e.g., log error, reset system)
    printf("Stack overflow in task: %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for(;;); // Loop forever to halt the system
}

void  vApplicationMallocFailedHook(){
    printf("Malloc Failed\n");
    taskDISABLE_INTERRUPTS();
    for(;;); // Loop forever to halt the system
}

int main(){
    stdio_init_all();
    TaskHandle_t task;
    auto main_rslt = xTaskCreate(
            main_task,
            "TestMainThread",
            configMINIMAL_STACK_SIZE,
            NULL,
            MAIN_TASK_PRIORITY,
            &task
    );
    if (main_rslt != pdPASS) {
        printf("Error creating main task: %d\n", main_rslt);
    } else {
        printf("main task created successfully\n");
    }

    vTaskStartScheduler();
}
