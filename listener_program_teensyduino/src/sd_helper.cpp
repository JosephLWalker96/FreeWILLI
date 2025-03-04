#include "arduino_freertos.h"
#include "avr/pgmspace.h"

#include "SD.h"

#include "freewilli_sd.h"

void SDHelperTask(void*) {
  if (!SD.begin(BUILTIN_SDCARD)) {
      arduino::Serial.println("initialization failed!");
  } else {
      arduino::Serial.println("initialization done.");
  }

  File root;
  while (true) {
    root = SD.open("/");
    if (!root.isDirectory()) {
      arduino::Serial.println("open / failed!");
      if (!SD.begin(BUILTIN_SDCARD)) {
          arduino::Serial.println("initialization failed!");
          continue;
      }
      arduino::Serial.println("initialization done.");
      root = SD.open("/");
      if (!root.isDirectory()) {
          arduino::Serial.println("open / failed!");
          continue;
      }
    }

    while (true) {
      auto entry { root.openNextFile() };
      if (!entry) {
          break;
      }

      ::Serial.print(entry.name());
      if (!entry.isDirectory()) {
          arduino::Serial.print("\t\t");
          arduino::Serial.println(entry.size());
      } else {
          arduino::Serial.println();
      }

      entry.close();
    }
    root.close();
#ifdef SDFAT_BASE
    SD.sdfs.end();
#endif
    arduino::Serial.println("\n");
  }
}