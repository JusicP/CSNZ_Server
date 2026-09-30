#include "classmodmanager.h"
#include "itemmanager.h"
#include "csvtable.h"
#include "packetmanager.h"
#include "userdatabase.h"

#include "nlohmann/json.hpp"
#include "keyvalues.hpp"

#include "common/utils.h"

using namespace std;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

CClassModManager g_ClassModManager;

CClassModManager::CClassModManager() : CBaseManager("ClassModManager")
{
	m_pClassStatusTable = NULL;
	m_pClassModPresetTable = NULL;
	m_pClassModItemsTable = NULL;
	m_ClassModConfig = NULL;
}

CClassModManager::~CClassModManager()
{
}

bool CClassModManager::Init()
{
	LoadClassMod();

	m_pClassStatusTable = new CCSVTable("ZBS_class.csv", rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true), rapidcsv::LineReaderParams());
	m_pClassModPresetTable = new CCSVTable("ClassModPreset.csv", rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true), rapidcsv::LineReaderParams());
	m_pClassModItemsTable = new CCSVTable("ClassMod.csv", rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true), rapidcsv::LineReaderParams());

	if (m_pClassStatusTable->IsLoadFailed() || m_pClassModPresetTable->IsLoadFailed() || m_pClassModItemsTable->IsLoadFailed())
	{
		Logger().Fatal("CClassModManager::Init(): couldn't load some csv files. Required csv:\nZBS_class.csv\nClassModPreset.csv\nClassMod.csv\n");
		return false;
	}

	return true;
}

void CClassModManager::Shutdown()
{
	CBaseManager::Shutdown();

	delete m_pClassStatusTable;
	delete m_pClassModPresetTable;
	delete m_pClassModItemsTable;
}

void CClassModManager::LoadClassMod()
{
	try
	{
		ifstream f("ClassModConfig.json");
		ordered_json jClassModConfig = ordered_json::parse(f, nullptr, false, true);

		if (jClassModConfig.is_discarded() || !jClassModConfig.is_object())
		{
			Logger().Error("CClassModManager::LoadClassMod: couldn't load ClassModConfig.json.\n");
			return;
		}

		ClassModConfig* classModConfig = new ClassModConfig();

		classModConfig->changestatus.itemid = jClassModConfig["ClassMod_Item_Changes"]["id"];
		classModConfig->changestatus.cost = jClassModConfig["ClassMod_Item_Changes"]["cost"];

		classModConfig->addslot.itemid = jClassModConfig["ClassMod_Item_AddSlot"]["id"];
		classModConfig->addslot.costs = jClassModConfig["ClassMod_Item_AddSlot"]["costs"].get<std::vector<int>>();

		classModConfig->protect.itemid = jClassModConfig["ClassMod_Item_Protect"]["id"];
		classModConfig->protect.cost = jClassModConfig["ClassMod_Item_Protect"]["cost"];

		classModConfig->categorymaxslots = jClassModConfig["CategoryMaxSlots"].get<std::vector<int>>();

		m_ClassModConfig = classModConfig;
	}
	catch (exception& ex)
	{
		Logger().Error("CClassModManager::LoadClassMod: an error occured while parsing ClassModConfig.json: %s\n", ex.what());
	}
}

bool CClassModManager::HasClassMod(IUser* user, int slot)
{
	return g_UserDatabase.IsClassModExist(user->GetID(), slot);
}

ClassModInfo_t CClassModManager::GetClassModBySlot(IUser* user, int slot)
{
	ClassModInfo_t info;
	g_UserDatabase.GetUserClassModLoadOut(user->GetID(), slot, info);
	return info;
}

ClassModInfo_t CClassModManager::GetClassModPresetById(int itemId)
{
	ClassModInfo_t info;

	std::vector<int> preset = m_pClassModPresetTable->GetRow<int>(to_string(itemId));
	std::vector<int> status = m_pClassStatusTable->GetRow<int>(to_string(itemId));

	int presetId = preset.at(0); // where to use

	info.slotId = -1;
	info.status.health = status.at(0) + 1;
	info.status.attack = status.at(1) + 1;
	info.status.speed = status.at(2) + 1;
	info.status.armor = status.at(3) + 1;
	info.status.ammo = status.at(4) + 1;

	info.sessionbonus.itemId[0] = preset.at(1);
	info.sessionbonus.itemId[1] = -1;
	info.sessionbonus.itemId[2] = -1;
	info.sessionbonus.itemId[3] = -1;
	info.sessionbonus.itemId[4] = -1;

	info.infodisplay.itemId[0] = preset.at(2);
	info.infodisplay.itemId[1] = -1;
	info.infodisplay.itemId[2] = -1;
	info.infodisplay.itemId[3] = -1;
	info.infodisplay.itemId[4] = -1;

	info.modbuff.itemId[0] = preset.at(3);
	info.modbuff.itemId[1] = preset.at(4);
	info.modbuff.itemId[2] = -1;
	info.modbuff.itemId[3] = -1;
	info.modbuff.itemId[4] = -1;

	info.activeskill.itemId[0] = preset.at(5);
	info.activeskill.itemId[1] = -1;
	info.activeskill.itemId[2] = -1;
	info.activeskill.itemId[3] = -1;
	info.activeskill.itemId[4] = -1;

	info.passiveskill.itemId[0] = preset.at(6);
	info.passiveskill.itemId[1] = preset.at(7);
	info.passiveskill.itemId[2] = preset.at(8);
	info.passiveskill.itemId[3] = preset.at(9);
	info.passiveskill.itemId[4] = preset.at(10);

	info.addon.itemId[0] = preset.at(11);
	info.addon.itemId[1] = preset.at(12);
	info.addon.itemId[2] = preset.at(13);
	info.addon.itemId[3] = preset.at(14);
	info.addon.itemId[4] = preset.at(15);

	info.pairingweapon.itemId[0] = preset.at(16);
	info.pairingweapon.itemId[1] = -1;
	info.pairingweapon.itemId[2] = -1;
	info.pairingweapon.itemId[3] = -1;
	info.pairingweapon.itemId[4] = -1;

	return info;
}

