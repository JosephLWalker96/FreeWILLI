
/*

This file contains all function prototypes for listener_thread.cpp

*/

#pragma once

//#include "custom_types.h"
class Session;
class SocketManager;
void UdpListener(Session &sess, SocketManager &socketManager, const int PACKET_SIZE);