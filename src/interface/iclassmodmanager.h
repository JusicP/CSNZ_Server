#pragma once

#include "imanager.h"

class IUser;

struct ClassModInfo_t;

class IClassModManager : public IBaseManager
{
public:
	virtual void LoadClassMod() = 0;

	virtual	bool HasClassMod(IUser* user, int slot) = 0;
	virtual	ClassModInfo_t GetClassModBySlot(IUser* user, int slot) = 0;
	virtual ClassModInfo_t GetClassModPresetById(int itemId) = 0;

	virtual bool EnableSlot(IUser* user, int itemslot, int category, int& enabledslot, ClassModInfo_t& info) = 0;
	virtual bool ApplyMod(IUser* user, int itemslot, int category, int slot, int modItem, ClassModInfo_t& info) = 0;
	virtual bool RemoveMod(IUser* use, int itemslotr, int category, int slot, ClassModInfo_t& info) = 0;
	virtual bool InterchangeMod(IUser* user, int itemslot, int category, int oldslot, int newslot, ClassModInfo_t& info) = 0;
	virtual bool ChangeStats(IUser* user, int itemslot, ClassModInfo_t& info) = 0;

	virtual void SaveClassModInfo(IUser* user, int slot, const ClassModInfo_t& info) = 0;
};