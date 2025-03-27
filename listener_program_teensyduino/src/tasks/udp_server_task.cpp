/**
 * @file udp_server_task.cpp
 * @brief This file implements the UDP server required to receive data packets
 * and pass them over to the DSP Pipeline. This is the equivalent of
 * "Producer Thread" as used in the RPi implementation.
 */

#include <Arduino.h>
#include <arduino_freertos.h>
#include "QNEthernet.h"
#include "tasks/FW_udp.h"
#include <stdint.h>

using namespace qindesign::network;

// UDP port.
static EthernetUDP udp;

// Set the static IP to something other than INADDR_NONE (all zeros)
// to not use DHCP. The values here are just examples.
static IPAddress staticIP{192, 168, 1, 101};
static IPAddress subnetMask{255, 255, 255, 0};
static IPAddress gateway{192, 168, 1, 1};
static uint16_t udpPort = 9600;

// The link timeout, in milliseconds. Set to zero to not wait and
// instead rely on the listener to inform us of a link.
constexpr uint32_t kLinkTimeout = 5'000;  // 5 seconds

FLASHMEM int32_t InitUDPServer(void)
{
  uint8_t mac[6];

  // QNEthernet uses the Teensy's internal MAC address by default, so we can
  // retrieve it here
  Ethernet.macAddress(mac);
  arduino::Serial.printf("MAC = %02x:%02x:%02x:%02x:%02x:%02x\r\n",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // Add listeners
  // It's important to add these before doing anything with Ethernet
  // so no events are missed.

  // Listen for link changes
  Ethernet.onLinkState([](bool state) {
    arduino::Serial.printf("[Ethernet] Link %s\r\n", state ? "ON" : "OFF");
  });

  // Using the static IP accepted as params
  arduino::Serial.printf("Starting Ethernet with static IP...\r\n");
  if (!Ethernet.begin(staticIP, subnetMask, gateway)) {
    arduino::Serial.printf("Failed to start Ethernet\r\n");
    return UDP_ERR_CONNECT_FAILED;
  }

  // When setting a static IP, the address is changed immediately,
  // but the link may not be up; optionally wait for the link here
  // if (kLinkTimeout > 0) {
  //   printf("Waiting for link...\r\n");
  //   if (!Ethernet.waitForLink(kLinkTimeout)) {
  //     printf("No link yet\r\n");
  //     // We may still see a link later, after the timeout, so
  //     // continue instead of returning
  //   }
  // }

  return UDP_OK;
}

// Control character names.
static const char *kCtrlNames[]{
  "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
  "BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "SO",  "SI",
  "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
  "CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US",
};

FLASHMEM void UDPServerTask(void*)
{
  IPAddress ip;

  arduino::Serial.println("Executing UDP Server Task.");

  // Start UDP listening on the port
  udp.begin(udpPort);

  // YJ// Surround with try/catch
  while(true) {
    // YJ// This only gets one packet at a time.
    // Update this to aggregate 8 packets and pass them over to the "Consumer thread" somehow
    int size = udp.parsePacket();
    if (size < 0) {
      vTaskDelay(pdTICKS_TO_MS(1));   // YJ// Set timeout accordingly
      continue;
    }

    // Get the packet data and remote address
    const uint8_t *data = udp.data();
    ip = udp.remoteIP();

    arduino::Serial.printf("[%u.%u.%u.%u][%d] ", ip[0], ip[1], ip[2], ip[3], size);

    // Print each character
    for (int i = 0; i < size; i++) {
      // uint8_t b = data[i];
      // if (b < 0x20) {
      //   arduino::Serial.printf("<%s>", kCtrlNames[b]);
      // } else if (b < 0x7f) {
      //   putchar(data[i]);
      // } else {
      //   arduino::Serial.printf("<%02xh>", data[i]);
      // }
      arduino::Serial.printf("%c", data[i]);
    }
    arduino::Serial.printf("\r\n");

    // YJ// Print optional statistics
  }
}