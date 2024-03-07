#include "net/socketshared.h"

/**
 * Listen thread for server/client
 * @param data Pointer to the listener object
 * @return NULL
 */
void* ListenThread(void* data)
{
	ISocketListenable* listener = static_cast<ISocketListenable*>(data);

	while (listener->IsRunning())
	{
		listener->Listen();
	}

	return 0;
}