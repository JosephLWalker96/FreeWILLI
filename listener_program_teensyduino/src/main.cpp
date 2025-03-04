#include <Arduino.h>
#include <arduino_freertos.h>       // Teensy port for FreeRTOS found here: https://github.com/tsandmann/freertos-teensy
#include <stdint.h>

#include <freewilli_udp.h>
#include <freewilli_sd.h>

// put function declarations here:
static void DemoTask(void*);

TaskHandle_t udpServerTaskHandle_g;
TaskHandle_t sdHelperTaskHandle_g;

void setup() {
  int8_t rv = UDP_OK;

  Serial.begin(115200);
  pinMode(arduino::LED_BUILTIN, arduino::OUTPUT);

  Serial.println("Starting UDP Init!");

  rv = InitUDPServer();
  if (rv != UDP_OK) {
    Serial.printf("UDP Server Init failed! Error %d\n", rv);
    while (true) {
      // Serial.println("Loop");
      digitalWriteFast(arduino::LED_BUILTIN, arduino::LOW);
      vTaskDelay(pdMS_TO_TICKS(100));

      digitalWriteFast(arduino::LED_BUILTIN, arduino::HIGH);
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  } else {
    Serial.println("UDP Server Initialized successfully!");
  }

  // Create the demo task
  xTaskCreate(DemoTask, "DemoTask", 128, nullptr, 2, nullptr);

  // Create the UDP task
  xTaskCreate(UDPServerTask, "UDPServerTask", 4096, nullptr, 1, &udpServerTaskHandle_g);

  // Create the SD Helper task
  // xTaskCreate(SDHelperTask, "SDHelperTask", 512, nullptr, 1, &sdHelperTaskHandle_g);

  // Start scheduler and let it take over the execution from here on.
  vTaskStartScheduler();
}

void loop() {
  // This can remain empty
}

static void DemoTask(void*) {
  while (true) {
    // Serial.println("Loop");
    digitalWriteFast(arduino::LED_BUILTIN, arduino::LOW);
    vTaskDelay(pdMS_TO_TICKS(500));

    digitalWriteFast(arduino::LED_BUILTIN, arduino::HIGH);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}