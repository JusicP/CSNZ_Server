#include "minigamemanager.h"
#include "packetmanager.h"
#include "userdatabase.h"
#include "itemmanager.h"
#include "serverconfig.h"

using namespace std;

CMiniGameManager g_MiniGameManager;

CMiniGameManager::CMiniGameManager() : CBaseManager("MiniGameManager")
{
}

CMiniGameManager::~CMiniGameManager()
{
}

void CMiniGameManager::OnPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = g_UserManager.GetUserBySocket(socket);
	if (!user)
		return;

	int requestID = msg->ReadInt8();
	switch (requestID)
	{
	case MiniGamePacketType::RequestMiniGameBingo:
		OnBingoRequest(msg, user);
		break;
	case MiniGamePacketType::RequestMiniGameWeaponRelease:
		OnWeaponReleaseRequest(msg, user);
		break;

	default:
		Logger().Info(OBFUSCATE("[User '%s'] Packet_Minigame request %d is not implemented\n"), user->GetLogName(), requestID);
	}
}

void CMiniGameManager::WeaponReleaseAddCharacter(IUser* user, char charID, int count)
{
	UserWeaponReleaseCharacter character;
	character.character = charID;
	character.count = count;

	g_UserDatabase.UpdateWeaponReleaseCharacter(user->GetID(), character);
}

void CMiniGameManager::OnBingoRequest(CReceivePacket* msg, IUser* user)
{
	int requestID = msg->ReadInt8();
	switch (requestID)
	{
	case MiniGameBingoPacketType::RequestMiniGameBingoUpdate:
		OnBingoUpdateRequest(user);
		break;
	case MiniGameBingoPacketType::RequestMiniGameBingoOpenRandomNumber:
		//OnBingoOpenRandomNumberRequest(user);
		break;
	case MiniGameBingoPacketType::RequestMiniGameBingoReset:
		OnBingoResetRequest(user);
		break;
	case MiniGameBingoPacketType::RequestMiniGameBingoShuffle:
		OnBingoShuffleRequest(user);
		break;
	default:
		Logger().Info(OBFUSCATE("[User '%s'] Packet_Minigame bingo request %d is not implemented\n"), user->GetLogName(), requestID);
	}
}

