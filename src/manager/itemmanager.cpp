#include "itemmanager.h"

#include "serverinstance.h"
#include "packetmanager.h"
#include "userdatabase.h"
#include "luckyitemmanager.h"

#include "user/userinventoryitem.h"

#include "common/utils.h"

#include "csvtable.h"
#include "serverconfig.h"
#include "keyvalues.hpp"

using namespace std;

#define ITEM_REWARDS_VERSION 1
#define WEAPON_PAINTS_VERSION 1

CItemManager g_ItemManager;

CItemManager::CItemManager() : CBaseManager("ItemManager", true, true)
{
	m_pReinforceMaxLvTable = NULL;
	m_pReinforceMaxExpTable = NULL;
}

CItemManager::~CItemManager()
{
}

bool CItemManager::Init()
{
	if (!KVToJson())
	{
		if (!LoadRewards())
			return false;
	}

	if (!LoadWeaponPaints())
		return false;

	m_pReinforceMaxLvTable = new CCSVTable(OBFUSCATE("Data/ReinforceMaxLv.csv"), rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true), rapidcsv::LineReaderParams());
	m_pReinforceMaxExpTable = new CCSVTable(OBFUSCATE("Data/ReinforceMaxEXP.csv"), rapidcsv::LabelParams(0, 0), rapidcsv::SeparatorParams(), rapidcsv::ConverterParams(true), rapidcsv::LineReaderParams());

	if (m_pReinforceMaxLvTable->IsLoadFailed() || m_pReinforceMaxExpTable->IsLoadFailed())
	{
		Logger().Fatal("CItemManager::Init(): couldn't load some csv files. Required csv:\nData/ReinforceMaxLv.csv\nData/ReinforceMaxEXP.csv\n");
		return false;
	}

	return true;
}

void CItemManager::Shutdown()
{
	CBaseManager::Shutdown();

	m_Rewards.clear();
	m_WeaponPaints.clear();

	delete m_pReinforceMaxLvTable;
	delete m_pReinforceMaxExpTable;
}

bool CItemManager::LoadRewards()
{
	try
	{
		ifstream f("ItemRewards.json");
		ordered_json cfg = ordered_json::parse(f, nullptr, false, true);

		if (cfg.is_discarded())
		{
			Logger().Fatal("CItemManager::LoadRewards: couldn't load ItemRewards.json.\n");
			return false;
		}

		int version = cfg.value("Version", 0);
		if (version != ITEM_REWARDS_VERSION)
		{
			Logger().Fatal("CItemManager::LoadRewards: %d != ITEM_REWARDS_VERSION(%d)\n", version, ITEM_REWARDS_VERSION);
			return false;
		}

		for (auto& iReward : cfg.items())
		{
			json jReward = iReward.value();
			if (!jReward.is_object())
				continue;

			Reward reward;
			reward.rewardId = stoi(iReward.key());
			reward.select = jReward.value("Select", 0);
			reward.lvlRestriction = jReward.value("LvlRestriction", 0);
			reward.points = jReward.value("Points", reward.points);
			reward.exp = jReward.value("Exp", reward.exp);
			reward.honorPoints = jReward.value("HonorPoints", reward.honorPoints);
			reward.title = jReward.value("Title", "");
			reward.description = jReward.value("Description", "");
			reward.localized = jReward.value("Localized", 0);

			if (jReward.contains("Items"))
			{
				int rewardSelectID = 1;
				json jItems = jReward["Items"];
				for (auto& iItem : jItems.items())
				{
					json jItem = iItem.value();

					RewardItem rewardItem;
					rewardItem.selectID = 0;
					rewardItem.eventFlag = 0;

					if (reward.select == true)
						rewardItem.selectID = rewardSelectID++;

					rewardItem.itemID = stoi(iItem.key());
					rewardItem.count = jItem.value("Count", 1);
					rewardItem.duration = jItem.value("Duration", 0);

					reward.items.push_back(rewardItem);
				}
			}
			if (jReward.contains("RandomItems"))
			{
				json jRandomItems = jReward["RandomItems"];
				for (auto& jRandomItem : jRandomItems)
				{
					RewardItemsRandom rewardItemsRandom = {};
					for (auto& iItems : jRandomItem.items())
					{
						json jItems = iItems.value();

						if (iItems.key() == "Durations")
						{
							for (auto& iDurations : jItems.items())
							{
								RewardItemRandomDuration rewardItemRandomDuration;
								rewardItemRandomDuration.chance = stoi(iDurations.key());
								rewardItemRandomDuration.durations = iDurations.value().get<vector<int>>();

								rewardItemsRandom.durations.push_back(rewardItemRandomDuration);
							}

							continue;
						}

						RewardItemRandom rewardItemRandom;
						rewardItemRandom.chance = stoi(iItems.key());
						int rewardSelectID = 1;
						for (auto& iItem : jItems.items())
						{
							json jItem = iItem.value();

							RewardItem rewardItem;
							rewardItem.selectID = 0;
							rewardItem.eventFlag = 0;

							if (reward.select == true)
								rewardItem.selectID = rewardSelectID++;

							rewardItem.itemID = stoi(iItem.key());
							rewardItem.count = jItem.value("Count", 1);
							rewardItem.duration = jItem.value("Duration", 0);

							rewardItemRandom.items.push_back(rewardItem);
						}
						rewardItemsRandom.items.push_back(rewardItemRandom);
					}
					reward.randomItems.push_back(rewardItemsRandom);
				}
			}

			m_Rewards.push_back(reward);
		}
	}
	catch (exception& ex)
	{
		Logger().Fatal("CItemManager::LoadRewards: an error occured while parsing ItemRewards.json: %s\n", ex.what());
		return false;
	}

	return true;
}

bool CItemManager::LoadWeaponPaints()
{
	try
	{
		ifstream f("Data/WeaponPaints.json");
		ordered_json cfg = ordered_json::parse(f, nullptr, false, true);

		if (cfg.is_discarded())
		{
			Logger().Fatal("CItemManager::LoadWeaponPaints: couldn't load WeaponPaints.json.\n");
			return false;
		}

		int version = cfg.value("Version", 0);
		if (version != WEAPON_PAINTS_VERSION)
		{
			Logger().Fatal("CItemManager::LoadWeaponPaints: %d != WEAPON_PAINTS_VERSION(%d)\n", version, WEAPON_PAINTS_VERSION);
			return false;
		}

		for (auto& iWeaponPaint : cfg.items())
		{
			json jWeaponPaint = iWeaponPaint.value();
			if (!jWeaponPaint.is_object())
				continue;

			WeaponPaint weaponPaint;
			weaponPaint.itemID = stoi(iWeaponPaint.key());
			weaponPaint.paintIDs = jWeaponPaint.value("Paints", weaponPaint.paintIDs);
			
			m_WeaponPaints.push_back(weaponPaint);
		}
	}
	catch (exception& ex)
	{
		Logger().Fatal("CItemManager::LoadWeaponPaints: an error occured while parsing WeaponPaints.json: %s\n", ex.what());
		return false;
	}

	return true;
}

bool CItemManager::KVToJson()
{
	ofstream f("ItemRewards.json", ios::in);
	if (f)
	{
		f.close();
		return false;
	}

	f.close();

	{
		KV::KeyValues root = KV::KeyValues::parseFromFile("ItemRewards.txt");
		if (root.isEmpty())
			return false;

		KV::KeyValues& kvRewards = root["Rewards"];

		istringstream iss(string(""));
		vector<int> results;

		for (auto& kvReward : kvRewards)
		{
			if (!kvReward.isSection())
				continue;

			Reward reward;

			iss.clear();
			iss.str(kvReward.getKey());
			iss >> reward.rewardId;

			reward.select = kvReward["Select"].getValueAsBool();
			reward.lvlRestriction = kvReward["LvlRestriction"].getValueAsInt();

			iss.clear();
			iss.str(kvReward["Points"].getValue());

			results.assign((istream_iterator<int>(iss)), istream_iterator<int>());
			reward.points = results;

			iss.clear();
			iss.str(kvReward["Exp"].getValue());

			results.assign((istream_iterator<int>(iss)), istream_iterator<int>());
			reward.exp = results;

			iss.clear();
			iss.str(kvReward["HonorPoints"].getValue());

			results.assign((istream_iterator<int>(iss)), istream_iterator<int>());
			reward.honorPoints = results;

			reward.title = kvReward["Title"].getValue();
			reward.description = kvReward["Description"].getValue();

			KV::KeyValues& kvRewardItems = kvReward["Items"];
			int rewardSelectID = 1;
			for (auto& kvItem : kvRewardItems)
			{
				RewardItem rewardItem;
				rewardItem.selectID = 0;
				rewardItem.eventFlag = 0;

				if (reward.select == true)
					rewardItem.selectID = rewardSelectID++;

				rewardItem.itemID = stoi(kvItem.getKey());
				rewardItem.count = kvItem["Count"].getValueAsInt(1);
				rewardItem.duration = kvItem["Duration"].getValueAsInt();

				reward.items.push_back(rewardItem);
			}

			KV::KeyValues& kvRandomItems = kvReward["RandomItems"];
			for (auto& kvRandomItem : kvRandomItems)
			{
				RewardItemsRandom rewardItemsRandom;
				for (auto& kvItems : kvRandomItem)
				{
					if (kvItems.getKey() == "Durations")
					{
						for (auto& kvDurations : kvItems)
						{
							RewardItemRandomDuration rewardItemRandomDuration;
							rewardItemRandomDuration.chance = stoi(kvDurations.getKey());
							for (auto& kvDuration : kvDurations)
								rewardItemRandomDuration.durations.push_back(stoi(kvDuration.getKey()));

							rewardItemsRandom.durations.push_back(rewardItemRandomDuration);
						}

						continue;
					}

					RewardItemRandom rewardItemRandom;
					rewardItemRandom.chance = stoi(kvItems.getKey());
					int rewardSelectID = 1;
					for (auto& kvItem : kvItems)
					{
						RewardItem rewardItem;
						rewardItem.selectID = 0;
						rewardItem.eventFlag = 0;

						if (reward.select == true)
							rewardItem.selectID = rewardSelectID++;

						rewardItem.itemID = stoi(kvItem.getKey());
						rewardItem.count = kvItem["Count"].getValueAsInt(1);
						rewardItem.duration = kvItem["Duration"].getValueAsInt();

						rewardItemRandom.items.push_back(rewardItem);
					}

					rewardItemsRandom.items.push_back(rewardItemRandom);
				}

				reward.randomItems.push_back(rewardItemsRandom);
			}
			m_Rewards.push_back(reward);
		}
	}

	if (m_Rewards.empty())
		return false;

	ordered_json cfg;
	cfg["Version"] = ITEM_REWARDS_VERSION;
	for (auto& reward : m_Rewards)
	{
		if (reward.select)
			cfg[to_string(reward.rewardId)]["Select"] = reward.select;

		if (reward.lvlRestriction)
			cfg[to_string(reward.rewardId)]["LvlRestriction"] = reward.lvlRestriction;

		if (reward.points.size())
			cfg[to_string(reward.rewardId)]["Points"] = reward.points;

		if (reward.exp.size())
			cfg[to_string(reward.rewardId)]["Exp"] = reward.exp;

		if (reward.honorPoints.size())
			cfg[to_string(reward.rewardId)]["HonorPoints"] = reward.honorPoints;

		if (!reward.title.empty())
			cfg[to_string(reward.rewardId)]["Title"] = reward.title;

		if (!reward.description.empty())
			cfg[to_string(reward.rewardId)]["Description"] = reward.description;

		for (auto& item : reward.items)
		{
			cfg[to_string(reward.rewardId)]["Items"][to_string(item.itemID)]["Count"] = item.count;
			cfg[to_string(reward.rewardId)]["Items"][to_string(item.itemID)]["Duration"] = item.duration;
		}

		for (auto& itemNum : reward.randomItems)
		{
			ordered_json jItemNum({});
			for (auto& duration : itemNum.durations)
			{
				jItemNum["Durations"][to_string(duration.chance)] = duration.durations;
			}

			for (auto& item : itemNum.items)
			{
				for (auto& rewardItem : item.items)
				{
					jItemNum[to_string(item.chance)][to_string(rewardItem.itemID)]["Count"] = rewardItem.count;
					jItemNum[to_string(item.chance)][to_string(rewardItem.itemID)]["Duration"] = rewardItem.duration;
				}
			}

			cfg[to_string(reward.rewardId)]["RandomItems"] += jItemNum;
		}
	}

	f.open("ItemRewards.json");
	f << setfill('\t') << setw(1) << cfg << endl;

	return true;
}

