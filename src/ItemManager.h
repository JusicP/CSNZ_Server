#pragma once

#include "UserManager.h"
#include "Definitions.h"
#include "CSVTable.h"

#define ITEM_ADD_SUCCESS 1
#define ITEM_ADD_INVENTORY_FULL -1
#define ITEM_ADD_UNKNOWN_ITEMID -2
#define ITEM_ADD_DB_ERROR -3

#define ITEM_USE_SUCCESS 1
#define ITEM_USE_BAD_SLOT -1
#define ITEM_USE_WRONG_ITEM -2

class CItemManager
{
public:
	CItemManager();
	~CItemManager();

	void Init();
	void Shutdown();
	void LoadRewards();
	bool KVToJson();
	void ProcessEvents(time_t curTime);
	void OnDayTick();
	bool OnItemPacket(CReceivePacket* msg, CExtendedSocket* socket);
	int AddItem(int userID, CUser* user, int itemId, int count, int duration);
	int AddItems(int userID, CUser* user, std::vector<RewardItem>& item);
	bool RemoveItem(int userID, CUser* user, CUserInventoryItem& item);
	int UseItem(CUser* user, int slot, int additionalArg = 0, int additionalArg2 = 0);
	bool CanUseItem(const CUserInventoryItem& item);
	bool OpenDecoder(CUser* user, int count, int slot);
	//void PartCheck(CUser* user, CPacketIn_ItemPartCheck packet);
	bool ExtendItem(int userID, CUser* user, CUserInventoryItem& item, int newExpiryDate, bool duration = false);
	bool OnDisassembleRequest(CUser* user, CReceivePacket* msg);
	RewardNotice GiveReward(int userID, CUser* user, int rewardID, int rewardSelectID = 0, bool ignoreClient = false, int randomRepeatCount = 0);

	void UpdateDailyRewards(int userID);
	void OnUserLogin(CUser* user);
	void OnNicknameChangeUse(CUser* user, std::string newNickname);
	void OnRewardSelect(CReceivePacket* msg, CUser* user);
	void OnCostumeEquip(CUser* user, int slot);

private:
	Reward* GetRewardByID(int rewardID);
	bool OnItemUse(CUser* user, CUserInventoryItem& item, int count = 1);
	bool OnDailyRewardsRequest(CUser* user, int requestId);
	bool OnEnhancementRequest(CUser* user, CReceivePacket* msg);
	bool OnWeaponPaintRequest(CUser* user, CReceivePacket* msg);
	bool OnWeaponPaintSwitchRequest(CUser* user, CReceivePacket* msg);
	bool OnPartEquipRequest(CUser* user, CReceivePacket* msg);
	bool OnSwitchInUseRequest(CUser* user, CReceivePacket* msg);
	bool OnLockItemRequest(CUser* user, CReceivePacket* msg);
	void OnDailyRewardsSpinRequest(CUser* user);

	// enhance funcs
	void InsertExp(CUser* user, CUserInventoryItem& targetItem, std::vector<CUserInventoryItem>& items);

	std::vector<Reward> m_Rewards;

	CCSVTable* m_pReinforceMaxLvTable;
	CCSVTable* m_pReinforceMaxExpTable;
};