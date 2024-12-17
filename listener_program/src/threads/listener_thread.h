#pragma once

class SharedDataManager;
class ISocketManager;

void udpListener(SharedDataManager &sess, ISocketManager *socketManager, const int &PACKET_SIZE);