bool CItemManager::OnItemPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = g_UserManager.GetUserBySocket(socket);
	if (!user)
	{
		return false;
	}

	int type = msg->ReadUInt8();
	switch (type)
	{
	case ItemPacketType::UseItem:
	{
		int inventoryType = msg->ReadUInt8();
		int itemSlot = msg->ReadUInt16();
		int unk = msg->ReadUInt8();
		int itemCount = msg->ReadUInt16();

		// TODO: shit
		CUserInventoryItem item;
		g_UserDatabase.GetInventoryItemBySlot(user->GetID(), item.GameSlotToSlot(itemSlot), item);

		if (!item.m_nItemID)
			return false;

		string className = g_pItemTable->GetCell<string>("ClassName", to_string(item.m_nItemID));

		if (inventoryType == 1 || className == "LobbyBG" || className == "zbRespawnEffect" || className == "CombatInfoItem") // switch status
		{
			vector<CUserInventoryItem> itemsWithSameID;
			if (g_UserDatabase.GetInventoryItemsByID(user->GetID(), item.m_nItemID, itemsWithSameID) < 0)
			{
				Logger().Warn("CItemManager::OnItemPacket: cannot use item, database error\n");
				return false;
			}

			vector<CUserInventoryItem> items;

			// turn off status of items with same ID that have status on
			for (auto& i : itemsWithSameID)
			{
				if (i.m_nStatus)
				{
					i.m_nStatus = 0;

					i.PushItem(items, i);
				}
			}

			// turn on status of the desired item
			item.m_nStatus = 1;

			int flag = UITEM_FLAG_STATUS;

			if (className == "LobbyBG")
			{
				CUserCharacter character = user->GetCharacter(UFLAG_LOW_NAMEPLATE);
				if (character.lowFlag == 0)
				{
					Logger().Warn("CItemManager::OnItemPacket: cannot use item, database error\n");
					return false;
				}

				if (character.nameplateID == item.m_nItemID)
				{
					user->UpdateNameplate(0);
					item.m_nInUse = 0;
				}
				else
				{
					if (character.nameplateID)
					{
						CUserInventoryItem item1;
						if (g_UserDatabase.GetFirstActiveItemByItemID(user->GetID(), character.nameplateID, item1) < 0)
						{
							Logger().Warn("CItemManager::OnItemPacket: cannot use item, database error\n");
							return false;
						}

						if (item1.m_nItemID)
						{
							item1.m_nInUse = 0;

							item1.PushItem(items, item1);
						}
					}

					user->UpdateNameplate(item.m_nItemID);
					item.m_nInUse = 1;
				}

				flag |= UITEM_FLAG_INUSE;
			}
			else if (className == "zbRespawnEffect")
			{
				CUserCharacterExtended character = user->GetCharacterExtended(EXT_UFLAG_ZBRESPAWNEFFECT);
				if (character.flag == 0)
				{
					Logger().Warn("CItemManager::OnItemPacket: cannot use item, database error\n");
					return false;
				}

				if (character.zbRespawnEffect == item.m_nItemID)
				{
					user->UpdateZbRespawnEffect(0);
					item.m_nInUse = 0;
				}
				else
				{
					if (character.zbRespawnEffect)
					{
						CUserInventoryItem item1;
						if (g_UserDatabase.GetFirstActiveItemByItemID(user->GetID(), character.zbRespawnEffect, item1) < 0)
						{
							Logger().Warn("CItemManager::OnItemPacket: cannot use item, database error\n");
							return false;
						}

						if (item1.m_nItemID)
						{
							item1.m_nInUse = 0;

							item1.PushItem(items, item1);
						}
					}

					user->UpdateZbRespawnEffect(item.m_nItemID);
					item.m_nInUse = 1;
				}

				flag |= UITEM_FLAG_INUSE;
			}
			else if (className == "CombatInfoItem")
			{
				CUserCharacterExtended character = user->GetCharacterExtended(EXT_UFLAG_KILLERMARKEFFECT);
				if (character.flag == 0)
				{
					Logger().Warn("CItemManager::OnItemPacket: cannot use item, database error\n");
					return false;
				}

				if (character.killerMarkEffect == item.m_nItemID)
				{
					user->UpdateKillerMarkEffect(0);
					item.m_nInUse = 0;
				}
				else
				{
					if (character.killerMarkEffect)
					{
						CUserInventoryItem item1;
						if (g_UserDatabase.GetFirstActiveItemByItemID(user->GetID(), character.killerMarkEffect, item1) < 0)
						{
							Logger().Warn("CItemManager::OnItemPacket: cannot use item, database error\n");
							return false;
						}

						if (item1.m_nItemID)
						{
							item1.m_nInUse = 0;

							item1.PushItem(items, item1);
						}
					}

					user->UpdateKillerMarkEffect(item.m_nItemID);
					item.m_nInUse = 1;
				}

				flag |= UITEM_FLAG_INUSE;
			}

			item.PushItem(items, item);

			if (g_UserDatabase.UpdateInventoryItems(user->GetID(), items, flag) <= 0)
			{
				Logger().Warn("CItemManager::OnItemPacket: cannot use item, database error\n");
				return false;
			}

			// send inventory update to user
			g_PacketManager.SendInventoryAdd(socket, items);
		}
		else if (inventoryType == 8 || className == "Tattoo")
		{
			OnCostumeEquip(user, itemSlot);
		}
		else
		{
			UseItem(user, itemSlot, itemCount);
		}

		Logger().Info("CItemManager::OnItemPacket: inventoryType: %d, slot: %d, unk: %d, itemCount: %d\n", inventoryType, itemSlot, unk, itemCount);
		break;
	}
	case ItemPacketType::OpenDecoder:
		OpenDecoder(user, msg->ReadUInt16(), msg->ReadUInt16());
		break;
	case ItemPacketType::WeaponAssem:
		break;
	case ItemPacketType::MaterialAssem:
		break;
	case ItemPacketType::DailyRewards:
		OnDailyRewardsRequest(user, msg->ReadUInt8());
		break;
	case ItemPacketType::Disassemble:
	{
		OnDisassembleRequest(user, msg);
		break;
	}
	case ItemPacketType::ReqTattooEquip:
	case ItemPacketType::ReqCostumeEquip:
		OnCostumeEquip(user, msg->ReadUInt16());
		break;
	case ItemPacketType::ItemExtend:
	{
		int unk = msg->ReadUInt8();
		int a1 = msg->ReadUInt16();
		int a2 = msg->ReadUInt16();

		vector<CUserInventoryItem> items;
		if (!g_UserDatabase.GetInventoryItemsByID(user->GetID(), 676 /*item extender*/, items))
			return false;

		UseItem(user, items[0].GetGameSlot(), a1, a2);
		break;
	}
	case ItemPacketType::EnhanceReq:
		OnEnhancementRequest(user, msg);
		break;
	case ItemPacketType::WeaponPaintReq: // weapon paint request
		OnWeaponPaintRequest(user, msg);
		break;
	case ItemPacketType::Part:
		OnPartEquipRequest(user, msg);
		break;
	case ItemPacketType::SwitchInUse: // switch in use
		OnSwitchInUseRequest(user, msg);
		break;
	case ItemPacketType::WeaponPaintSwitchReq: // switch weapon paint request
		OnWeaponPaintSwitchRequest(user, msg);
		break;
	case ItemPacketType::LockItem:
		OnLockItemRequest(user, msg);
		break;
	default:
		Logger().Warn("Packet_Item type %d is not implemented\n", type);
		break;
	}

	return true;
}

