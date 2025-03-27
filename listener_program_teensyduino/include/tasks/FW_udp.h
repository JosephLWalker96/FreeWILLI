#ifndef FREEWILLI_UDP_H
#define FREEWILLI_UDP_H

#include <stdint.h>

#define UDP_ERR_CONNECT_FAILED      (-1)
#define UDP_OK                      0

// int32_t InitUDPServer(
//   const IPAddress staticIp,
//   const IPAddress subnetMask,
//   const IPAddress gateway,
//   const uint16_t udpPort);

int32_t InitUDPServer(void);

void UDPServerTask(void*);

#endif /* !FREEWILLI_UDP_H */
