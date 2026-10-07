#pragma once

#include "manager.h"
#include "interface/ivoxelmanager.h"

class IExtendedSocket;
class CReceivePacket;
class CRoomSettings;

class CVoxelManager : public CBaseManager<IVoxelManager>
{
public:
	CVoxelManager();
	~CVoxelManager();

	virtual bool Init();
	virtual void Shutdown();

	bool LoadVoxelConfigList();
	std::vector<VoxelConfig> GetVoxelConfigList();
	bool OnPacket(CReceivePacket* msg, IExtendedSocket* socket);
	std::string GetSlotDetails(const std::string& slotId, int serverId);

private:
	std::vector<VoxelConfig> m_VoxelConfigList;
};

extern CVoxelManager g_VoxelManager;