int CItemManager::AddItem(int userID, IUser* user, int itemID, int count, int duration, int lockStatus)
{
	int itemStatus = 1;
	int itemInUse = 1;

	if (g_UserDatabase.IsInventoryFull(userID))
		return ITEM_ADD_INVENTORY_FULL;

	if (g_pItemTable->GetRowIdx(to_string(itemID)) < 0)
		return ITEM_ADD_UNKNOWN_ITEMID;

	if (count <= 0)
		count = 1;

	if (duration < 0)
		duration = 0;

	// don't add PasswordBox add as separate item, increment uData->passwordBoxes value
	if (itemID == 166) // PasswordBox
	{
		if (!user)
		{
			CUserCharacter character;
			character.lowFlag = UFLAG_LOW_PASSWORDBOXES;
			if (g_UserDatabase.GetCharacter(userID, character) <= 0)
				return ITEM_ADD_DB_ERROR;

			character.passwordBoxes += count;

			if (g_UserDatabase.UpdateCharacter(userID, character) <= 0)
				return ITEM_ADD_DB_ERROR;
		}
		else
		{
			if (user->UpdatePasswordBoxes(count) <= 0)
				return ITEM_ADD_DB_ERROR;
		}

		return ITEM_ADD_SUCCESS;
	}

	CUserInventoryItem itemWithSameID;
	if (g_UserDatabase.GetFirstItemByItemID(userID, itemID, itemWithSameID) < 0)
		return ITEM_ADD_DB_ERROR;

	int currentTimestamp = (int)g_pServerInstance->GetCurrentTime();

	if (itemWithSameID.m_nItemID)
	{
		itemStatus = 0;
		itemInUse = 0;

		if (duration)
		{
			CUserInventoryItem itemToExtend;
			if (g_UserDatabase.GetFirstExtendableItemByItemID(userID, itemID, itemToExtend) < 0)
				return ITEM_ADD_DB_ERROR;

			if (itemToExtend.m_nItemID)
			{
				int ret = ExtendItem(userID, user, itemToExtend, duration, true);
				if (ret == 1)
					return ITEM_ADD_SUCCESS;
				else if (ret == -1)
					return ITEM_ADD_DB_ERROR;
			}
		}
	}
	else if (duration)
		duration = currentTimestamp + duration * CSO_24_HOURS_IN_MINUTES; // !!! items must have inuse = 1 and status = 1

	// Category: 1 - pistols, 2 - shotguns, 3 - SMG, 4 - rifle, 5 - machine guns, 6 - equipment,  7 - class, 12 - costume, 13 - weapon parts, 
	int useType = g_pItemTable->GetCell<int>(OBFUSCATE("UseType"), to_string(itemID));
	int category = g_pItemTable->GetCell<int>("Category", to_string(itemID));
	// countable items with use button
	if ((category == 9 && (useType == 0 || useType == 1 || useType == 2)) || (category == 8 && (useType == 0 || useType == 2)))
	{
		vector<CUserInventoryItem> items;
		if (itemWithSameID.m_nItemID)
		{
			// update first item
			itemWithSameID.m_nCount += count;

			if (g_UserDatabase.UpdateInventoryItem(userID, itemWithSameID, UITEM_FLAG_COUNT) <= 0)
				return ITEM_ADD_DB_ERROR;

			itemWithSameID.PushItem(items, itemWithSameID);
		}
		else
		{
			itemStatus = 0;
			itemInUse = 1;

			CUserInventoryItem item;

			// push item to vector after adding item to db!
			item.PushItem(items, itemID, count, itemStatus, itemInUse, currentTimestamp, duration, 0, 0, 0, 0, 0, {}, 0, 0, lockStatus); // push new items to inventory

			if (g_UserDatabase.AddInventoryItem(userID, items.back()) <= 0)
				return ITEM_ADD_DB_ERROR;
		}

		// update client inventory
		if (user)
			g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);

		return ITEM_ADD_SUCCESS;
	}

	string className = g_pItemTable->GetCell<string>("ClassName", to_string(itemID));
	if (category == 12 || className == "Tattoo")
	{
		CUserCostumeLoadout loadout;
		if (g_UserDatabase.GetCostumeLoadout(userID, loadout) <= 0)
			return ITEM_ADD_DB_ERROR;

		int zombieSkinType = -1;

		itemStatus = 0;
		itemInUse = 0;

		if (className == "HeadCostume")
		{
			if (!loadout.m_nHeadCostumeID)
			{
				itemInUse = 1;
				itemStatus = 1;
				loadout.m_nHeadCostumeID = itemID;
			}
		}
		else if (className == "BackCostume")
		{
			if (!loadout.m_nBackCostumeID)
			{
				itemInUse = 1;
				itemStatus = 1;
				loadout.m_nBackCostumeID = itemID;
			}
		}
		else if (className == "ArmCostume")
		{
			if (!loadout.m_nArmCostumeID)
			{
				itemInUse = 1;
				itemStatus = 1;
				loadout.m_nArmCostumeID = itemID;
			}
		}
		else if (className == "PelvisCostume")
		{
			if (!loadout.m_nPelvisCostumeID)
			{
				itemInUse = 1;
				itemStatus = 1;
				loadout.m_nPelvisCostumeID = itemID;
			}
		}
		else if (className == "FaceCostume")
		{
			if (!loadout.m_nFaceCostumeID)
			{
				itemInUse = 1;
				itemStatus = 1;
				loadout.m_nFaceCostumeID = itemID;
			}
		}
		else if (className == "ZombieSkinCostume")
		{
			zombieSkinType = g_pItemTable->GetCell<int>("ZombieSkin", to_string(itemID));
			if (zombieSkinType >= ZB_COSTUME_SLOT_COUNT_MAX)
			{
				Logger().Warn("CItemManager::AddItem: can't setup zb costume loadout (zombieSkinType >= ZB_COSTUME_SLOT_COUNT_MAX)!!!\n");
			}
			else if (!loadout.m_ZombieSkinCostumeID.count(zombieSkinType) && !loadout.m_ZombieSkinCostumeID[zombieSkinType])
			{
				itemInUse = 1;
				itemStatus = 1;
				loadout.m_ZombieSkinCostumeID[zombieSkinType] = itemID;
			}
		}
		else if (className == "Tattoo")
		{
			if (!loadout.m_nTattooID)
			{
				itemInUse = 1;
				itemStatus = 1;
				loadout.m_nTattooID = itemID;
			}
		}
		else if (className == "PetCostume")
		{
			if (!loadout.m_nPetCostumeID)
			{
				itemInUse = 1;
				itemStatus = 1;
				loadout.m_nPetCostumeID = itemID;
			}
		}

		if (g_UserDatabase.UpdateCostumeLoadout(userID, loadout, zombieSkinType) <= 0)
			return ITEM_ADD_DB_ERROR;
	}

	if (className == "LobbyBG")
	{
		CUserCharacter character = {};
		character.lowFlag = UFLAG_LOW_NAMEPLATE;
		if (g_UserDatabase.GetCharacter(userID, character) <= 0)
			return ITEM_ADD_DB_ERROR;

		if (character.nameplateID)
			itemInUse = 0;
		else if (user)
			user->UpdateNameplate(itemID);
		else
		{
			character.nameplateID = itemID;
			g_UserDatabase.UpdateCharacter(userID, character);
		}
	}
	else if (className == "zbRespawnEffect")
	{
		CUserCharacterExtended character = {};
		character.flag = EXT_UFLAG_ZBRESPAWNEFFECT;
		if (g_UserDatabase.GetCharacterExtended(userID, character) <= 0)
			return ITEM_ADD_DB_ERROR;

		if (character.zbRespawnEffect)
			itemInUse = 0;
		else if (user)
			user->UpdateZbRespawnEffect(itemID);
		else
		{
			character.zbRespawnEffect = itemID;
			g_UserDatabase.UpdateCharacterExtended(userID, character);
		}
	}
	else if (className == "CombatInfoItem")
	{
		CUserCharacterExtended character = {};
		character.flag = EXT_UFLAG_KILLERMARKEFFECT;
		if (g_UserDatabase.GetCharacterExtended(userID, character) <= 0)
			return ITEM_ADD_DB_ERROR;

		if (character.killerMarkEffect)
			itemInUse = 0;
		else if (user)
			user->UpdateKillerMarkEffect(itemID);
		else
		{
			character.killerMarkEffect = itemID;
			g_UserDatabase.UpdateCharacterExtended(userID, character);
		}
	}

	string resourceName = g_pItemTable->GetCell<string>("recourcename", to_string(itemID));
	if (resourceName.find("chatcolor_") != std::string::npos)
	{
		CUserCharacter character = {};
		character.highFlag = UFLAG_HIGH_CHATCOLOR;
		if (g_UserDatabase.GetCharacter(userID, character) <= 0)
			return ITEM_ADD_DB_ERROR;

		if (!character.chatColorID)
		{
			if (user)
				user->UpdateChatColor(itemID);
			else
			{
				character.chatColorID = itemID;
				g_UserDatabase.UpdateCharacter(userID, character);
			}
		}
	}

	CUserInventoryItem item;
	vector<CUserInventoryItem> items;

	// push new item
	item.PushItem(items, itemID, count, itemStatus, itemInUse, currentTimestamp, duration, 0, 0, 0, 0, 0, {}, 0, 0, lockStatus); // push new items to inventory

	// add new item to db
	if (g_UserDatabase.AddInventoryItem(userID, items.back()) <= 0)
	{
		return ITEM_ADD_DB_ERROR;
	}

	// send inventory update to user
	if (user)
		g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);

	if (user)
	{
		if (itemID == 8357) // superRoom
		{
			IRoom* currentRoom = user->GetCurrentRoom();
			if (currentRoom != NULL)
			{
				if (user == currentRoom->GetHostUser()) // it's the room host, so add the superRoom flag
				{
					CRoomSettings* roomSettings = currentRoom->GetSettings();

					if (!roomSettings->superRoom)
					{
						roomSettings->superRoom = 1;

						for (auto u : currentRoom->GetUsers())
						{
							g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), roomSettings, 0, ROOM_LOWMID_SUPERROOM);
						}
					}
				}
			}
		}
		else if (itemID == 439) // BigHeadEvent
		{
			IRoom* currentRoom = user->GetCurrentRoom();
			if (currentRoom != NULL)
			{
				if (user == currentRoom->GetHostUser()) // it's the room host, so add the sd flag
				{
					CRoomSettings* roomSettings = currentRoom->GetSettings();

					if ((roomSettings->gameModeId == 3 || roomSettings->gameModeId == 4 || roomSettings->gameModeId == 5 || roomSettings->gameModeId == 15 || roomSettings->gameModeId == 24) && !roomSettings->sd)
					{
						roomSettings->sd = 1;

						for (auto u : currentRoom->GetUsers())
						{
							g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), roomSettings, 0, ROOM_LOWMID_SD);
						}
					}
				}
			}
		}
	}

	return ITEM_ADD_SUCCESS;
}

