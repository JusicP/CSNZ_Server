#pragma once

#include "net/net.h"

void* ListenThread(void* data);

class ISocketListenable
{
public:
    virtual void Listen() = 0;
    virtual bool IsRunning() = 0;
};