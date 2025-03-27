// For TeensyDebug
#pragma GCC optimize ("O0")

#include <Arduino.h>
#include <arduino_freertos.h>       // Teensy port for FreeRTOS found here: https://github.com/tsandmann/freertos-teensy
#include <stdint.h>

#include <TeensyDebug.h>

#include <tasks/FW_udp.h>
#include <tasks/FW_sd.h>
#include <tasks/FW_pipeline.h>

// put function declarations here:
static void DemoTask(void*);
// static void PipelineTaskDispatcher(void*);

static Pipeline pipeline_g;

TaskHandle_t udpServerTaskHandle_g;
TaskHandle_t pipelineTaskHandle_g;
TaskHandle_t sdHelperTaskHandle_g;

void setup() {
  int8_t rv = UDP_OK;

  Serial.begin(115200);
  debug.begin(SerialUSB1);

  halt_cpu();

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

  // Initialize Pipeline
  auto [socketVariables, pipelineVars] = parseJsonConfig();
  FW_InitPipeline(pipelineVars);
  // We need to destruct the pipeline object and reconstruct it with the new pipeline variables
  // YJ// This is a work-around. Fix this later by initializing the pipeline without the constructor.
  // pipeline_g.~Pipeline();
  // new (&pipeline_g) Pipeline(pipelineVars);

  // Create the demo task
  xTaskCreate(DemoTask, "DemoTask", 128, nullptr, 2, nullptr);

  // Create the UDP task
  xTaskCreate(UDPServerTask, "UDPServerTask", 1500, nullptr, 1, &udpServerTaskHandle_g);

  // Create the Pipeline task
  // xTaskCreate(PipelineTask, "PipelineTask", 10240, &pipeline_g, 3, &pipelineTaskHandle_g);

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
    Serial.print(".");
    digitalWriteFast(arduino::LED_BUILTIN, arduino::LOW);
    vTaskDelay(pdMS_TO_TICKS(500));

    digitalWriteFast(arduino::LED_BUILTIN, arduino::HIGH);
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// static void PipelineTaskDispatcher(void* arg)
// {
//   Pipeline* pipeline = (Pipeline*) arg;
//   pipeline->PipelineTask();
// }