int CItemManager::AddItems(int userID, IUser* user, vector<RewardItem>& items)
{
	int inventoryItemsCount = g_UserDatabase.GetInventoryItemsCount(userID);

	if (inventoryItemsCount < 0)
		return ITEM_ADD_DB_ERROR;

	vector<CUserInventoryItem> insertedItems;
	vector<CUserInventoryItem> updatedItems;

	int result = ITEM_ADD_SUCCESS;

	g_UserDatabase.CreateTransaction();

	for (auto& item : items)
	{
		if (inventoryItemsCount + insertedItems.size() >= g_pServerConfig->inventorySlotMax)
		{
			result = ITEM_ADD_INVENTORY_FULL;
			break;
		}

		int itemStatus = 1;
		int itemInUse = 1;
		int itemID = item.itemID;
		int duration = item.duration;
		int count = item.count;
		int lockStatus = item.lockStatus;

		if (g_pItemTable->GetRowIdx(to_string(itemID)) < 0)
			continue;

		if (count <= 0)
			count = 1;

		if (duration < 0)
			duration = 0;

		// don't add PasswordBox add as separate item, increment uData->passwordBoxes value
		if (itemID == 166) // PasswordBox
		{
			if (!user)
			{
				CUserCharacter character;
				character.lowFlag = UFLAG_LOW_PASSWORDBOXES;
				if (g_UserDatabase.GetCharacter(userID, character) <= 0)
				{
					result = ITEM_ADD_DB_ERROR;
					break;
				}

				character.passwordBoxes += count;

				if (g_UserDatabase.UpdateCharacter(userID, character) <= 0)
				{
					result = ITEM_ADD_DB_ERROR;
					break;
				}
			}
			else
			{
				if (user->UpdatePasswordBoxes(count) <= 0)
				{
					result = ITEM_ADD_DB_ERROR;
					break;
				}
			}

			continue;
		}

		CUserInventoryItem itemWithSameID;
		if (g_UserDatabase.GetFirstItemByItemID(userID, itemID, itemWithSameID) < 0)
		{
			result = ITEM_ADD_DB_ERROR;
			break;
		}

		int currentTimestamp = (int)g_pServerInstance->GetCurrentTime();

		if (itemWithSameID.m_nItemID)
		{
			itemStatus = 0;
			itemInUse = 0;

			if (duration)
			{
				CUserInventoryItem itemToExtend;
				if (g_UserDatabase.GetFirstExtendableItemByItemID(userID, itemID, itemToExtend) < 0)
				{
					result = ITEM_ADD_DB_ERROR;
					break;
				}

				if (itemToExtend.m_nItemID)
				{
					int ret = ExtendItem(userID, user, itemToExtend, duration, true);
					if (ret == -1)
					{
						result = ITEM_ADD_DB_ERROR;
						break;
					}
					else if (ret == 1)
						continue;
				}
			}
		}
		else if (duration)
			duration = currentTimestamp + duration * CSO_24_HOURS_IN_MINUTES;

		// Category: 1 - pistols, 2 - shotguns, 3 - SMG, 4 - rifle, 5 - machine guns, 6 - equipment,  7 - class, 12 - costume, 13 - weapon parts, 
		int useType = g_pItemTable->GetCell<int>(OBFUSCATE("UseType"), to_string(itemID));
		int category = g_pItemTable->GetCell<int>("Category", to_string(itemID));
		// countable items with use button
		if ((category == 9 && (useType == 0 || useType == 2)) || (category == 8 && (useType == 0 || useType == 2)))
		{
			if (itemWithSameID.m_nItemID)
			{
				// update first item
				itemWithSameID.m_nCount += count;

				itemWithSameID.PushItem(updatedItems, itemWithSameID);
			}
			else
			{
				itemStatus = 0;
				itemInUse = 1;

				CUserInventoryItem item;
				item.PushItem(insertedItems, itemID, count, itemStatus, itemInUse, currentTimestamp, duration, 0, 0, 0, 0, 0, {}, 0, 0, lockStatus); // push new items to inventory
			}

			continue;
		}

		string className = g_pItemTable->GetCell<string>("ClassName", to_string(itemID));
		if (category == 12 || className == "Tattoo")
		{
			CUserCostumeLoadout loadout;
			if (g_UserDatabase.GetCostumeLoadout(userID, loadout) <= 0)
			{
				result = ITEM_ADD_DB_ERROR;
				break;
			}

			int zombieSkinType = -1;

			itemInUse = 0;

			if (className == "HeadCostume")
			{
				if (!loadout.m_nHeadCostumeID)
				{
					itemInUse = 1;
					loadout.m_nHeadCostumeID = itemID;
				}
			}
			else if (className == "BackCostume")
			{
				if (!loadout.m_nBackCostumeID)
				{
					itemInUse = 1;
					loadout.m_nBackCostumeID = itemID;
				}
			}
			else if (className == "ArmCostume")
			{
				if (!loadout.m_nArmCostumeID)
				{
					itemInUse = 1;
					loadout.m_nArmCostumeID = itemID;
				}
			}
			else if (className == "PelvisCostume")
			{
				if (!loadout.m_nPelvisCostumeID)
				{
					itemInUse = 1;
					loadout.m_nPelvisCostumeID = itemID;
				}
			}
			else if (className == "FaceCostume")
			{
				if (!loadout.m_nFaceCostumeID)
				{
					itemInUse = 1;
					loadout.m_nFaceCostumeID = itemID;
				}
			}
			else if (className == "ZombieSkinCostume")
			{
				zombieSkinType = g_pItemTable->GetCell<int>("ZombieSkin", to_string(itemID));
				if (zombieSkinType >= ZB_COSTUME_SLOT_COUNT_MAX)
				{
					Logger().Warn("CItemManager::AddItem: can't setup zb costume loadout (zombieSkinType >= ZB_COSTUME_SLOT_COUNT_MAX)!!!\n");
					continue;
				}
				else if (!loadout.m_ZombieSkinCostumeID.count(zombieSkinType) && !loadout.m_ZombieSkinCostumeID[zombieSkinType])
				{
					itemInUse = 1;
					loadout.m_ZombieSkinCostumeID[zombieSkinType] = itemID;
				}
			}
			else if (className == "Tattoo")
			{
				if (!loadout.m_nTattooID)
				{
					itemInUse = 1;
					loadout.m_nTattooID = itemID;
				}
			}
			else if (className == "PetCostume")
			{
				if (!loadout.m_nPetCostumeID)
				{
					itemInUse = 1;
					loadout.m_nPetCostumeID = itemID;
				}
			}

			if (g_UserDatabase.UpdateCostumeLoadout(userID, loadout, zombieSkinType) <= 0)
			{
				result = ITEM_ADD_DB_ERROR;
				break;
			}
		}

		if (className == "LobbyBG")
		{
			CUserCharacter character = {};
			character.lowFlag = UFLAG_LOW_NAMEPLATE;
			if (g_UserDatabase.GetCharacter(userID, character) <= 0)
			{
				result = ITEM_ADD_DB_ERROR;
				break;
			}

			if (character.nameplateID)
				itemInUse = 0;
			else if (user)
				user->UpdateNameplate(itemID);
			else
			{
				character.nameplateID = itemID;
				g_UserDatabase.UpdateCharacter(userID, character);
			}
		}
		else if (className == "zbRespawnEffect")
		{
			CUserCharacterExtended character = {};
			character.flag = EXT_UFLAG_ZBRESPAWNEFFECT;
			if (g_UserDatabase.GetCharacterExtended(userID, character) <= 0)
			{
				result = ITEM_ADD_DB_ERROR;
				break;
			}

			if (character.zbRespawnEffect)
				itemInUse = 0;
			else if (user)
				user->UpdateZbRespawnEffect(itemID);
			else
			{
				character.zbRespawnEffect = itemID;
				g_UserDatabase.UpdateCharacterExtended(userID, character);
			}
		}
		else if (className == "CombatInfoItem")
		{
			CUserCharacterExtended character = {};
			character.flag = EXT_UFLAG_KILLERMARKEFFECT;
			if (g_UserDatabase.GetCharacterExtended(userID, character) <= 0)
			{
				result = ITEM_ADD_DB_ERROR;
				break;
			}

			if (character.killerMarkEffect)
				itemInUse = 0;
			else if (user)
				user->UpdateKillerMarkEffect(itemID);
			else
			{
				character.killerMarkEffect = itemID;

				g_UserDatabase.UpdateCharacterExtended(userID, character);
			}
		}

		string resourceName = g_pItemTable->GetCell<string>("recourcename", to_string(itemID));
		if (resourceName.find("chatcolor_") != std::string::npos)
		{
			CUserCharacter character = {};
			character.highFlag = UFLAG_HIGH_CHATCOLOR;
			if (g_UserDatabase.GetCharacter(userID, character) <= 0)
			{
				result = ITEM_ADD_DB_ERROR;
				break;
			}

			if (!character.chatColorID)
			{
				if (user)
					user->UpdateChatColor(itemID);
				else
				{
					character.chatColorID = itemID;
					g_UserDatabase.UpdateCharacter(userID, character);
				}
			}
		}

		if (user)
		{
			if (itemID == 8357) // superRoom
			{
				IRoom* currentRoom = user->GetCurrentRoom();
				if (currentRoom != NULL)
				{
					if (user == currentRoom->GetHostUser()) // it's the room host, so add the superRoom flag
					{
						CRoomSettings* roomSettings = currentRoom->GetSettings();

						if (!roomSettings->superRoom)
						{
							roomSettings->superRoom = 1;

							for (auto u : currentRoom->GetUsers())
							{
								g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), roomSettings, 0, ROOM_LOWMID_SUPERROOM);
							}
						}
					}
				}
			}
			else if (itemID == 439) // BigHeadEvent
			{
				IRoom* currentRoom = user->GetCurrentRoom();
				if (currentRoom != NULL)
				{
					if (user == currentRoom->GetHostUser()) // it's the room host, so add the sd flag
					{
						CRoomSettings* roomSettings = currentRoom->GetSettings();

						if ((roomSettings->gameModeId == 3 || roomSettings->gameModeId == 4 || roomSettings->gameModeId == 5 || roomSettings->gameModeId == 15 || roomSettings->gameModeId == 24) && !roomSettings->sd)
						{
							roomSettings->sd = 1;

							for (auto u : currentRoom->GetUsers())
							{
								g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), roomSettings, 0, ROOM_LOWMID_SD);
							}
						}
					}
				}
			}
		}

		CUserInventoryItem item;
		item.PushItem(insertedItems, itemID, count, itemStatus, itemInUse, currentTimestamp, duration, 0, 0, 0, 0, 0, {}, 0, 0, lockStatus); // push new items to inventory
	}

	if (result == ITEM_ADD_DB_ERROR)
	{
		g_UserDatabase.CommitTransaction();
		return ITEM_ADD_DB_ERROR;
	}

	if (!(updatedItems.empty() && insertedItems.empty()))
	{
		if (!insertedItems.empty())
		{
			if (g_UserDatabase.AddInventoryItems(userID, insertedItems) <= 0)
			{
				g_UserDatabase.CommitTransaction();
				return ITEM_ADD_DB_ERROR;
			}
		}

		if (!updatedItems.empty())
		{
			if (g_UserDatabase.UpdateInventoryItems(userID, updatedItems, UITEM_FLAG_COUNT) <= 0)
			{
				g_UserDatabase.CommitTransaction();
				return ITEM_ADD_DB_ERROR;
			}
		}

		insertedItems.insert(insertedItems.end(), updatedItems.begin(), updatedItems.end());

		if (g_UserDatabase.CommitTransaction() == true)
		{
			if (user)
				g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), insertedItems);
		}
		else
			return ITEM_ADD_DB_ERROR;
	}
	else if (g_UserDatabase.CommitTransaction() == false)
		return ITEM_ADD_DB_ERROR;

	return result;
}

bool CItemManager::CanUseItem(const CUserInventoryItem& item)
{
	return item.m_nCount != 0 && item.m_nStatus == 0;
}

int CItemManager::UseItem(IUser* user, int slot, int additionalArg, int additionalArg2)
{
	CUserInventoryItem item;
	if (!g_UserDatabase.GetInventoryItemBySlot(user->GetID(), item.GameSlotToSlot(slot), item))
	{
		Logger().Info(OBFUSCATE("CItemManager::UseItem: item == NULL, slot: %d\n"), slot);
		return ITEM_USE_BAD_SLOT;
	}

	string className = g_pItemTable->GetCell<string>("ClassName", to_string(item.m_nItemID));
	string name = g_pItemTable->GetCell<string>("Name", to_string(item.m_nItemID));

	if (!CanUseItem(item))
	{
		// costume equip
		return ITEM_USE_WRONG_ITEM;
	}

	if (className == "LotteryKeyItem")
	{
		int openedDecoderCount = g_LuckyItemManager.OpenItemBox(user, item.m_nItemID, additionalArg);
		if (openedDecoderCount)
		{
			OnItemUse(user, item, openedDecoderCount);
		}
		return ITEM_USE_SUCCESS;
	}
	else if (className == "Requirement" || className == "RewardItem" || className == "NicknameChange")
	{
		OnItemUse(user, item, additionalArg);
		return ITEM_USE_SUCCESS;
	}

	switch (item.m_nItemID)
	{
	case 676:
	{
		CUserInventoryItem targetItem;
		if (!g_UserDatabase.GetInventoryItemBySlot(user->GetID(), item.GameSlotToSlot(additionalArg), targetItem))
			return ITEM_USE_BAD_SLOT;

		if (ExtendItem(user->GetID(), user, targetItem, additionalArg2, true))
			OnItemUse(user, item, additionalArg2);
		break;
	}
	default:
	{
		int category = g_pItemTable->GetCell<int>("Category", to_string(item.m_nItemID));
		if (category < 9 || category == 11) // if extentable item(should be moved to onitemuse)
		{
			int flag = 0;

			vector<CUserInventoryItem> items;
			if (item.m_nExpiryDate)
			{
				item.ConvertDurationToExpiryDate();

				g_UserDatabase.GetInventoryItemsByID(user->GetID(), item.m_nItemID, items);
				for (auto& i : items)
				{
					if (i.m_nExpiryDate && i.m_nStatus)
					{
						ExtendItem(user->GetID(), user, i, item.m_nExpiryDate);

						// remove item
						RemoveItem(user->GetID(), user, item);

						return ITEM_USE_SUCCESS;
					}
				}

				flag |= UITEM_FLAG_EXPIRYDATE;
			}

			item.m_nStatus = 1;
			flag |= UITEM_FLAG_STATUS;

			g_UserDatabase.UpdateInventoryItem(user->GetID(), item, flag);

			items.clear();
			item.PushItem(items, item);

			g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);
		}
		else
		{
			Logger().Warn("User '%s' tried to use unknown item (itemId: %d, category: %d, status: %d)\n", user->GetLogName(), item.m_nItemID, category, item.m_nStatus);
		}
	}
	}
	return ITEM_USE_SUCCESS;
}

bool CItemManager::OnItemUse(IUser* user, CUserInventoryItem& item, int count)
{
	if (item.m_nCount <= 0 || item.m_nCount < count)
	{
		return false;
	}

	item.m_nCount -= count;

	int rewardID = g_pItemTable->GetCell<int>("rewardID", to_string(item.m_nItemID));
	if (rewardID)
	{
		RewardNotice notice = {};
		for (int i = 0; i < count; i++)
		{
			notice += GiveReward(user->GetID(), user, rewardID, 0, true);
		}

		if (notice.status)
		{
			Reward* reward = GetRewardByID(rewardID);
			g_PacketManager.SendUMsgRewardNotice(user->GetExtendedSocket(), notice, reward->title, reward->description, reward->localized);
		}
	}

	if (item.m_nCount == 0)
	{
		RemoveItem(user->GetID(), user, item);
	}
	else // update item
	{
		vector<CUserInventoryItem> items;
		item.PushItem(items, item);

		g_UserDatabase.UpdateInventoryItem(user->GetID(), item, UITEM_FLAG_COUNT);

		g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);
	}

	return true;
}


