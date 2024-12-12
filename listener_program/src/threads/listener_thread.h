#pragma once

class Session;
class ISocketManager;

void udpListener(Session &sess, ISocketManager *socketManager, const int PACKET_SIZE);