bool CMiniGameManager::BingoInitDesk(IUser* user, UserBingo& bingo)
{
	if (bingo.status != UserBingoStatus::BINGO_UNINITIALIZED)
		return false;

	vector<int> shuffledNumbers;
	for (int i = 0; i < 100; i++)
		shuffledNumbers.push_back(i);

	random_device rd;
	mt19937 g(rd());

	shuffle(shuffledNumbers.begin(), shuffledNumbers.end(), g);

	vector<UserBingoSlot> slots;
	for (int i = 0; i < 25; i++)
	{
		UserBingoSlot slot;
		slot.number = shuffledNumbers[i];
		slot.opened = false;

		slots.push_back(slot);
	}
	g_UserDatabase.UpdateBingoSlot(user->GetID(), slots, true);

	vector<RewardItem> items = g_pServerConfig->bingo.prizeItems;
	if (items.size() < 12)
	{
		Logger().Info(OBFUSCATE("CMiniGameManager::OnBingoUpdateRequest: could not initialize bingo prize desk, items.size() < 12!!!\n"));
		g_PacketManager.SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("Internal error occurred while initializing bingo desk, tell administrator"));
		return false;
	}

	shuffle(items.begin(), items.end(), g);

	vector<UserBingoPrizeSlot> prizes;
	for (int i = 0; i < 12; i++)
	{
		UserBingoPrizeSlot prizeSlot;
		prizeSlot.index = i;
		prizeSlot.item = items[i];
		prizeSlot.opened = false;

		prizes.push_back(prizeSlot);
	}

	// init prizes(rewrite?)
	for (int i = 0, a = 0; i < 5; i++, a = i * 5)
	{
		for (int j = 0; j < 5; j++)
			prizes[i].bingoIndexes.push_back(j + a);
	}

	// вертикаль
	prizes[5].bingoIndexes.push_back(0);
	prizes[5].bingoIndexes.push_back(5);
	prizes[5].bingoIndexes.push_back(10);
	prizes[5].bingoIndexes.push_back(15);
	prizes[5].bingoIndexes.push_back(20);

	prizes[6].bingoIndexes.push_back(1);
	prizes[6].bingoIndexes.push_back(6);
	prizes[6].bingoIndexes.push_back(11);
	prizes[6].bingoIndexes.push_back(16);
	prizes[6].bingoIndexes.push_back(21);

	prizes[7].bingoIndexes.push_back(2);
	prizes[7].bingoIndexes.push_back(7);
	prizes[7].bingoIndexes.push_back(12);
	prizes[7].bingoIndexes.push_back(17);
	prizes[7].bingoIndexes.push_back(22);

	prizes[8].bingoIndexes.push_back(3);
	prizes[8].bingoIndexes.push_back(8);
	prizes[8].bingoIndexes.push_back(13);
	prizes[8].bingoIndexes.push_back(18);
	prizes[8].bingoIndexes.push_back(23);

	prizes[9].bingoIndexes.push_back(4);
	prizes[9].bingoIndexes.push_back(9);
	prizes[9].bingoIndexes.push_back(14);
	prizes[9].bingoIndexes.push_back(19);
	prizes[9].bingoIndexes.push_back(24);

	prizes[10].bingoIndexes.push_back(0);
	prizes[10].bingoIndexes.push_back(6);
	prizes[10].bingoIndexes.push_back(12);
	prizes[10].bingoIndexes.push_back(18);
	prizes[10].bingoIndexes.push_back(24);

	prizes[11].bingoIndexes.push_back(4);
	prizes[11].bingoIndexes.push_back(8);
	prizes[11].bingoIndexes.push_back(12);
	prizes[11].bingoIndexes.push_back(16);
	prizes[11].bingoIndexes.push_back(20);

	g_UserDatabase.UpdateBingoPrizeSlot(user->GetID(), prizes, true);

	bingo.status = UserBingoStatus::BINGO_IN_PROCCESS;

	g_UserDatabase.UpdateBingoProgress(user->GetID(), bingo);

	g_PacketManager.SendMiniGameBingoUpdate(user->GetExtendedSocket(), bingo, slots, prizes);

	return true;
}

void CMiniGameManager::OnBingoUpdateRequest(IUser* user)
{
	UserBingo bingo = {};
	g_UserDatabase.GetBingoProgress(user->GetID(), bingo);

	if (!BingoInitDesk(user, bingo) && !BingoOpenRandomNumber(user, bingo))
	{
		vector<UserBingoPrizeSlot> prizes;
		g_UserDatabase.GetBingoPrizeSlot(user->GetID(), prizes);

		vector<UserBingoSlot> slots;
		g_UserDatabase.GetBingoSlot(user->GetID(), slots);

		g_PacketManager.SendMiniGameBingoUpdate(user->GetExtendedSocket(), bingo, slots, prizes);
	}
}

UserBingoSlot* GetSlotByNum(vector<UserBingoSlot>& bingo, int num)
{
	for (int i = 0; i < (int)bingo.size(); i++)
	{
		if (bingo[i].number == num)
			return &bingo[i];
	}

	return NULL;
}