bool CItemManager::RemoveItem(int userID, IUser* user, CUserInventoryItem& item)
{
	if (item.m_nSlot <= 0)
	{
		Logger().Warn("CItemManager::RemoveItem: cannot remove item, slot <= 0\n");
		return false;
	}

	if (item.IsItemDefaultOrPseudo())
	{
		// can't remove default item
		return false;
	}

	if (item.m_nStatus && item.m_nInUse)
	{
		vector<CUserInventoryItem> itemsWithSameID;
		g_UserDatabase.GetInventoryItemsByID(userID, item.m_nItemID, itemsWithSameID);

		for (auto& i : itemsWithSameID)
		{
			if (i.m_nSlot != item.m_nSlot && i.m_nInUse)
			{
				i.m_nStatus = 1;

				vector<CUserInventoryItem> items;

				i.PushItem(items, i); // push item to temp vector

				g_UserDatabase.UpdateInventoryItem(userID, i, UITEM_FLAG_STATUS);

				if (user)
					g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);

				break;
			}
		}
	}

	if (user)
	{
		if (item.m_nItemID == 8357) // superRoom
		{
			IRoom* currentRoom = user->GetCurrentRoom();
			if (currentRoom != NULL)
			{
				if (user == currentRoom->GetHostUser()) // it's the room host, so remove the superRoom flag
				{
					vector<CUserInventoryItem> itemsWithSameID;
					g_UserDatabase.GetInventoryItemsByID(userID, item.m_nItemID, itemsWithSameID);
					if (itemsWithSameID.size() == 1)
					{
						CRoomSettings* roomSettings = currentRoom->GetSettings();
						roomSettings->superRoom = 0;

						for (auto u : currentRoom->GetUsers())
						{
							g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), roomSettings, 0, ROOM_LOWMID_SUPERROOM);
						}
					}
				}
			}
		}
		else if (item.m_nItemID == 439) // BigHeadEvent
		{
			IRoom* currentRoom = user->GetCurrentRoom();
			if (currentRoom != NULL)
			{
				if (user == currentRoom->GetHostUser()) // it's the room host, so remove the sd flag
				{
					vector<CUserInventoryItem> itemsWithSameID;
					g_UserDatabase.GetInventoryItemsByID(userID, item.m_nItemID, itemsWithSameID);
					if (itemsWithSameID.size() == 1)
					{
						CRoomSettings* roomSettings = currentRoom->GetSettings();

						if (roomSettings->sd)
						{
							roomSettings->sd = 0;

							for (auto u : currentRoom->GetUsers())
							{
								g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), roomSettings, 0, ROOM_LOWMID_SD);
							}
						}
					}
				}
			}
		}
		else if (item.m_nItemID == 112) // C4Sound
		{
			IRoom* currentRoom = user->GetCurrentRoom();
			if (currentRoom != NULL)
			{
				if (user == currentRoom->GetHostUser()) // it's the room host, so remove the c4Timer flag
				{
					vector<CUserInventoryItem> itemsWithSameID;
					g_UserDatabase.GetInventoryItemsByID(userID, item.m_nItemID, itemsWithSameID);
					if (itemsWithSameID.size() == 1)
					{
						CRoomSettings* roomSettings = currentRoom->GetSettings();

						if (roomSettings->c4Timer)
						{
							roomSettings->c4Timer = 0;

							for (auto u : currentRoom->GetUsers())
							{
								g_PacketManager.SendRoomUpdateSettings(u->GetExtendedSocket(), roomSettings, 0, ROOM_LOWMID_C4TIMER);
							}
						}
					}
				}
			}
		}
	}

	string className = g_pItemTable->GetCell<string>("ClassName", to_string(item.m_nItemID));
	if (className == "LobbyBG")
	{
		CUserCharacter character = {};
		character.lowFlag = UFLAG_LOW_NAMEPLATE;
		if (g_UserDatabase.GetCharacter(userID, character) <= 0)
		{
			Logger().Warn("CItemManager::RemoveItem: cannot remove item, database error\n");
			return false;
		}

		if (character.nameplateID == item.m_nItemID)
		{
			if (user)
			{
				user->UpdateNameplate(0);
			}
			else
			{
				character.nameplateID = 0;
				g_UserDatabase.UpdateCharacter(userID, character);
			}
		}
	}
	else if (className == "zbRespawnEffect")
	{
		CUserCharacterExtended characterExt = {};
		characterExt.flag = EXT_UFLAG_ZBRESPAWNEFFECT;
		if (g_UserDatabase.GetCharacterExtended(userID, characterExt) <= 0)
		{
			Logger().Warn("CItemManager::RemoveItem: cannot remove item, database error\n");
			return false;
		}

		if (characterExt.zbRespawnEffect == item.m_nItemID)
		{
			if (user)
			{
				user->UpdateZbRespawnEffect(0);
			}
			else
			{
				characterExt.zbRespawnEffect = 0;
				g_UserDatabase.UpdateCharacterExtended(userID, characterExt);
			}
		}
	}
	else if (className == "CombatInfoItem")
	{
		CUserCharacterExtended characterExt = {};
		characterExt.flag = EXT_UFLAG_KILLERMARKEFFECT;
		if (g_UserDatabase.GetCharacterExtended(userID, characterExt) <= 0)
		{
			Logger().Warn("CItemManager::RemoveItem: cannot remove item, database error\n");
			return false;
		}

		if (characterExt.killerMarkEffect == item.m_nItemID)
		{
			if (user)
			{
				user->UpdateKillerMarkEffect(0);
			}
			else
			{
				characterExt.killerMarkEffect = 0;
				g_UserDatabase.UpdateCharacterExtended(userID, characterExt);
			}
		}
	}

	string resourceName = g_pItemTable->GetCell<string>("recourcename", to_string(item.m_nItemID));
	if (resourceName.find("chatcolor_") != std::string::npos)
	{
		CUserCharacter character = {};
		character.highFlag = UFLAG_HIGH_CHATCOLOR;
		if (g_UserDatabase.GetCharacter(userID, character) <= 0)
		{
			Logger().Warn("CItemManager::RemoveItem: cannot remove item, database error\n");
			return false;
		}

		if (character.chatColorID == item.m_nItemID)
		{
			if (user)
			{
				user->UpdateChatColor(0);
			}
			else
			{
				character.chatColorID = 0;
				g_UserDatabase.UpdateCharacter(userID, character);
			}
		}
	}

	item.Reset();

	g_UserDatabase.UpdateInventoryItem(userID, item, UITEM_FLAG_ALL);

	if (user)
	{
		vector<CUserInventoryItem> items;
		item.PushItem(items, item); // push null item to temp vector
		g_PacketManager.SendInventoryRemove(user->GetExtendedSocket(), items);
	}

	return true;
}

bool CItemManager::OpenDecoder(IUser* user, int count, int slot)
{
	int status = UseItem(user, slot, count);

	if (status < 0)
		g_PacketManager.SendItemOpenDecoderErrorReply(user->GetExtendedSocket(), ItemBoxError::FAIL_USEITEM);

	return true;
}

int CItemManager::ExtendItem(int userID, IUser* user, CUserInventoryItem& item, int newExpiryDate, bool duration)
{
	if (item.m_nExpiryDate == 0)
		return 0; // can't extend permanent item

	if (!duration)
	{
		int addontialTime = newExpiryDate - (int)g_pServerInstance->GetCurrentTime();
		item.m_nExpiryDate += addontialTime;
	}
	else if (duration && item.m_nInUse == 0)
		item.m_nExpiryDate += newExpiryDate;
	else if (newExpiryDate == 0 || newExpiryDate + ((item.m_nExpiryDate - item.m_nObtainDate) / 1440) >= 999)	//if 0 or (new + (ExpiryDate - ObtainDate) >= 999)
		item.m_nExpiryDate = 0;
	else
	{
		int addontialTime = newExpiryDate * 1440; // 86400
		item.m_nExpiryDate += addontialTime;
	}

	if (g_UserDatabase.UpdateInventoryItem(userID, item, UITEM_FLAG_EXPIRYDATE) <= 0)
		return -1; // db error

	vector<CUserInventoryItem> items;
	item.PushItem(items, item);

	// update client inventory
	if (user)
		g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);

	return 1;
}

bool CItemManager::OnDisassembleRequest(IUser* user, CReceivePacket* msg)
{
	int count = msg->ReadUInt8();
	int unk = msg->ReadUInt8();
	for (int i = 0; i < count; i++)
	{
		int unk2 = msg->ReadUInt8();
		int slot = msg->ReadUInt16();
		int unk3 = msg->ReadUInt32(); // че за говно?

		Logger().Warn("CItemManager::OnDisassembleRequest: %d, %d, %d, %d\n", unk, unk2, slot, unk3);

		CUserInventoryItem item;
		g_UserDatabase.GetInventoryItemBySlot(user->GetID(), item.GameSlotToSlot(slot), item);
		if (item.m_nSlot)
		{
			RemoveItem(user->GetID(), user, item);
		}
		else
		{
			Logger().Error("CItemManager::OnDisassembleRequest: could not find item with %d slot\n", slot);
		}
	}

	return true;
}

Reward* CItemManager::GetRewardByID(int rewardID)
{
	for (int i = 0; i < (int)m_Rewards.size(); i++)
	{
		if (m_Rewards[i].rewardId == rewardID)
		{
			return &m_Rewards[i];
		}
	}

	return NULL;
}

std::vector<WeaponPaint> CItemManager::GetWeaponPaints()
{
	return m_WeaponPaints;
}

RewardNotice CItemManager::GiveReward(int userID, IUser* user, int rewardID, int rewardSelectID, bool ignoreClient, int randomRepeatCount)
{
	RewardNotice rewardNotice;
	rewardNotice.rewardId = 0;
	rewardNotice.exp = 0;
	rewardNotice.honorPoints = 0;
	rewardNotice.points = 0;
	rewardNotice.status = true;

	Reward* reward = GetRewardByID(rewardID);
	if (reward == NULL)
	{
		rewardNotice.status = false;
		return rewardNotice;
	}

	if (user == NULL)
	{
		g_UserDatabase.UpdateRewardNotices(userID, rewardID);

		return rewardNotice;
	}

	rewardNotice.rewardId = rewardID;
	string title = reward->title;
	string description = reward->description;
	if (reward->select == true && rewardSelectID != 0)
	{
		int status = AddItem(userID, user, reward->items[rewardSelectID - 1].itemID, reward->items[rewardSelectID - 1].count, reward->items[rewardSelectID - 1].duration);
		if (status < 0)
		{
			// add to storage
		}
	}
	else if (reward->select == true)
	{
		g_PacketManager.SendUMsgRewardSelect(user->GetExtendedSocket(), reward);
		rewardNotice.status = false;
		return rewardNotice;
	}
	else
	{
		for (RewardItemsRandom rewardItemsRandom : reward->randomItems)
		{
			bool gotItem = false;
			while (!gotItem)
			{
				int lastItemRandomIdx = 1;
				for (RewardItemRandom& rewardItemRandom : rewardItemsRandom.items)
				{
					if (yesOrNo(rewardItemRandom.chance) || lastItemRandomIdx == rewardItemsRandom.items.size())
					{
						Randomer randomItem(rewardItemRandom.items.size() - 1);
						rewardNotice.items.push_back(rewardItemRandom.items[randomItem()]);
						if ((rewardNotice.items.back().duration > 0 || rewardNotice.items.back().duration != -1) && rewardItemsRandom.durations.size())
						{
							bool gotDuration = false;
							while (!gotDuration)
							{
								int lastItemRandomDurationIdx = 1;
								for (RewardItemRandomDuration& rewardItemRandomDuration : rewardItemsRandom.durations)
								{
									if (yesOrNo(rewardItemRandomDuration.chance) || lastItemRandomDurationIdx == rewardItemsRandom.durations.size())
									{
										Randomer randomDuration(rewardItemRandomDuration.durations.size() - 1);
										rewardNotice.items.back().duration = rewardItemRandomDuration.durations[randomDuration()];
										gotDuration = true;
										break;
									}
									lastItemRandomDurationIdx++;
								}
							}
						}
						int status = AddItem(userID, user, rewardNotice.items.back().itemID, rewardNotice.items.back().count, rewardNotice.items.back().duration);
						if (status < 0)
						{
							// add to storage
						}

						randomRepeatCount--;
						if (randomRepeatCount > 0)
						{
							break;
						}

						gotItem = true;
						break;
					}
					lastItemRandomIdx++;
				}
			}
		}

		if (reward->items.size())
		{
			vector<RewardItem> rewardItems;
			for (RewardItem& item : reward->items)
			{
				item.lockStatus = 0;
				rewardItems.push_back(item);
				rewardNotice.items.push_back(item);
			}

			int status = AddItems(userID, user, rewardItems);
			if (status < 0)
			{
				// add to storage
			}
		}
	}

	if (reward->exp.size())
	{
		Randomer randomerExp(reward->exp.size() - 1);
		rewardNotice.exp = reward->exp[randomerExp()];

		user->UpdateExp(rewardNotice.exp);
	}
	if (reward->points.size())
	{
		Randomer randomerPoints(reward->points.size() - 1);
		rewardNotice.points = reward->points[randomerPoints()];

		user->UpdatePoints(rewardNotice.points);
	}
	if (reward->honorPoints.size())
	{
		Randomer randomerHonorPoints(reward->honorPoints.size() - 1);
		rewardNotice.honorPoints = reward->honorPoints[randomerHonorPoints()];

		user->UpdateHonorPoints(rewardNotice.honorPoints);
	}

	if (reward->select == false && !ignoreClient)
	{
		//if (!reward->zbsReward)
		//	g_PacketManager.SendUMsgRewardNotice(user->GetExtendedSocket(), rewardNotice, reward->title, reward->description);
		g_PacketManager.SendUMsgRewardNotice(user->GetExtendedSocket(), rewardNotice, reward->title, reward->description, reward->localized);

		if (user->IsPlaying())
			g_PacketManager.SendUMsgRewardNotice(user->GetExtendedSocket(), rewardNotice, reward->title, reward->description, reward->localized, true);
	}

	return rewardNotice;
}

