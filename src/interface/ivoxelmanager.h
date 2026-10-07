#pragma once

#include "imanager.h"

struct VoxelConfig;

class IVoxelManager : public IBaseManager
{
public:
	virtual bool LoadVoxelConfigList() = 0;
	virtual std::vector<VoxelConfig> GetVoxelConfigList() = 0;
	virtual bool OnPacket(CReceivePacket* msg, IExtendedSocket* socket) = 0;
	virtual std::string GetSlotDetails(const std::string& slotId, int serverId) = 0;
};