#pragma once

#include "interface/iclassmodmanager.h"
#include "usermanager.h"
#include "manager.h"

class CClassModManager : public CBaseManager<IClassModManager>
{
public:
	CClassModManager();
	~CClassModManager();

	virtual bool Init();
	virtual void Shutdown();

	void LoadClassMod();
	bool HasClassMod(IUser* user, int slot);
	ClassModInfo_t GetClassModBySlot(IUser* user, int slot);
	ClassModInfo_t GetClassModPresetById(int itemId);

	bool EnableSlot(IUser* user, int itemslot, int category, int& enabledslot, ClassModInfo_t& info);
	bool ApplyMod(IUser* user, int itemslot, int category, int slot, int modItem, ClassModInfo_t& info);
	bool RemoveMod(IUser* user, int itemslot, int category, int slot, ClassModInfo_t& info);
	bool InterchangeMod(IUser* user, int itemslot, int category, int oldslot, int newslot, ClassModInfo_t& info);
	bool ChangeStats(IUser* user, int itemslot, ClassModInfo_t& info);

private:
	CCSVTable* m_pClassStatusTable;
	CCSVTable* m_pClassModPresetTable;
	CCSVTable* m_pClassModItemsTable;

	ClassModConfig* m_ClassModConfig;

	std::unordered_map<unsigned short, ClassModInfo_t> m_Presets;

	void SaveClassModInfo(IUser* user, int slot, const ClassModInfo_t& info);
};

extern CClassModManager g_ClassModManager;