void CItemManager::OnNicknameChangeUse(IUser* user, string newNickname)
{
	vector<CUserInventoryItem> items;
	if (!g_UserDatabase.GetInventoryItemsByID(user->GetID(), 65/*nickname changer*/, items))
	{
		g_PacketManager.SendUpdateInfoNicknameChangeReply(user->GetExtendedSocket(), 8);
		return;
	}

	if (g_ItemManager.CanUseItem(items[0]))
	{
		int replyCode = g_UserManager.ChangeUserNickname(user, newNickname);
		g_PacketManager.SendUpdateInfoNicknameChangeReply(user->GetExtendedSocket(), replyCode);

		if (replyCode < 0)
			return;

		g_ItemManager.UseItem(user, items[0].GetGameSlot());
	}
	else
	{
		g_PacketManager.SendUpdateInfoNicknameChangeReply(user->GetExtendedSocket(), 8);
	}
}

bool CItemManager::OnDailyRewardsRequest(IUser* user, int requestID)
{
	switch (requestID)
	{
	case 0:
		Logger().Info("CItemManager::OnDailyRewardsRequest: received type 0\n");
		break;
	case 1:
		Logger().Info("CItemManager::OnDailyRewardsRequest: received type 1\n");
		break;
	default:
		Logger().Warn("CItemManager::OnDailyRewardsRequest: unknown request: %d\n", requestID);
		break;
	}

	return true;
}

void CItemManager::InsertExp(IUser* user, CUserInventoryItem& targetItem, vector<CUserInventoryItem>& items)
{
	if (!items.empty())
	{
		static vector<int> itemsExp{0, 10, 20, 50, 100, 200, 500};
		int totalExp = 0;

		int grade = g_pItemTable->GetCell<int>("ItemGrade", to_string(targetItem.m_nItemID));
		vector<int> row = m_pReinforceMaxExpTable->GetRow<int>(to_string(targetItem.m_nEnhancementLevel));
		int expToUpgrade = row[grade];

		for (auto& i : items)
		{
			int grade = g_pItemTable->GetCell<int>("ItemGrade", to_string(i.m_nItemID));
			switch (i.m_nItemID)
			{
			case 8481: // WeaponReinforceExp100
				grade = 4;
				break;
			case 8482: // WeaponReinforceExp200
				grade = 5;
				break;
			case 8566: // WeaponReinforceExp50
				grade = 3;
				break;
			}

			if (!grade)
			{
				string targetItemName = g_pItemTable->GetCell<string>("Name", to_string(targetItem.m_nItemID));
				string enhName = g_pItemTable->GetCell<string>("Name", to_string(i.m_nItemID));
				if (enhName.find("Enh") == 0)
				{
					g_PacketManager.SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), "this material is under dev");
					// TODO: get itemID of original weapon from enhance material (like Enhbuffm4 -> buffm4 -> 809)
					//grade = g_pItemTable->GetRowValueByItemName<int>("Grade", enhName.substr(3));
				}
			}

			totalExp += (int)(i.m_nEnhancementLevel ? itemsExp[grade] * i.m_nEnhancementLevel * 1.4 : itemsExp[grade]);
			if (targetItem.m_nItemID == i.m_nItemID)
				totalExp *= 2; // multiply by 2 exp if target item and material item are the same

			OnItemUse(user, i); // used as exp for target item
		}

		targetItem.m_nEnhancementExp += totalExp > expToUpgrade ? expToUpgrade : totalExp; // wrong???
		if (targetItem.m_nEnhancementExp > 0)
		{
			g_UserDatabase.UpdateInventoryItem(user->GetID(), targetItem, UITEM_FLAG_ENHANCEMENTEXP);

			vector<CUserInventoryItem> items;
			targetItem.PushItem(items, targetItem);

			// update client inventory
			g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);
		}
	}
}

int GetEnhanceLevel(vector<int>& itemEnhanceAttributes)
{
	int itemEnhLevel = 0;
	for (int i = 0; i < (int)itemEnhanceAttributes.size(); i++)
	{
		if (i == 6 && itemEnhanceAttributes[i])
		{
			itemEnhLevel += 1;
		}
		else
		{
			itemEnhLevel += itemEnhanceAttributes[i];
		}
	}

	return itemEnhLevel;
}