bool CClassModManager::EnableSlot(IUser* user, int itemslot, int category, int& enabledslot, ClassModInfo_t& info)
{
	if (category < 2 || category > 8)
		return false;

	ClassModInfo_t::ClassModSlot_t* categories[9] = { nullptr, nullptr, &info.sessionbonus, &info.infodisplay, &info.modbuff, &info.activeskill, &info.passiveskill, &info.addon, &info.pairingweapon };

	int lastIdx = -1;
	for (int i = 0; i < 5; ++i)
	{
		if (categories[category]->itemId[i] == -1)
		{
			lastIdx = i;
			break;
		}
	}

	if (lastIdx == -1)
		return false;

	if (false)
	{
		if (m_ClassModConfig->categorymaxslots.at(category) > lastIdx)
		{
			return false;
		}
	}

	categories[category]->itemId[lastIdx] = 0;
	enabledslot = lastIdx;

	SaveClassModInfo(user, itemslot, info);
	return true;
}

bool CClassModManager::ApplyMod(IUser* user, int itemslot, int category, int slot, int modItem, ClassModInfo_t& info)
{
	if (category < 2 || category > 8)
		return false;

	if (slot < 0 || slot > 4)
		return false;

	ClassModInfo_t::ClassModSlot_t* categories[9] = { nullptr, nullptr, &info.sessionbonus, &info.infodisplay, &info.modbuff, &info.activeskill, &info.passiveskill, &info.addon, &info.pairingweapon };

	if (categories[category]->itemId[slot] == -1)
		return false;

	categories[category]->itemId[slot] = modItem;

	SaveClassModInfo(user, itemslot, info);
	return true;
}

bool CClassModManager::RemoveMod(IUser* user, int itemslot, int category, int slot, ClassModInfo_t& info)
{
	if (category < 2 || category > 8)
		return false;

	if (slot < 0 || slot > 4)
		return false;

	ClassModInfo_t::ClassModSlot_t* categories[9] = { nullptr, nullptr, &info.sessionbonus, &info.infodisplay, &info.modbuff, &info.activeskill, &info.passiveskill, &info.addon, &info.pairingweapon };

	if (categories[category]->itemId[slot] == -1)
		return false;

	categories[category]->itemId[slot] = 0;
	
	SaveClassModInfo(user, itemslot, info);
	return true;
}

bool CClassModManager::InterchangeMod(IUser* user, int itemslot, int category, int oldslot, int newslot, ClassModInfo_t& info)
{
	if (category < 2 || category > 8)
		return false;

	if (oldslot < 0 || oldslot > 4)
		return false;

	if (newslot < 0 || newslot > 4)
		return false;

	ClassModInfo_t::ClassModSlot_t* categories[9] = { nullptr, nullptr, &info.sessionbonus, &info.infodisplay, &info.modbuff, &info.activeskill, &info.passiveskill, &info.addon, &info.pairingweapon };

	if (categories[category]->itemId[oldslot] == -1)
		return false;

	if (categories[category]->itemId[newslot] == -1)
		return false;

	int item = categories[category]->itemId[newslot];
	categories[category]->itemId[newslot] = categories[category]->itemId[oldslot];
	categories[category]->itemId[oldslot] = item;

	SaveClassModInfo(user, itemslot, info);
	return true;
}

bool CClassModManager::ChangeStats(IUser* user, int itemslot, ClassModInfo_t& info)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> levelRange(105, 125);
	std::uniform_int_distribution<int> attributeRange(0, 4);

	info.status.health = 15;
	info.status.attack = 15;
	info.status.speed = 15;
	info.status.armor = 15;
	info.status.ammo = 15;

	int level = levelRange(gen);

	int remaindingLevel = level - 75;

	unsigned char* status = &info.status.health;
	while (remaindingLevel)
	{
		int idx = attributeRange(gen);
		if (status[idx] == 30)
			continue;
		status[idx]++;
		--remaindingLevel;
	}

	SaveClassModInfo(user, itemslot, info);
	return true;
}




void CClassModManager::SaveClassModInfo(IUser* user, int slot, const ClassModInfo_t& info)
{
	if (g_UserDatabase.IsClassModExist(user->GetID(), slot))
		g_UserDatabase.UpdateUserClassModLoadOut(user->GetID(), slot, info);
	else
		g_UserDatabase.AddUserClassModLoadOut(user->GetID(), slot, info);
}