#include "userinventoryitem.h"
#include "serverconfig.h"
#include "common/utils.h"

CUserInventoryItem::CUserInventoryItem()
{
	m_nSlot = 0;
	Reset();
}

CUserInventoryItem::CUserInventoryItem(int slot, int itemID, int count, int status, int inUse, int obtainDate, int expiryDate, int isClanItem, int enhanceLvl, int enhanceExp, int enhanceValue, int paintID, std::vector<int> paintIDList, int partSlot1, int partSlot2, int lockStatus)
{
	Init(slot, itemID, count, status, inUse, obtainDate, expiryDate, isClanItem, enhanceLvl, enhanceExp, enhanceValue, paintID, paintIDList, partSlot1, partSlot2, lockStatus);
}

// for sqlite 
CUserInventoryItem::CUserInventoryItem(int userID, int slot, int itemID, int count, int status, int inUse, int obtainDate, int expiryDate, int isClanItem, int enhanceLvl, int enhanceExp, int enhanceValue, int paintID, std::string paintIDList, int partSlot1, int partSlot2, int lockStatus)
{
	Init(slot, itemID, count, status, inUse, obtainDate, expiryDate, isClanItem, enhanceLvl, enhanceExp, enhanceValue, paintID, deserialize_array_int(paintIDList), partSlot1, partSlot2, lockStatus);
}

void CUserInventoryItem::Init(int slot, int itemID, int count, int status, int inUse, int obtainDate, int expiryDate, int isClanItem, int enhanceLvl, int enhanceExp, int enhanceValue, int paintID, std::vector<int> paintIDList, int partSlot1, int partSlot2, int lockStatus)
{
	m_nSlot = slot;
	m_nItemID = itemID;
	m_nCount = count;
	m_nStatus = status;
	m_nInUse = inUse;
	m_nObtainDate = obtainDate;
	m_nExpiryDate = expiryDate;
	m_nIsClanItem = isClanItem;
	m_nPaintID = paintID;
	m_nPaintIDList = paintIDList;
	m_nEnhancementLevel = enhanceLvl;
	m_nEnhancementExp = enhanceExp;
	m_nEnhanceValue = enhanceValue;
	m_nPartSlot1 = partSlot1;
	m_nPartSlot2 = partSlot2;
	m_nLockStatus = lockStatus;
}

void CUserInventoryItem::Reset()
{
	m_nItemID = 0;
	m_nCount = 0;
	m_nInUse = 0;
	m_nStatus = 0;
	m_nObtainDate = 0;
	m_nExpiryDate = 0;
	m_nPaintID = 0;
	m_nPaintIDList.clear();
	m_nIsClanItem = 0;
	m_nEnhancementLevel = 0;
	m_nEnhancementExp = 0;
	m_nEnhanceValue = 0;
	m_nPartSlot1 = 0;
	m_nPartSlot2 = 0;
	m_nLockStatus = 0;
}

void CUserInventoryItem::PushItem(std::vector<CUserInventoryItem>& vec, int itemID, int count, int status, int inUse, int obtainDate, int expiryDate, int isClanItem, int enhanceLvl, int enhanceExp, int enhanceValue, int paintID, std::vector<int> paintIDList, int partSlot1, int partSlot2, int lockStatus)
{
	vec.push_back(CUserInventoryItem(0, itemID, count, status, inUse, obtainDate, expiryDate, isClanItem, enhanceLvl, enhanceExp, enhanceValue, paintID, paintIDList, partSlot1, partSlot2, lockStatus));
}

void CUserInventoryItem::PushItem(std::vector<CUserInventoryItem>& vec, CUserInventoryItem& item)
{
	vec.push_back(item);
}

void CUserInventoryItem::ConvertDurationToExpiryDate()
{
	m_nExpiryDate = m_nExpiryDate > 0 ? g_pServerInstance->GetCurrentTime() + m_nExpiryDate * CSO_24_HOURS_IN_MINUTES : 0;
}

bool CUserInventoryItem::IsItemDefault()
{
	std::vector<int>& defaultItems = g_pServerConfig->defUser.defaultItems;
	return find(defaultItems.begin(), defaultItems.end(), m_nItemID) != defaultItems.end();
}

bool CUserInventoryItem::IsItemDefault(int itemID)
{
	std::vector<int>& defaultItems = g_pServerConfig->defUser.defaultItems;
	return find(defaultItems.begin(), defaultItems.end(), itemID) != defaultItems.end();
}

bool CUserInventoryItem::IsItemPseudoDefault()
{
	std::vector<int>& defaultItems = g_pServerConfig->defUser.pseudoDefaultItems;
	return find(defaultItems.begin(), defaultItems.end(), m_nItemID) != defaultItems.end();
}

bool CUserInventoryItem::IsItemPseudoDefault(int itemID)
{
	std::vector<int>& defaultItems = g_pServerConfig->defUser.pseudoDefaultItems;
	return find(defaultItems.begin(), defaultItems.end(), itemID) != defaultItems.end();
}

bool CUserInventoryItem::IsItemDefaultOrPseudo(int itemID)
{
	if (!itemID)
		itemID = m_nItemID;

	return IsItemDefault(itemID) || IsItemPseudoDefault(itemID);
}

int CUserInventoryItem::GetGameSlot() const
{
	return g_pServerConfig->defUser.defaultItems.size() + m_nSlot;
}

int CUserInventoryItem::GetSlot()
{
	return m_nSlot - g_pServerConfig->defUser.defaultItems.size();
}

int CUserInventoryItem::GameSlotToSlot(int gameSlot)
{
	return gameSlot - g_pServerConfig->defUser.defaultItems.size();
}

int CUserInventoryItem::GetPartCount() const
{
	int count = 0;
	if (m_nPartSlot1 != 0)
		count++;
	if (m_nPartSlot2 != 0)
		count++;

	return count;
}