bool CMiniGameManager::BingoOpenRandomNumber(IUser* user, UserBingo& bingo)
{
	/*if (!bingo.canPlay)
		return false;

	std::vector<UserBingoPrizeSlot> prizes;
	if (g_UserDatabase.GetBingoPrizeSlot(user->GetID(), prizes) <= 0)
		return false;

	std::vector<UserBingoSlot> slots;
	if (g_UserDatabase.GetBingoSlot(user->GetID(), slots) <= 0)
		return false;

	g_PacketManager.SendMiniGameBingoUpdate(user->GetExtendedSocket(), bingo, slots, prizes);

	UserBingoOpenNumberResult result;
	result.opened = false;
	int openedSlots = 0;

	Randomer random(99);
	result.number = random();
	for (auto& slot : slots)
	{
		if (slot.number == result.number && !slot.opened)
		{
			slot.opened = true;
			result.opened = true;
		}

		if (slot.opened)
			openedSlots++;
	}

	for (auto& prize : prizes)
	{
		// todo rewrite
		int numsOpened = 0;
		for (auto& index : prize.bingoIndexes)
		{
			if (index > (int)slots.size())
			{
				Logger().Info(OBFUSCATE("CMiniGameManager::OnBingoOpenRandomNumberRequest: bingo internal error, index > uData->bingo.slots.size(), %d > %d\n"), index, bingo.slots.size());
				break;
			}

			UserBingoSlot bingoSlot = slots[index];
			if (bingoSlot.opened && !prize.opened)
				numsOpened++;
		}
		if (numsOpened == prize.bingoIndexes.size() && !prize.opened)
		{
			prize.opened = true;

			result.prizes.push_back(prize);
		}
	}

	g_PacketManager.SendMiniGameBingoOpenRandomNumber(user->GetExtendedSocket(), result.number);

	if (result.prizes.size())
	{
		g_PacketManager.SendMiniGameBingoOpenPrize(user->GetExtendedSocket(), result.prizes);

		for (auto prize : result.prizes)
		{
			int result = g_ItemManager.AddItem(user->GetID(), user, prize.item.itemID, prize.item.count, prize.item.duration);
			switch (result)
			{
			case ITEM_ADD_INVENTORY_FULL:
				g_PacketManager.SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("You inventory is full"));
				break;
			case ITEM_ADD_UNKNOWN_ITEMID:
				g_PacketManager.SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("An internal error occured while adding an item. Contact the administrator"));
				break;
			}
			
			if (result <= 0)
			{
				break;
			}
		}
	}

	if (openedSlots == 25)
	{
		bingo.status = UserBingoStatus::BINGO_UNINITIALIZED;
		slots.clear();
		prizes.clear();
	}

	bingo.canPlay = false;

	g_UserDatabase.UpdateBingoProgress(user->GetID(), bingo);
	g_UserDatabase.UpdateBingoSlot(user->GetID(), slots, true);
	g_UserDatabase.UpdateBingoPrizeSlot(user->GetID(), prizes, true);*/

	return true;
}

void CMiniGameManager::OnBingoResetRequest(IUser* user)
{
	/*UserBingo bingo = {};
	g_UserDatabase.GetBingoProgress(user->GetID(), bingo);

	bingo.status = UserBingoStatus::BINGO_UNINITIALIZED;

	BingoInitDesk(user, bingo);*/
}

void CMiniGameManager::OnBingoShuffleRequest(IUser* user)
{
	/*UserBingo bingo = {};
	g_UserDatabase.GetBingoProgress(user->GetID(), bingo);

	if (bingo.status != UserBingoStatus::BINGO_IN_PROCCESS)
		return;

	std::vector<UserBingoPrizeSlot> prizes;
	g_UserDatabase.GetBingoPrizeSlot(user->GetID(), prizes);

	std::vector<UserBingoSlot> slots;
	g_UserDatabase.GetBingoSlot(user->GetID(), slots);

	int openedSlotCount = 0;
	for (auto slot : slots)
	{
		if (slot.opened)
			openedSlotCount++;
	}

	int openedPrizeSlotCount = 0;
	for (auto prizeSlot : prizes)
	{
		if (prizeSlot.opened)
			openedPrizeSlotCount++;
	}

	if (openedSlotCount > 11 || openedPrizeSlotCount > 1)
	{
		g_PacketManager.SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("BINGO_SHUFFLE_NUMBERS_FAILED_SHUFFLE_LIMIT_EXCEED"));
		return;
	}

	random_shuffle(slots.begin(), slots.end());

	g_PacketManager.SendMiniGameBingoUpdate(user->GetExtendedSocket(), bingo, slots, prizes);*/
}


void CMiniGameManager::OnWeaponReleaseRequest(CReceivePacket* msg, IUser* user)
{
	int requestID = msg->ReadUInt8();
	switch (requestID)
	{
	case MiniGameWeaponReleasePacketType::RequestMiniGameWeaponReleaseUpdate:
		SendWeaponReleaseUpdate(user);
		break;
	case MiniGameWeaponReleasePacketType::RequestMiniGameWeaponReleaseSetCharacter:
		OnWeaponReleaseSetCharacterRequest(msg, user);
		break;
	case MiniGameWeaponReleasePacketType::RequestMiniGameWeaponReleaseGetJoker:
		OnWeaponReleaseGetJokerRequest(user);
		break;
	default:
		Logger().Info(OBFUSCATE("[User '%s'] Packet_Minigame weapon release request %d is not implemented\n"), user->GetLogName(), requestID);
	}
}

