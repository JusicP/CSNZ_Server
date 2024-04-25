#ifdef DB_PROXY
#include "userdatabase.h"
#include "userdatabase_shared.h"

using namespace std;

CUserDatabaseProxy g_UserDatabase;

CUserDatabaseProxy::CUserDatabaseProxy() : CBaseManager("UserDatabase")
{
	m_pDatabase = NULL;
}

bool CUserDatabaseProxy::Init()
{
	// get real DB
	m_pDatabase = (IUserDatabase*)Manager().GetManager(REAL_DATABASE_NAME);
	return m_pDatabase != NULL;
}

int CUserDatabaseProxy::Login(const string& userName, const string& password, IExtendedSocket* socket, UserBan& ban, UserRestoreData* restoreData)
{
	ExecCalcStart();
	int result = m_pDatabase->Login(userName, password, socket, ban, restoreData);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::AddToRestoreList(int userID, int channelServerID, int channelID)
{
	ExecCalcStart();
	int result = m_pDatabase->AddToRestoreList(userID, channelServerID, channelID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::Register(const string& userName, const string& password, const string& ip)
{
	ExecCalcStart();
	int result = m_pDatabase->Register(userName, password, ip);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetUserSessions(vector<UserSession>& sessions)
{
	ExecCalcStart();
	int result = m_pDatabase->GetUserSessions(sessions);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::DropSession(int userID)
{
	ExecCalcStart();
	int result = m_pDatabase->DropSession(userID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::DropSessions()
{
	ExecCalcStart();
	int result = m_pDatabase->DropSessions();
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::AddInventoryItem(int userID, CUserInventoryItem& item)
{
	ExecCalcStart();
	int result = m_pDatabase->AddInventoryItem(userID, item);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::AddInventoryItems(int userID, vector<CUserInventoryItem>& items)
{
	ExecCalcStart();
	int result = m_pDatabase->AddInventoryItems(userID, items);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateInventoryItem(int userID, const CUserInventoryItem& item)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateInventoryItem(userID, item);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetInventoryItems(int userID, vector<CUserInventoryItem>& items)
{
	ExecCalcStart();
	int result = m_pDatabase->GetInventoryItems(userID, items);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetInventoryItemsByID(int userID, int itemID, vector<CUserInventoryItem>& items)
{
	ExecCalcStart();
	int result = m_pDatabase->GetInventoryItemsByID(userID, itemID, items);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetInventoryItemBySlot(int userID, int slot, CUserInventoryItem& item)
{
	ExecCalcStart();
	int result = m_pDatabase->GetInventoryItemBySlot(userID, slot, item);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetFirstActiveItemByItemID(int userID, int itemID, CUserInventoryItem& item)
{
	ExecCalcStart();
	int result = m_pDatabase->GetFirstActiveItemByItemID(userID, itemID, item);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::IsInventoryFull(int userID)
{
	ExecCalcStart();
	int result = m_pDatabase->IsInventoryFull(userID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetUserData(int userID, CUserData& data)
{
	ExecCalcStart();
	int result = m_pDatabase->GetUserData(userID, data);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateUserData(int userID, CUserData data)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateUserData(userID, data);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::CreateCharacter(int userID, const string& gameName)
{
	ExecCalcStart();
	int result = m_pDatabase->CreateCharacter(userID, gameName);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::DeleteCharacter(int userID)
{
	ExecCalcStart();
	int result = m_pDatabase->DeleteCharacter(userID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetCharacter(int userID, CUserCharacter& character)
{
	ExecCalcStart();
	int result = m_pDatabase->GetCharacter(userID, character);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateCharacter(int userID, CUserCharacter& character)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateCharacter(userID, character);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetCharacterExtended(int userID, CUserCharacterExtended& character)
{
	ExecCalcStart();
	int result = m_pDatabase->GetCharacterExtended(userID, character);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateCharacterExtended(int userID, CUserCharacterExtended& character)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateCharacterExtended(userID, character);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetUserBan(int userID, UserBan& ban)
{
	ExecCalcStart();
	int result = m_pDatabase->GetUserBan(userID, ban);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateUserBan(int userID, UserBan ban)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateUserBan(userID, ban);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetLoadouts(int userID, CUserLoadout& loadout)
{
	ExecCalcStart();
	int result = m_pDatabase->GetLoadouts(userID, loadout);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateLoadout(int userID, int loadoutID, int slot, int itemID)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateLoadout(userID, loadoutID, slot, itemID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetFastBuy(int userID, vector<CUserFastBuy>& fastBuy)
{
	ExecCalcStart();
	int result = m_pDatabase->GetFastBuy(userID, fastBuy);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateFastBuy(int userID, int slot, const string& name, const vector<int>& items)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateFastBuy(userID, slot, name, items);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetBuyMenu(int userID, vector<CUserBuyMenu>& buyMenu)
{
	ExecCalcStart();
	int result = m_pDatabase->GetBuyMenu(userID, buyMenu);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateBuyMenu(int userID, int subMenuID, int subMenuSlot, int itemID)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateBuyMenu(userID, subMenuID, subMenuSlot, itemID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetBookmark(int userID, vector<int>& bookmark)
{
	ExecCalcStart();
	int result = m_pDatabase->GetBookmark(userID, bookmark);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateBookmark(int userID, int bookmarkID, int itemID)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateBookmark(userID, bookmarkID, itemID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetCostumeLoadout(int userID, CUserCostumeLoadout& loadout)
{
	ExecCalcStart();
	int result = m_pDatabase->GetCostumeLoadout(userID, loadout);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateCostumeLoadout(int userID, CUserCostumeLoadout& loadout, int zbSlot)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateCostumeLoadout(userID, loadout, zbSlot);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetRewardNotices(int userID, vector<int>& notices)
{
	ExecCalcStart();
	int result = m_pDatabase->GetRewardNotices(userID, notices);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateRewardNotices(int userID, int rewardID)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateRewardNotices(userID, rewardID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetExpiryNotices(int userID, vector<int>& notices)
{
	ExecCalcStart();
	int result = m_pDatabase->GetExpiryNotices(userID, notices);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateExpiryNotices(int userID, int itemID)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateExpiryNotices(userID, itemID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetDailyRewards(int userID, UserDailyRewards& dailyRewards)
{
	ExecCalcStart();
	int result = m_pDatabase->GetDailyRewards(userID, dailyRewards);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateDailyRewards(int userID, UserDailyRewards& dailyRewards)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateDailyRewards(userID, dailyRewards);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetQuestsProgress(int userID, vector<UserQuestProgress>& questsProgress)
{
	ExecCalcStart();
	int result = m_pDatabase->GetQuestsProgress(userID, questsProgress);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetQuestProgress(int userID, int questID, UserQuestProgress& questProgress)
{
	ExecCalcStart();
	int result = m_pDatabase->GetQuestProgress(userID, questID, questProgress);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateQuestProgress(int userID, UserQuestProgress& questProgress)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateQuestProgress(userID, questProgress);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetQuestTaskProgress(int userID, int questID, int taskID, UserQuestTaskProgress& taskProgress)
{
	ExecCalcStart();
	int result = m_pDatabase->GetQuestTaskProgress(userID, questID, taskID, taskProgress);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateQuestTaskProgress(int userID, int questID, UserQuestTaskProgress& taskProgress)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateQuestTaskProgress(userID, questID, taskProgress);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

bool CUserDatabaseProxy::IsQuestTaskFinished(int userID, int questID, int taskID)
{
	ExecCalcStart();
	int result = m_pDatabase->IsQuestTaskFinished(userID, questID, taskID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetQuestStat(int userID, int flag, UserQuestStat& stat)
{
	ExecCalcStart();
	int result = m_pDatabase->GetQuestStat(userID, flag, stat);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateQuestStat(int userID, int flag, UserQuestStat& stat)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateQuestStat(userID, flag, stat);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetBingoProgress(int userID, UserBingo& bingo)
{
	ExecCalcStart();
	int result = m_pDatabase->GetBingoProgress(userID, bingo);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateBingoProgress(int userID, UserBingo& bingo)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateBingoProgress(userID, bingo);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetBingoSlot(int userID, vector<UserBingoSlot>& slots)
{
	ExecCalcStart();
	int result = m_pDatabase->GetBingoSlot(userID, slots);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateBingoSlot(int userID, vector<UserBingoSlot>& slots, bool remove)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateBingoSlot(userID, slots, remove);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetBingoPrizeSlot(int userID, vector<UserBingoPrizeSlot>& prizes)
{
	ExecCalcStart();
	int result = m_pDatabase->GetBingoPrizeSlot(userID, prizes);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateBingoPrizeSlot(int userID, vector<UserBingoPrizeSlot>& prizes, bool remove)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateBingoPrizeSlot(userID, prizes, remove);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetUserRank(int userID, CUserCharacter& character)
{
	ExecCalcStart();
	int result = m_pDatabase->GetUserRank(userID, character);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateUserRank(int userID, CUserCharacter& character)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateUserRank(userID, character);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetBanList(int userID, vector<string>& banList)
{
	ExecCalcStart();
	int result = m_pDatabase->GetBanList(userID, banList);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateBanList(int userID, string gameName, bool remove)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateBanList(userID, gameName, remove);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

bool CUserDatabaseProxy::IsInBanList(int userID, int destUserID)
{
	ExecCalcStart();
	int result = m_pDatabase->IsInBanList(userID, destUserID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

bool CUserDatabaseProxy::IsSurveyAnswered(int userID, int surveyID)
{
	ExecCalcStart();
	bool result = m_pDatabase->IsSurveyAnswered(userID, surveyID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::SurveyAnswer(int userID, UserSurveyAnswer& answer)
{
	ExecCalcStart();
	int result = m_pDatabase->SurveyAnswer(userID, answer);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetWeaponReleaseRows(int userID, vector<UserWeaponReleaseRow>& rows)
{
	ExecCalcStart();
	int result = m_pDatabase->GetWeaponReleaseRows(userID, rows);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetWeaponReleaseRow(int userID, UserWeaponReleaseRow& row)
{
	ExecCalcStart();
	int result = m_pDatabase->GetWeaponReleaseRow(userID, row);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateWeaponReleaseRow(int userID, UserWeaponReleaseRow& row)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateWeaponReleaseRow(userID, row);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetWeaponReleaseCharacters(int userID, vector<UserWeaponReleaseCharacter>& characters, int& totalCount)
{
	ExecCalcStart();
	int result = m_pDatabase->GetWeaponReleaseCharacters(userID, characters, totalCount);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetWeaponReleaseCharacter(int userID, UserWeaponReleaseCharacter& character)
{
	ExecCalcStart();
	int result = m_pDatabase->GetWeaponReleaseCharacter(userID, character);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateWeaponReleaseCharacter(int userID, UserWeaponReleaseCharacter& character)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateWeaponReleaseCharacter(userID, character);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::SetWeaponReleaseCharacter(int userID, int weaponSlot, int slot, int character, bool opened)
{
	ExecCalcStart();
	int result = m_pDatabase->SetWeaponReleaseCharacter(userID, weaponSlot, slot, character, opened);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetAddons(int userID, vector<int>& addons)
{
	ExecCalcStart();
	int result = m_pDatabase->GetAddons(userID, addons);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::SetAddons(int userID, vector<int>& addons)
{
	ExecCalcStart();
	int result = m_pDatabase->SetAddons(userID, addons);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetUsersAssociatedWithIP(const string& ip, vector<CUserData>& userData)
{
	ExecCalcStart();
	int result = m_pDatabase->GetUsersAssociatedWithIP(ip, userData);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetUsersAssociatedWithHWID(const vector<unsigned char>& hwid, vector<CUserData>& userData)
{
	ExecCalcStart();
	int result = m_pDatabase->GetUsersAssociatedWithHWID(hwid, userData);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::CreateClan(ClanCreateConfig& clanCfg)
{
	ExecCalcStart();
	int result = m_pDatabase->CreateClan(clanCfg);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::JoinClan(int userID, int clanID, string& clanName)
{
	ExecCalcStart();
	int result = m_pDatabase->JoinClan(userID, clanID, clanName);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::CancelJoin(int userID, int clanID)
{
	ExecCalcStart();
	int result = m_pDatabase->CancelJoin(userID, clanID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::LeaveClan(int userID)
{
	ExecCalcStart();
	int result = m_pDatabase->LeaveClan(userID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::DissolveClan(int userID)
{
	ExecCalcStart();
	int result = m_pDatabase->DissolveClan(userID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetClanList(vector<ClanList_s>& clans, string clanName, int flag, int gameModeID, int playTime, int pageID, int& pageMax)
{
	ExecCalcStart();
	int result = m_pDatabase->GetClanList(clans, clanName, flag, gameModeID, playTime, pageID, pageMax);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetClanInfo(int clanID, Clan_s& clan)
{
	ExecCalcStart();
	int result = m_pDatabase->GetClanInfo(clanID, clan);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::AddClanStorageItem(int userID, int pageID, CUserInventoryItem& item)
{
	ExecCalcStart();
	int result = m_pDatabase->AddClanStorageItem(userID, pageID, item);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::DeleteClanStorageItem(int userID, int pageID, int slot)
{
	ExecCalcStart();
	int result = m_pDatabase->DeleteClanStorageItem(userID, pageID, slot);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetClanStorageItem(int userID, int pageID, int slot, CUserInventoryItem& item)
{
	ExecCalcStart();
	int result = m_pDatabase->GetClanStorageItem(userID, pageID, slot, item);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetClanStorageLastItems(int userID, vector<RewardItem>& items)
{
	ExecCalcStart();
	int result = m_pDatabase->GetClanStorageLastItems(userID, items);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetClanStoragePage(int userID, ClanStoragePage& clanStoragePage)
{
	ExecCalcStart();
	int result = m_pDatabase->GetClanStoragePage(userID, clanStoragePage);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetClanStorageHistory(int userID, ClanStorageHistory& clanStorageHistory)
{
	ExecCalcStart();
	int result = m_pDatabase->GetClanStorageHistory(userID, clanStorageHistory);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetClanStorageAccessGrade(int userID, vector<int>& accessGrade)
{
	ExecCalcStart();
	int result = m_pDatabase->GetClanStorageAccessGrade(userID, accessGrade);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateClanStorageAccessGrade(int userID, int pageID, int accessGrade)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateClanStorageAccessGrade(userID, pageID, accessGrade);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetClanUserList(int id, bool byUser, vector<ClanUser>& users)
{
	ExecCalcStart();
	int result = m_pDatabase->GetClanUserList(id, byUser, users);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetClanMemberList(int userID, vector<ClanUser>& users)
{
	ExecCalcStart();
	int result = m_pDatabase->GetClanMemberList(userID, users);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetClanMemberJoinUserList(int userID, vector<ClanUserJoinRequest>& users)
{
	ExecCalcStart();
	int result = m_pDatabase->GetClanMemberJoinUserList(userID, users);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetClan(int userID, int flag, Clan_s& clan)
{
	ExecCalcStart();
	int result = m_pDatabase->GetClan(userID, flag, clan);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetClanMember(int userID, ClanUser& clanUser)
{
	ExecCalcStart();
	int result = m_pDatabase->GetClanMember(userID, clanUser);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateClan(int userID, int flag, Clan_s clan)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateClan(userID, flag, clan);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateClanMemberGrade(int userID, const string& userName, int newGrade, ClanUser& targetMember)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateClanMemberGrade(userID, userName, newGrade, targetMember);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::ClanReject(int userID, const string& userName)
{
	ExecCalcStart();
	int result = m_pDatabase->ClanReject(userID, userName);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::ClanRejectAll(int userID)
{
	ExecCalcStart();
	int result = m_pDatabase->ClanRejectAll(userID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::ClanApprove(int userID, const string& userName)
{
	ExecCalcStart();
	int result = m_pDatabase->ClanApprove(userID, userName);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::IsClanWithMarkExists(int markID)
{
	ExecCalcStart();
	int result = m_pDatabase->IsClanWithMarkExists(markID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::ClanInvite(int userID, const string& gameName, IUser*& destUser, int& clanID)
{
	ExecCalcStart();
	int result = m_pDatabase->ClanInvite(userID, gameName, destUser, clanID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::ClanKick(int userID, const string& userName)
{
	ExecCalcStart();
	int result = m_pDatabase->ClanKick(userID, userName);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::ClanMasterDelegate(int userID, const string& userName)
{
	ExecCalcStart();
	int result = m_pDatabase->ClanMasterDelegate(userID, userName);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::IsClanExists(const string& clanName)
{
	ExecCalcStart();
	int result = m_pDatabase->IsClanExists(clanName);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

// quest event related
int CUserDatabaseProxy::GetQuestEventProgress(int userID, int questID, UserQuestProgress& questProgress)
{
	ExecCalcStart();
	int result = m_pDatabase->GetQuestEventProgress(userID, questID, questProgress);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateQuestEventProgress(int userID, const UserQuestProgress& questProgress)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateQuestEventProgress(userID, questProgress);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::GetQuestEventTaskProgress(int userID, int questID, int taskID, UserQuestTaskProgress& taskProgress)
{
	ExecCalcStart();
	int result = m_pDatabase->GetQuestEventTaskProgress(userID, questID, taskID, taskProgress);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateQuestEventTaskProgress(int userID, int questID, const UserQuestTaskProgress& taskProgress)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateQuestEventTaskProgress(userID, questID, taskProgress);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

bool CUserDatabaseProxy::IsQuestEventTaskFinished(int userID, int questID, int taskID)
{
	ExecCalcStart();
	bool result = m_pDatabase->IsQuestEventTaskFinished(userID, questID, taskID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::IsUserExists(int userID)
{
	ExecCalcStart();
	int result = m_pDatabase->IsUserExists(userID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::IsUserExists(const string& userName, bool searchByUserName)
{
	ExecCalcStart();
	int result = m_pDatabase->IsUserExists(userName, searchByUserName);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

// suspect system
int CUserDatabaseProxy::SuspectAddAction(const vector<unsigned char>& hwid, int actionID)
{
	ExecCalcStart();
	int result = m_pDatabase->SuspectAddAction(hwid, actionID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::IsUserSuspect(int userID)
{
	ExecCalcStart();
	int result = m_pDatabase->IsUserSuspect(userID);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

void CUserDatabaseProxy::OnMinuteTick(time_t curTime)
{
}

void CUserDatabaseProxy::OnDayTick()
{
}

void CUserDatabaseProxy::OnWeekTick()
{
}

map<int, UserBan> CUserDatabaseProxy::GetUserBanList()
{
	ExecCalcStart();
	auto result = m_pDatabase->GetUserBanList();
	ExecCalcEnd(__FUNCTION__);
	return result;
}

vector<int> CUserDatabaseProxy::GetUsers(int lastLoginTime)
{
	ExecCalcStart();
	auto result = m_pDatabase->GetUsers(lastLoginTime);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateIPBanList(const string& ip, bool remove)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateIPBanList(ip, remove);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

vector<string> CUserDatabaseProxy::GetIPBanList()
{
	ExecCalcStart();
	auto result = m_pDatabase->GetIPBanList();
	ExecCalcEnd(__FUNCTION__);
	return result;
}

bool CUserDatabaseProxy::IsIPBanned(const string& ip)
{
	ExecCalcStart();
	bool result = m_pDatabase->IsIPBanned(ip);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

int CUserDatabaseProxy::UpdateHWIDBanList(const vector<unsigned char>& hwid, bool remove)
{
	ExecCalcStart();
	int result = m_pDatabase->UpdateHWIDBanList(hwid, remove);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

vector<vector<unsigned char>> CUserDatabaseProxy::GetHWIDBanList()
{
	ExecCalcStart();
	auto result = m_pDatabase->GetHWIDBanList();
	ExecCalcEnd(__FUNCTION__);
	return result;
}

bool CUserDatabaseProxy::IsHWIDBanned(vector<unsigned char>& hwid)
{
	ExecCalcStart();
	bool result = m_pDatabase->IsHWIDBanned(hwid);
	ExecCalcEnd(__FUNCTION__);
	return result;
}

void CUserDatabaseProxy::ResetQuestEvent(int eventID)
{
	ExecCalcStart();
	m_pDatabase->ResetQuestEvent(eventID);
	ExecCalcEnd(__FUNCTION__);
}

void CUserDatabaseProxy::CreateTransaction()
{
	ExecCalcStart();
	m_pDatabase->CreateTransaction();
	ExecCalcEnd(__FUNCTION__);
}

bool CUserDatabaseProxy::CommitTransaction()
{
	ExecCalcStart();
	bool result = m_pDatabase->CommitTransaction();
	ExecCalcEnd(__FUNCTION__);
	return result;
}

void CUserDatabaseProxy::ExecCalcStart()
{
	m_StartTime = chrono::high_resolution_clock::now();
}

void CUserDatabaseProxy::ExecCalcEnd(const string& funcName)
{
	auto end = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(end - m_StartTime).count();

	if (duration > 0)
		Logger().Warn(OBFUSCATE("%s: %d ms\n"), funcName.c_str(), duration);
}
#endif