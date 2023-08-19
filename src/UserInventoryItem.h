#pragma once

#include <vector>

class CUserInventoryItem
{
public:
	// !! keep argument order!
	CUserInventoryItem();
	//CUserInventoryItem(int slot, int count, int inUse, int obtainDate, int expiryDate, int status, int isClanItem, int enhanceLvl, int enhanceExp, int enhanceValue, int paintID, std::vector<int> parts);
	CUserInventoryItem(int slot, int itemID, int count, int status, int inUse, int obtainDate, int expiryDate, int isClanItem, int enhanceLvl, int enhanceExp, int enhanceValue, int paintID, std::vector<int> paintIDList, int partSlot1, int partSlot2, int lockStatus);
	CUserInventoryItem(int userID, int slot, int itemID, int count, int status, int inUse, int obtainDate, int expiryDate, int isClanItem, int enhanceLvl, int enhanceExp, int enhanceValue, int paintID, std::string paintIDList, int partSlot1, int partSlot2, int lockStatus);

	void Init(int slot, int itemID, int count, int status, int inUse, int obtainDate, int expiryDate, int isClanItem, int enhanceLvl, int enhanceExp, int enhanceValue, int paintID, std::vector<int> paintIDList, int partSlot1, int partSlot2, int lockStatus);
	void Reset();
	void PushItem(std::vector<CUserInventoryItem>& vec, int itemID, int count, int status, int inUse, int obtainDate, int expiryDate, int isClanItem, int enhanceLvl, int enhanceExp, int enhanceValue, int paintID, std::vector<int> paintIDList, int partSlot1, int partSlot2, int lockStatus);
	void PushItem(std::vector<CUserInventoryItem>& vec, CUserInventoryItem& item);
	void ConvertDurationToExpiryDate();
	bool IsItemDefault();
	bool IsItemDefault(int itemID);
	bool IsItemPseudoDefault();
	bool IsItemPseudoDefault(int itemID);
	bool IsItemDefaultOrPseudo(int itemID = 0);
	int GetGameSlot();
	int GetSlot();
	int GameSlotToSlot(int gameSlot);
	int GetPartCount();

	int m_nSlot; // real slot from DB
	int m_nItemID;
	int m_nCount;
	int m_nInUse;
	int m_nObtainDate;
	int m_nExpiryDate;
	int m_nPaintID;
	std::vector<int> m_nPaintIDList;
	int m_nStatus;
	int m_nEnhancementLevel;
	int m_nEnhancementExp;
	int m_nEnhanceValue;
	int m_nIsClanItem;
	int m_nPartSlot1;
	int m_nPartSlot2;
	int m_nLockStatus;
};