void CMiniGameManager::SendWeaponReleaseUpdate(IUser* user)
{
	vector<UserWeaponReleaseCharacter> characters;
	vector<UserWeaponReleaseRow> rows;
	int totalCharacterCount = 0;

	g_UserDatabase.GetWeaponReleaseCharacters(user->GetID(), characters, totalCharacterCount);
	g_UserDatabase.GetWeaponReleaseRows(user->GetID(), rows);

	g_PacketManager.SendMiniGameWeaponReleaseUpdate(user->GetExtendedSocket(), g_pServerConfig->weaponRelease, rows, characters, totalCharacterCount);
}

void CMiniGameManager::OnWeaponReleaseSetCharacterRequest(CReceivePacket* msg, IUser* user)
{
	int weaponSlot = msg->ReadUInt8();
	int slot = msg->ReadUInt8();
	int charID = msg->ReadUInt8();

	if ((int)g_pServerConfig->weaponRelease.rows.size() < weaponSlot)
		return;

	WeaponReleaseConfigRow rowCfg = g_pServerConfig->weaponRelease.rows[weaponSlot];
	UserWeaponReleaseRow row = {};
	row.id = rowCfg.item.itemID;

	g_UserDatabase.GetWeaponReleaseRow(user->GetID(), row);
	if (row.opened)
	{
		return;
	}

	UserWeaponReleaseCharacter character = {};
	character.character = charID;
	if (g_UserDatabase.GetWeaponReleaseCharacter(user->GetID(), character) <= 0 && character.count <= 0)
	{
		// try to use joker char
		character.character = '~';
		if (g_UserDatabase.GetWeaponReleaseCharacter(user->GetID(), character) <= 0 && character.count <= 0)
		{
			return;
		}
	}

	int len = rowCfg.rowName.size();
	int charPos = len - (slot + 1);
	int progress = 1 << charPos;
	if (progress & row.progress)
	{
		g_PacketManager.SendMiniGameWeaponReleaseSetCharacter(user->GetExtendedSocket(), 5, 0, 0, 0, 0);
		return;
	}

	bool foundChar = false;
	if (!foundChar)
	{
		for (auto& rowName : rowCfg.rowName)
		{
			if (rowName == charID && !(row.progress & 1 << charPos))
			{
				foundChar = true;
				break;
			}
		}
	}

	if (!foundChar)
	{
		return;
	}

	bool lastChar = true;
	row.progress |= progress;
	for (int i = 0; i < len; i++)
	{
		if (!(row.progress & (1 << i)))
		{
			lastChar = false;
			break;
		}
	}

	if (g_UserDatabase.SetWeaponReleaseCharacter(user->GetID(), row.id, charPos, character.character, lastChar) <= 0)
	{
		return;
	}

	if (lastChar)
	{
		g_ItemManager.AddItem(user->GetID(), user, rowCfg.item.itemID, rowCfg.item.count, rowCfg.item.duration);

		RewardItem rewardItem = {};
		rewardItem.itemID = rowCfg.item.itemID;
		rewardItem.count = rowCfg.item.count;
		rewardItem.duration = rowCfg.item.duration;

		RewardNotice rewardNotice = {};
		rewardNotice.items.push_back(rewardItem);

		g_PacketManager.SendUMsgRewardNotice(user->GetExtendedSocket(), rewardNotice, OBFUSCATE("Weapon Release Event"), OBFUSCATE("Congratulations! You have received the below weapon by collecting the word."));
	}

	g_PacketManager.SendMiniGameWeaponReleaseSetCharacter(user->GetExtendedSocket(), lastChar ? 2 : 0, weaponSlot, slot, character.character, character.count - 1);
}

void CMiniGameManager::OnWeaponReleaseGetJokerRequest(IUser* user)
{

}