bool CItemManager::OnEnhancementRequest(IUser* user, CReceivePacket* msg)
{
	int requestID = msg->ReadUInt8();

	EnhResult result;
	//result.repeat = false;
	result.itemSlot = 0;
	result.enhAttribute = -1;
	result.enhLevel = 0;
	result.status = EnhanceStatus::ENHANCE_SYSTEMERROR;

	switch (requestID)
	{
	case 0: // insert exp and enhance
	{
		int a2 = msg->ReadUInt8();
		int EnhancementTargetItemSlot = msg->ReadUInt16();
		int EXPMaterialCount = msg->ReadUInt8();

		vector<CUserInventoryItem> expMatItems;
		for (int i = 0; i < EXPMaterialCount; i++)
		{
			int unk = msg->ReadUInt8();

			CUserInventoryItem item;
			g_UserDatabase.GetInventoryItemBySlot(user->GetID(), item.GameSlotToSlot(msg->ReadUInt16()), item);
			if (!item.m_nSlot)
			{
				g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
				return false;
			}

			expMatItems.push_back(item);
		}

		CUserInventoryItem targetItem;
		g_UserDatabase.GetInventoryItemBySlot(user->GetID(), targetItem.GameSlotToSlot(EnhancementTargetItemSlot), targetItem);
		if (!targetItem.m_nSlot)
		{
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		if (targetItem.m_nEnhancementLevel >= 9)
		{
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		if (m_pReinforceMaxLvTable->GetRowIdx(to_string(targetItem.m_nItemID)) < 0)
		{
			//result.msg = "CSO_REINFORCE_ERR_NOT_REINFORCE_ITEM";
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		vector<int> itemEnhanceInfo;
		for (int i = 1; i < 8; i++)
		{
			itemEnhanceInfo.push_back(m_pReinforceMaxLvTable->GetCell<int>(i, to_string(targetItem.m_nItemID)));
		}

		if (itemEnhanceInfo.empty())
		{
			//result.msg = "CSO_REINFORCE_ERR_NOT_REINFORCE_ITEM";
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		int itemEnhanceValue = targetItem.m_nEnhanceValue;
		vector<int> itemEnhanceAttributes;

		itemEnhanceAttributes.push_back(itemEnhanceValue & 0xF);
		itemEnhanceAttributes.push_back((itemEnhanceValue & 0xF0) >> 4);
		itemEnhanceAttributes.push_back((itemEnhanceValue & 0xF00) >> 8);
		itemEnhanceAttributes.push_back((itemEnhanceValue & 0xF000) >> 12);
		itemEnhanceAttributes.push_back((itemEnhanceValue & 0xF0000) >> 16);
		itemEnhanceAttributes.push_back((itemEnhanceValue & 0xF00000) >> 20);
		itemEnhanceAttributes.push_back((itemEnhanceValue & 0xF000000) >> 24);

		// get current item enhance level
		int itemEnhLevel = GetEnhanceLevel(itemEnhanceAttributes);

		if (itemEnhLevel >= 9)
		{
			//result.msg = "CSO_REINFORCE_ERR_NEED_MORE_POWERFUL_MATERIAL";
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		int itemEnhLevelMax = m_pReinforceMaxLvTable->GetCell<int>("TotalMaxLv", to_string(targetItem.m_nItemID));
		//int itemEnhLevelMax = g_pItemTable->GetRowValueByItemID<int>("EnhWeapon", to_string(targetItem->m_nItemID));
		if (/*antiEnhAttribute == -1 && */itemEnhLevel >= itemEnhLevelMax)
		{
			//result.msg = "CSO_REINFORCE_ERR_MAX_LEVEL";
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		int grade = g_pItemTable->GetCell<int>("ItemGrade", to_string(targetItem.m_nItemID)); // get target item grade
		vector<int> row = m_pReinforceMaxExpTable->GetRow<int>(to_string(targetItem.m_nEnhancementLevel));
		int expToUpgrade = row[grade]; // get max exp upgrade value for the target item

		InsertExp(user, targetItem, expMatItems);

		int enhanceChancePercentage = (float)targetItem.m_nEnhancementExp / (float)expToUpgrade * 100; // calc chance for enhance

		Logger().Info("OnEnhance: itemID: %d, expToUpgrade: %d, enhExp: %d, enhChance: %d\n", targetItem.m_nItemID, expToUpgrade, targetItem.m_nEnhancementExp, enhanceChancePercentage);

		int flag = 0;

		int enhAttributeIndex = -1;
		if (yesOrNo(enhanceChancePercentage))
		{
			result.status = EnhanceStatus::ENHANCE_SUCCESS;

			targetItem.m_nEnhancementLevel += 1; // add +1 to item enchance lvl
			targetItem.m_nEnhancementExp = 0; // reset enhance exp

			flag |= UITEM_FLAG_ENHANCEMENTLEVEL | UITEM_FLAG_ENHANCEMENTEXP;
		}
		else
		{
			result.status = EnhanceStatus::ENHANCE_FAILURE;

			targetItem.m_nEnhancementExp = 0; // reset enhance exp

			flag |= UITEM_FLAG_ENHANCEMENTEXP;
		}

		if (itemEnhLevel == 8)
		{
			enhAttributeIndex = 6;
		}

		Randomer randomItemEnhAttribute(itemEnhanceInfo.size() - 2);

		bool gotEnh = false;
		while (!gotEnh)
		{
			if (enhAttributeIndex == -1)
			{
				enhAttributeIndex = (int)randomItemEnhAttribute();
			}

			int enhAttributeMax = itemEnhanceInfo[enhAttributeIndex];

			// swap damage and accuracy index to fix attribute mismatch
			if (enhAttributeIndex == 0)
			{
				enhAttributeIndex = 1;
			}
			else if (enhAttributeIndex == 1)
			{
				enhAttributeIndex = 0;
			}

			int enhAttributeLevel = itemEnhanceAttributes[enhAttributeIndex];

			if (result.status == EnhanceStatus::ENHANCE_SUCCESS && enhAttributeMax && enhAttributeLevel < enhAttributeMax)
			{
				itemEnhanceAttributes[enhAttributeIndex]++;
				gotEnh = true;
			}
			else if (result.status == EnhanceStatus::ENHANCE_LEVELDOWN && enhAttributeMax && enhAttributeLevel)
			{
				itemEnhanceAttributes[enhAttributeIndex]--;
				gotEnh = true;
			}
			else if (result.status == EnhanceStatus::ENHANCE_FAILURE && enhAttributeMax)
			{
				gotEnh = true;
			}

			if (!gotEnh)
				enhAttributeIndex = -1;
		}

		result.enhAttribute = enhAttributeIndex;

		// recalculate enhance level
		itemEnhLevel = GetEnhanceLevel(itemEnhanceAttributes);
		result.enhLevel = itemEnhLevel;

		targetItem.m_nEnhanceValue = itemEnhanceAttributes[0] | (itemEnhanceAttributes[1] << 4) | (itemEnhanceAttributes[2] << 8) | (itemEnhanceAttributes[3] << 12) | (itemEnhanceAttributes[4] << 16) | (itemEnhanceAttributes[5] << 20) | (itemEnhanceAttributes[6] << 24);

		flag |= UITEM_FLAG_ENHANCEVALUE;

		// update client item
		vector<CUserInventoryItem> items;
		targetItem.PushItem(items, targetItem);
		if (g_UserDatabase.UpdateInventoryItem(user->GetID(), targetItem, flag) <= 0)
		{
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);

		result.itemSlot = EnhancementTargetItemSlot;

		g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);

		// send system notification
		/*if (result.enhLevel == 8)
		{
			for (auto u : g_UserManager.users)
			{
				g_PacketManager.SendUMsgSystemReply(u->GetExtendedSocket(), UMsgPacketType::SystemReply_Green, SystemReplyMsg::REINFORCE_MAX_SUCCESS, vector<string> { user->GetData()->gameName.c_str(), to_string(itemToEnhance->m_nItemID) });
			}
		}*/

		//Enhance(EnhancementTargetItemID);
		break;
	}
	case 1: // insert exp // TODO: obsolete?
	{
		int a2 = msg->ReadUInt8();
		int EnhancementTargetItemSlot = msg->ReadUInt16();
		int EXPMaterialCount = msg->ReadUInt8();
		int a3 = msg->ReadUInt8();
		vector<CUserInventoryItem> items;

		CUserInventoryItem targetItem;
		g_UserDatabase.GetInventoryItemBySlot(user->GetID(), targetItem.GameSlotToSlot(EnhancementTargetItemSlot), targetItem);
		if (!targetItem.m_nSlot)
		{
			return false;
		}

		for (int i = 0; i < EXPMaterialCount; i++)
		{
			CUserInventoryItem item;
			g_UserDatabase.GetInventoryItemBySlot(user->GetID(), item.GameSlotToSlot(msg->ReadUInt16()), targetItem);
			if (!item.m_nSlot)
			{
				g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
				return false;
			}

			items.push_back(item);

			int unk = msg->ReadUInt8();
		}

		if (items.size())
		{
			static vector<int> itemsExp{0, 10, 20, 50, 100, 200, 500};
			int totalExp = 0;
			for (auto& i : items)
			{
				int grade = g_pItemTable->GetCell<int>("ItemGrade", to_string(i.m_nItemID));
				if (!grade)
				{
					switch (i.m_nItemID)
					{
					case 8481: // WeaponReinforceExp100
						grade = 4;
						break;
					case 8482: // WeaponReinforceExp200
						grade = 5;
						break;
					case 8566: // WeaponReinforceExp50
						grade = 3;
						break;
					}
				}

				totalExp += (int)(i.m_nEnhancementLevel ? itemsExp[grade] * i.m_nEnhancementLevel * 1.4 : itemsExp[grade]);
				if (targetItem.m_nItemID == i.m_nItemID)
					totalExp *= 2; // multiply by 2 exp if target item and material item are the same

				OnItemUse(user, i); // used as exp for target item
			}

			int grade = g_pItemTable->GetCell<int>("ItemGrade", to_string(targetItem.m_nItemID));
			vector<int> row = m_pReinforceMaxExpTable->GetRow<int>(to_string(targetItem.m_nEnhancementLevel));
			int expToUpgrade = row[grade];

			targetItem.m_nEnhancementExp += totalExp > expToUpgrade ? expToUpgrade : totalExp; // wrong???
			if (targetItem.m_nEnhancementExp > 0)
			{
				g_UserDatabase.UpdateInventoryItem(user->GetID(), targetItem, UITEM_FLAG_ENHANCEMENTEXP);

				vector<CUserInventoryItem> items;
				targetItem.PushItem(items, targetItem);

				// update client inventory
				g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);
			}
		}

		result.status = EnhanceStatus::ENHANCE_INSERT_EXP;

		g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);

		break;
	}
	case 2: // decrease
	{
		int a1 = msg->ReadUInt8();
		int enhanceTargetItemSlot = msg->ReadUInt16();

		CUserInventoryItem targetItem;
		g_UserDatabase.GetInventoryItemBySlot(user->GetID(), targetItem.GameSlotToSlot(enhanceTargetItemSlot), targetItem);
		if (!targetItem.m_nItemID)
		{
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		int a2 = msg->ReadUInt8();
		int enhanceMaterialSlot = msg->ReadUInt16();

		CUserInventoryItem materialItem;
		g_UserDatabase.GetInventoryItemBySlot(user->GetID(), materialItem.GameSlotToSlot(enhanceMaterialSlot), materialItem);
		if (!materialItem.m_nItemID)
		{
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		int itemEnhanceValue = targetItem.m_nEnhanceValue;
		vector<int> itemEnhanceAttributes;

		itemEnhanceAttributes.push_back(itemEnhanceValue & 0xF);
		itemEnhanceAttributes.push_back((itemEnhanceValue & 0xF0) >> 4);
		itemEnhanceAttributes.push_back((itemEnhanceValue & 0xF00) >> 8);
		itemEnhanceAttributes.push_back((itemEnhanceValue & 0xF000) >> 12);
		itemEnhanceAttributes.push_back((itemEnhanceValue & 0xF0000) >> 16);
		itemEnhanceAttributes.push_back((itemEnhanceValue & 0xF00000) >> 20);
		itemEnhanceAttributes.push_back((itemEnhanceValue & 0xF000000) >> 24);

		// get current item enhance level
		int itemEnhLevel = GetEnhanceLevel(itemEnhanceAttributes);
		if (itemEnhLevel <= 0)
		{
			// nothing to decrease
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		if (m_pReinforceMaxLvTable->GetRowIdx(to_string(targetItem.m_nItemID)) < 0)
		{
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		vector<int> itemEnhanceInfo;
		for (int i = 1; i < 8; i++)
		{
			itemEnhanceInfo.push_back(m_pReinforceMaxLvTable->GetCell<int>(i, to_string(targetItem.m_nItemID)));
		}

		// should never get here
		if (itemEnhanceInfo.empty())
		{
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		int antiEnhAttribute = -1;
		int materialItemType = materialItem.m_nItemID;
		switch (materialItemType)
		{
		case ENH_ANTI:
		case ENH_ANIT1GRADE:
			break;
		case ENH_ANTIDAMAGE:
			antiEnhAttribute = 0;
			break;
		case ENH_ANTIWEIGHT:
			antiEnhAttribute = 3;
			break;
		case ENH_ANTIACCURACY:
			antiEnhAttribute = 1;
			break;
		case ENH_ANTIFIRERATE:
			antiEnhAttribute = 4;
			break;
		case ENH_ANTIKICKBACK:
			antiEnhAttribute = 2;
			break;
		case ENH_ANTIAMMO:
			antiEnhAttribute = 5;
			break;
		default:
			g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
			return false;
		}

		if (materialItemType == ENH_ANTI)
		{
			result.status = EnhanceStatus::ENHANCE_FULLLEVELDOWN;

			targetItem.m_nEnhanceValue = 0;
			targetItem.m_nEnhancementLevel = 0;
		}
		else if (materialItemType == ENH_ANIT1GRADE)
		{
			Randomer randomAttribute(6);
			bool gotEnh = false;
			while (!gotEnh)
			{
				antiEnhAttribute = (int)randomAttribute();
				if (itemEnhanceAttributes[antiEnhAttribute])
				{
					itemEnhanceAttributes[antiEnhAttribute] = 0;
					targetItem.m_nEnhancementLevel -= itemEnhanceAttributes[antiEnhAttribute];
					gotEnh = true;
				}
			}

			result.status = EnhanceStatus::ENHANCE_LEVELDOWN;
		}
		else
		{
			if (!itemEnhanceAttributes[antiEnhAttribute])
			{
				// CSO_WeaponEnhance_CantAddEnhacer_Anti_ZeroLevel
				g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);
				return false;
			}

			targetItem.m_nEnhancementLevel -= itemEnhanceAttributes[antiEnhAttribute];
			itemEnhanceAttributes[antiEnhAttribute] = 0;

			result.status = EnhanceStatus::ENHANCE_LEVELDOWN;
		}

		result.enhAttribute = antiEnhAttribute;
		result.itemSlot = enhanceMaterialSlot;

		// recalculate enhance level
		// TODO: don't calculate lvl if full reset item used?
		itemEnhLevel = GetEnhanceLevel(itemEnhanceAttributes);
		result.enhLevel = itemEnhLevel;

		targetItem.m_nEnhanceValue = itemEnhanceAttributes[0] | (itemEnhanceAttributes[1] << 4) | (itemEnhanceAttributes[2] << 8) | (itemEnhanceAttributes[3] << 12) | (itemEnhanceAttributes[4] << 16) | (itemEnhanceAttributes[5] << 20) | (itemEnhanceAttributes[6] << 24);

		OnItemUse(user, materialItem);

		g_UserDatabase.UpdateInventoryItem(user->GetID(), targetItem, UITEM_FLAG_ENHANCEMENTLEVEL | UITEM_FLAG_ENHANCEVALUE);

		vector<CUserInventoryItem> items;
		targetItem.PushItem(items, targetItem);

		// update client inventory
		g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);

		g_PacketManager.SendItemEnhanceResult(user->GetExtendedSocket(), result);

		break;
	}
	default:
		Logger().Warn("CItemManager::OnEnhancementRequest: unknown request: %d\n", requestID);
		break;
	}

	return true;
}

bool CItemManager::OnWeaponPaintRequest(IUser* user, CReceivePacket* msg)
{
	int weaponSlot = msg->ReadUInt16();
	int paintSlot = msg->ReadUInt16();

	CUserInventoryItem weapon;
	CUserInventoryItem paint;

	g_UserDatabase.GetInventoryItemBySlot(user->GetID(), weapon.GameSlotToSlot(weaponSlot), weapon);
	g_UserDatabase.GetInventoryItemBySlot(user->GetID(), paint.GameSlotToSlot(paintSlot), paint);

	if (!weapon.m_nItemID || !paint.m_nItemID)
		return false;

	// If weapon can't be painted, don't use it
	auto it = std::find_if(m_WeaponPaints.begin(), m_WeaponPaints.end(), [weapon](const WeaponPaint& weaponPaint) { return weaponPaint.itemID == weapon.m_nItemID; });
	if (it == m_WeaponPaints.end())
		return false;

	// If paint can't be used on this weapon, don't use it
	std::vector<int> paintIDs = m_WeaponPaints.at(it - m_WeaponPaints.begin()).paintIDs;
	if (std::find(paintIDs.begin(), paintIDs.end(), paint.m_nItemID) == paintIDs.end())
		return false;

	// If paint is already in the list, don't use it
	if (std::find(weapon.m_nPaintIDList.begin(), weapon.m_nPaintIDList.end(), paint.m_nItemID) != weapon.m_nPaintIDList.end())
		return false;

	weapon.m_nPaintID = paint.m_nItemID;
	weapon.m_nPaintIDList.push_back(paint.m_nItemID);

	g_UserDatabase.UpdateInventoryItem(user->GetID(), weapon, UITEM_FLAG_PAINTID | UITEM_FLAG_PAINTIDLIST);

	vector<CUserInventoryItem> items;
	weapon.PushItem(items, weapon);

	// update client inventory
	g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);

	// remove paint item
	OnItemUse(user, paint);

	g_PacketManager.SendItemWeaponPaintReply(user->GetExtendedSocket());

	return true;
}

bool CItemManager::OnWeaponPaintSwitchRequest(IUser* user, CReceivePacket* msg)
{
	int weaponSlot = msg->ReadUInt16();
	int paintID = msg->ReadUInt16();

	CUserInventoryItem weapon;

	g_UserDatabase.GetInventoryItemBySlot(user->GetID(), weapon.GameSlotToSlot(weaponSlot), weapon);

	if (!weapon.m_nItemID)
		return false;

	// If weapon can't be painted, don't use it
	auto it = std::find_if(m_WeaponPaints.begin(), m_WeaponPaints.end(), [weapon](const WeaponPaint& weaponPaint) { return weaponPaint.itemID == weapon.m_nItemID; });
	if (it == m_WeaponPaints.end())
		return false;

	// If paint can't be used on this weapon, don't use it
	std::vector<int> paintIDs = m_WeaponPaints.at(it - m_WeaponPaints.begin()).paintIDs;
	if (std::find(paintIDs.begin(), paintIDs.end(), paintID) == paintIDs.end())
		return false;

	string paintClassName = g_pItemTable->GetCell<string>("ClassName", to_string(paintID));

	if (paintClassName != "WeaponPaintRemoveItem")
	{
		// If paint isn't in the list, don't switch
		if (std::find(weapon.m_nPaintIDList.begin(), weapon.m_nPaintIDList.end(), paintID) == weapon.m_nPaintIDList.end())
			return false;
	}
	else
		paintID = 0;

	weapon.m_nPaintID = paintID;

	g_UserDatabase.UpdateInventoryItem(user->GetID(), weapon, UITEM_FLAG_PAINTID);

	vector<CUserInventoryItem> items;
	weapon.PushItem(items, weapon);

	// update client inventory
	g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);

	g_PacketManager.SendItemWeaponPaintReply(user->GetExtendedSocket());

	return true;
}

bool CItemManager::OnPartEquipRequest(IUser* user, CReceivePacket* msg)
{
	int type = msg->ReadUInt8();
	switch (type)
	{
		//case 0: // equip part
		//	break;
	case 1: // unequip part
	{
		int weaponSlot = msg->ReadUInt16();

		CUserInventoryItem weapon;
		g_UserDatabase.GetInventoryItemBySlot(user->GetID(), weapon.GameSlotToSlot(weaponSlot), weapon);

		if (!weapon.m_nItemID)
			return false;

		int partID = msg->ReadUInt8();
		switch (partID)
		{
		case 0:
			weapon.m_nPartSlot1 = 0;
			break;
		case 1:
			weapon.m_nPartSlot2 = 0;
			break;
		default:
			Logger().Error("OnPartEquipRequest: wrong part id: %d\n", partID);
			return false;
		}

		g_UserDatabase.UpdateInventoryItem(user->GetID(), weapon, partID == 0 ? UITEM_FLAG_PARTSLOT1 : UITEM_FLAG_PARTSLOT2);

		vector<CUserInventoryItem> items;
		weapon.PushItem(items, weapon);

		// update client inventory
		g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);

		break;
	}
	case 2: // check part
	{
		int weaponSlot = msg->ReadUInt16();
		int partSlot = msg->ReadUInt16();

		CUserInventoryItem weapon;
		CUserInventoryItem part;

		g_UserDatabase.GetInventoryItemBySlot(user->GetID(), weapon.GameSlotToSlot(weaponSlot), weapon);
		g_UserDatabase.GetInventoryItemBySlot(user->GetID(), part.GameSlotToSlot(partSlot), part);

		if (!weapon.m_nItemID || !part.m_nItemID)
			return false;

		int weaponCategory = g_pItemTable->GetCell<int>("Category", to_string(weapon.m_nItemID));
		int partCategory = g_pItemTable->GetCell<int>("Category", to_string(part.m_nItemID));

		if (weaponCategory != 11 && (weaponCategory < 1 || weaponCategory > 6) || partCategory != 13)
			return false;

		int partID = msg->ReadUInt8();
		switch (partID)
		{
		case 0:
			weapon.m_nPartSlot1 = part.m_nItemID;
			break;
		case 1:
			weapon.m_nPartSlot2 = part.m_nItemID;
			break;
		default:
			Logger().Error("OnPartEquipRequest: wrong part id: %d\n", partID);
			return false;
		}

		g_UserDatabase.UpdateInventoryItem(user->GetID(), weapon, partID == 0 ? UITEM_FLAG_PARTSLOT1 : UITEM_FLAG_PARTSLOT2);

		vector<CUserInventoryItem> items;
		weapon.PushItem(items, weapon);

		// update client inventory
		g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);

		// remove part item
		OnItemUse(user, part);

		g_PacketManager.SendItemPartCheck(user->GetExtendedSocket(), weaponSlot, partSlot);
		break;
	}
	default:
		Logger().Warn("CItemManager::OnPartEquipRequest: unknown request: %d\n", type);
		break;
	}
	return true;
}

bool CItemManager::OnSwitchInUseRequest(IUser* user, CReceivePacket* msg)
{
	int newInUse = msg->ReadUInt8();
	int slot = msg->ReadUInt16();

	CUserInventoryItem item;
	g_UserDatabase.GetInventoryItemBySlot(user->GetID(), item.GameSlotToSlot(slot), item);

	if (!item.m_nItemID)
		return false;

	item.m_nInUse = 1;

	vector<CUserInventoryItem> itemsWithSameID;
	g_UserDatabase.GetInventoryItemsByID(user->GetID(), item.m_nItemID, itemsWithSameID);

	bool updateStatus = 1;
	for (auto& i : itemsWithSameID)
	{
		if (i.m_nStatus)
		{
			updateStatus = 0;
			break;
		}
	}

	item.m_nStatus = updateStatus;

	item.ConvertDurationToExpiryDate();

	g_UserDatabase.UpdateInventoryItem(user->GetID(), item, UITEM_FLAG_INUSE | UITEM_FLAG_STATUS | UITEM_FLAG_EXPIRYDATE);

	vector<CUserInventoryItem> items;
	item.PushItem(items, item);

	// send inventory update to user
	g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);

	return true;
}

bool CItemManager::OnLockItemRequest(IUser* user, CReceivePacket* msg)
{
	int newLockStatus = msg->ReadUInt8();
	int slot = msg->ReadUInt16();

	CUserInventoryItem item;
	g_UserDatabase.GetInventoryItemBySlot(user->GetID(), item.GameSlotToSlot(slot), item);

	if (!item.m_nItemID)
		return false;

	if (item.IsItemDefaultOrPseudo())
	{
		// can't switch lock status of default item
		return false;
	}

	item.m_nLockStatus = item.m_nLockStatus ? 0 : 1;

	g_UserDatabase.UpdateInventoryItem(user->GetID(), item, UITEM_FLAG_LOCKSTATUS);

	vector<CUserInventoryItem> items;
	item.PushItem(items, item);

	// send inventory update to user
	g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);

	return true;
}

