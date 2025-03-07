#pragma once

class SharedDataManager;
class ISocketManager;

void runListenerLoop(SharedDataManager& sess, ISocketManager* socketManager);