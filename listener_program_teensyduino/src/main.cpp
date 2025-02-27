#include <Arduino.h>
#include <arduino_freertos.h>       // Teensy port for FreeRTOS found here: https://github.com/tsandmann/freertos-teensy

// put function declarations here:
static void DemoTask(void*);

void setup() {
  // Create the demo task
  xTaskCreate(DemoTask, "DemoTask", 128, nullptr, 2, nullptr);

  // Start scheduler and let it take over the execution from here on.
  vTaskStartScheduler();
}

void loop() {
  // This can remain empty
}

static void DemoTask(void*) {
  pinMode(arduino::LED_BUILTIN, arduino::OUTPUT);
  while (true) {
    digitalWriteFast(arduino::LED_BUILTIN, arduino::LOW);
    vTaskDelay(pdMS_TO_TICKS(500));

    digitalWriteFast(arduino::LED_BUILTIN, arduino::HIGH);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}