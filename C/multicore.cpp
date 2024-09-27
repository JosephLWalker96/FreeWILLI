#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/cyw43_arch.h"

const int taskDelay = 100;
const int taskSize = 1024;  // Increased stack size for Wi-Fi init task

SemaphoreHandle_t mutex;             // For safe print operations
SemaphoreHandle_t wifiInitSemaphore; // For signaling Wi-Fi init completion

// Function for safe printing to avoid race conditions
void vSafePrint(const char *out) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    puts(out);
    xSemaphoreGive(mutex);
}

// Wi-Fi initialization task
void vWifiInitTask(void *pvParameters) {
    // Initialize the Wi-Fi driver on Core 0
    if (cyw43_arch_init()) {
        vSafePrint("Failed to initialize Wi-Fi driver\n");
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));  // Halt the task if init fails
        }
    } else {
        vSafePrint("Wi-Fi driver initialized successfully\n");
    }

    // Signal that Wi-Fi initialization is complete
    xSemaphoreGive(wifiInitSemaphore);
    vTaskDelete(NULL);  // Delete the task after initialization
}

// SMP task for Wi-Fi operations after initialization
void vTaskSMP(void *pvParameters) {
    // Wait for Wi-Fi initialization to complete
    if (xSemaphoreTake(wifiInitSemaphore, portMAX_DELAY) == pdTRUE) {
        vSafePrint("Wi-Fi initialization complete. Starting Wi-Fi operations...\n");

        while (true) {
            // Lock access to Wi-Fi functions
            // You can perform Wi-Fi operations here, e.g., connect to a network
            printf("Performing Wi-Fi operations...\n");
            vTaskDelay(taskDelay);
        }
    } else {
        vSafePrint("Failed to acquire Wi-Fi init semaphore\n");
    }
}

// FreeRTOS stack overflow hook
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("Stack overflow in task: %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

// FreeRTOS malloc failure hook
void vApplicationMallocFailedHook() {
    printf("Malloc Failed\n");
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

int main() {
    stdio_init_all();

    // Create semaphores for safe print and Wi-Fi initialization signaling
    mutex = xSemaphoreCreateMutex();
    wifiInitSemaphore = xSemaphoreCreateBinary();

    // Create the Wi-Fi initialization task on Core 0
    TaskHandle_t wifiInitHandle;
    xTaskCreate(vWifiInitTask, "WiFi Init Task", taskSize, NULL, 2, &wifiInitHandle);
    vTaskCoreAffinitySet(wifiInitHandle, (1 << 0));

    // Create a task for SMP operations on Core 1
    TaskHandle_t handleA;
    xTaskCreate(vTaskSMP, "SMP Task", taskSize, NULL, 1, &handleA);
    vTaskCoreAffinitySet(handleA, (1 << 1));

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    // Should never reach here
    while (true) {}
    return 0;
}