void CItemManager::OnUserLogin(IUser* user)
{
	vector<int> rewardsNotices;
	g_UserDatabase.GetRewardNotices(user->GetID(), rewardsNotices);

	for (int rewardID : rewardsNotices)
		GiveReward(user->GetID(), user, rewardID);

	// clear rewards notices
	if (rewardsNotices.size())
	{
		rewardsNotices.clear();
		g_UserDatabase.UpdateRewardNotices(user->GetID(), 0);
	}

	vector<int> expiryNotices;
	g_UserDatabase.GetExpiryNotices(user->GetID(), expiryNotices);
	if (expiryNotices.size())
	{
		g_PacketManager.SendUMsgExpiryNotice(user->GetExtendedSocket(), expiryNotices);

		expiryNotices.clear();
		g_UserDatabase.UpdateExpiryNotices(user->GetID(), 0);
	}
}

void CItemManager::OnRewardSelect(CReceivePacket* msg, IUser* user)
{
	if (!user)
	{
		return;
	}

	int rewardSelectID = msg->ReadUInt32();
	int rewardID = msg->ReadUInt32();

	GiveReward(user->GetID(), user, rewardID, rewardSelectID);
}

void CItemManager::OnCostumeEquip(IUser* user, int gameSlot)
{
	CUserInventoryItem item;
	g_UserDatabase.GetInventoryItemBySlot(user->GetID(), item.GameSlotToSlot(gameSlot), item);

	if (!item.m_nItemID)
		return;

	vector<CUserInventoryItem> items; // temp vector

	string className = g_pItemTable->GetCell<string>("ClassName", to_string(item.m_nItemID));
	int zombieSkinID = className == "ZombieSkinCostume" ? g_pItemTable->GetCell<int>("ZombieSkin", to_string(item.m_nItemID)) : -1;

	CUserCostumeLoadout loadout;
	if (g_UserDatabase.GetCostumeLoadout(user->GetID(), loadout) <= 0)
		return;

	int flag = UITEM_FLAG_INUSE;

	// unequip costume if item already in use
	if (item.m_nInUse == 1)
	{
		if (loadout.m_nHeadCostumeID == item.m_nItemID)
		{
			loadout.m_nHeadCostumeID = 0;
		}
		else if (loadout.m_nBackCostumeID == item.m_nItemID)
		{
			loadout.m_nBackCostumeID = 0;
		}
		else if (loadout.m_nArmCostumeID == item.m_nItemID)
		{
			loadout.m_nArmCostumeID = 0;
		}
		else if (loadout.m_nPelvisCostumeID == item.m_nItemID)
		{
			loadout.m_nPelvisCostumeID = 0;
		}
		else if (loadout.m_nFaceCostumeID == item.m_nItemID)
		{
			loadout.m_nFaceCostumeID = 0;
		}
		else if (loadout.m_ZombieSkinCostumeID.count(zombieSkinID) && loadout.m_ZombieSkinCostumeID[zombieSkinID] == item.m_nItemID)
		{
			loadout.m_ZombieSkinCostumeID[zombieSkinID] = 0;
		}
		else if (loadout.m_nTattooID == item.m_nItemID)
		{
			loadout.m_nTattooID = 0;
		}
		else if (loadout.m_nPetCostumeID == item.m_nItemID)
		{
			loadout.m_nPetCostumeID = 0;
		}

		item.m_nInUse = 0;
	}
	else
	{
		CUserInventoryItem activeCostumeItem;
		if (className == "HeadCostume")
		{
			if (loadout.m_nHeadCostumeID)
			{
				g_UserDatabase.GetFirstActiveItemByItemID(user->GetID(), loadout.m_nHeadCostumeID, activeCostumeItem);
			}
			loadout.m_nHeadCostumeID = item.m_nItemID;
		}
		else if (className == "BackCostume")
		{
			if (loadout.m_nBackCostumeID)
			{
				g_UserDatabase.GetFirstActiveItemByItemID(user->GetID(), loadout.m_nBackCostumeID, activeCostumeItem);
			}
			loadout.m_nBackCostumeID = item.m_nItemID;
		}
		else if (className == "ArmCostume")
		{
			if (loadout.m_nArmCostumeID)
			{
				g_UserDatabase.GetFirstActiveItemByItemID(user->GetID(), loadout.m_nArmCostumeID, activeCostumeItem);
			}
			loadout.m_nArmCostumeID = item.m_nItemID;
		}
		else if (className == "PelvisCostume")
		{
			if (loadout.m_nPelvisCostumeID)
			{
				g_UserDatabase.GetFirstActiveItemByItemID(user->GetID(), loadout.m_nPelvisCostumeID, activeCostumeItem);
			}
			loadout.m_nPelvisCostumeID = item.m_nItemID;
		}
		else if (className == "FaceCostume")
		{
			if (loadout.m_nFaceCostumeID)
			{
				g_UserDatabase.GetFirstActiveItemByItemID(user->GetID(), loadout.m_nFaceCostumeID, activeCostumeItem);
			}
			loadout.m_nFaceCostumeID = item.m_nItemID;
		}
		else if (className == "ZombieSkinCostume")
		{
			if (loadout.m_ZombieSkinCostumeID.count(zombieSkinID) && loadout.m_ZombieSkinCostumeID[zombieSkinID])
			{
				g_UserDatabase.GetFirstActiveItemByItemID(user->GetID(), loadout.m_ZombieSkinCostumeID[zombieSkinID], activeCostumeItem);
			}
			loadout.m_ZombieSkinCostumeID[zombieSkinID] = item.m_nItemID;
		}
		else if (className == "Tattoo")
		{
			if (loadout.m_nTattooID)
			{
				g_UserDatabase.GetFirstActiveItemByItemID(user->GetID(), loadout.m_nTattooID, activeCostumeItem);
			}
			loadout.m_nTattooID = item.m_nItemID;
		}
		else if (className == "PetCostume")
		{
			if (loadout.m_nPetCostumeID)
			{
				g_UserDatabase.GetFirstActiveItemByItemID(user->GetID(), loadout.m_nPetCostumeID, activeCostumeItem);
			}
			loadout.m_nPetCostumeID = item.m_nItemID;
		}

		if (activeCostumeItem.m_nItemID)
		{
			activeCostumeItem.m_nInUse = 0;
			activeCostumeItem.m_nStatus = 1;

			activeCostumeItem.PushItem(items, activeCostumeItem);
		}

		item.m_nInUse = 1;

		if (!item.m_nStatus)
			item.ConvertDurationToExpiryDate();

		item.m_nStatus = 1;

		flag |= UITEM_FLAG_STATUS;
	}

	if (g_UserDatabase.UpdateCostumeLoadout(user->GetID(), loadout, zombieSkinID) <= 0)
		return;

	item.PushItem(items, item);

	if (g_UserDatabase.UpdateInventoryItems(user->GetID(), items, flag) <= 0)
		return;

	// send inventory update to user
	g_PacketManager.SendInventoryAdd(user->GetExtendedSocket(), items);
}