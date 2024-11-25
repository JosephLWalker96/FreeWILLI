#pragma once

class Session;
class SocketManager;

void udpListener(Session &sess, SocketManager &socketManager, const int PACKET_SIZE);