#pragma once

#ifdef DB_SQLITE
#include "userdatabase_sqlite.h"
#elif defined DB_MYSQL
#include "userdatabase_mysql.h"
#elif defined DB_POSTGRESQL
#include "userdatabase_postgresql.h"
#else
#error No database defined
#endif

#ifndef DB_PROXY
#ifdef DB_SQLITE
extern CUserDatabaseSQLite g_UserDatabase;
#elif defined DB_MYSQL
extern CUserDatabaseMySQL g_UserDatabase;
#elif defined DB_POSTGRESQL
extern CUserDatabasePostgreSQL g_UserDatabase;
#else
#error No database defined
#endif

#else
#include <chrono>

/**
 * Proxy manager class for measuring execution time of database methods.
 * This is not quite a proxy in the traditional sense, since it is a separate manager and doesn't control the lifetime of real manager.
 */
class CUserDatabaseProxy : public CBaseManager<IUserDatabase>
{
public:
	CUserDatabaseProxy();

	virtual bool Init();

	virtual int Login(const std::string& userName, const std::string& password, IExtendedSocket* socket, UserBan& ban, UserRestoreData* restoreData);
	virtual int AddToRestoreList(int userID, int channelServerID, int channelID);
	virtual int Register(const std::string& userName, const std::string& password, const std::string& ip);
	virtual int GetUserSessions(std::vector<UserSession>& sessions);
	virtual int DropSession(int userID);
	virtual int DropSessions();

	// user related
	virtual int AddInventoryItem(int userID, CUserInventoryItem& item);
	virtual int AddInventoryItems(int userID, std::vector<CUserInventoryItem>& items); // TODO
	virtual int UpdateInventoryItem(int userID, const CUserInventoryItem& item, int flag);
	virtual int UpdateInventoryItems(int userID, std::vector<CUserInventoryItem>& items, int flag);
	virtual int GetInventoryItems(int userID, std::vector<CUserInventoryItem>& items);
	virtual int GetInventoryItemsByID(int userID, int itemID, std::vector<CUserInventoryItem>& items);
	virtual int GetInventoryItemBySlot(int userID, int slot, CUserInventoryItem& item);
	virtual int GetFirstItemByItemID(int userID, int itemID, CUserInventoryItem& item);
	virtual int GetFirstActiveItemByItemID(int userID, int itemID, CUserInventoryItem& item);
	virtual int GetFirstExtendableItemByItemID(int userID, int itemID, CUserInventoryItem& item);
	virtual int GetInventoryItemsCount(int userID);
	virtual int IsInventoryFull(int userID);
	virtual int GetUserData(int userID, CUserData& data);
	virtual int UpdateUserData(int userID, CUserData data);
	virtual int CreateCharacter(int userID, const std::string& gameName);
	virtual int DeleteCharacter(int userID);
	virtual int GetCharacter(int userID, CUserCharacter& character);
	virtual int UpdateCharacter(int userID, CUserCharacter& character);
	virtual int GetCharacterExtended(int userID, CUserCharacterExtended& character);
	virtual int UpdateCharacterExtended(int userID, CUserCharacterExtended& character);
	virtual int GetUserBan(int userID, UserBan& ban);
	virtual int UpdateUserBan(int userID, UserBan ban);
	virtual int GetLoadouts(int userID, std::vector<std::vector<CUserLoadout>>& loadouts);
	virtual int UpdateLoadout(int userID, int groupID, int loadoutID, int slot, int itemID);
	virtual int UpdateLoadoutName(int userID, int groupID, int loadoutID, std::string name);
	virtual int GetFastBuy(int userID, std::vector<CUserFastBuy>& fastBuy);
	virtual int UpdateFastBuy(int userID, int slot, const std::string& name, const std::vector<int>& items);
	virtual int GetBuyMenu(int userID, std::vector<CUserBuyMenu>& buyMenu);
	virtual int UpdateBuyMenu(int userID, int subMenuID, int subMenuSlot, int itemID);
	virtual int GetBookmark(int userID, std::vector<std::vector<int>>& bookmark);
	virtual int UpdateBookmark(int userID, int groupID, int bookmarkID, int itemID);
	virtual int GetCostumeLoadout(int userID, CUserCostumeLoadout& loadout);
	virtual int UpdateCostumeLoadout(int userID, CUserCostumeLoadout& loadout, int zbSlot);
	virtual int GetRewardNotices(int userID, std::vector<int>& notices);
	virtual int UpdateRewardNotices(int userID, int rewardID);
	virtual int GetExpiryNotices(int userID, std::vector<int>& notices);
	virtual int UpdateExpiryNotices(int userID, int itemID);
	virtual int GetDailyRewards(int userID, UserDailyRewards& dailyRewards);
	virtual int UpdateDailyRewards(int userID, UserDailyRewards& dailyRewards);
	virtual int GetQuestsProgress(int userID, std::vector<UserQuestProgress>& questsProgress);
	virtual int GetQuestProgress(int userID, int questID, UserQuestProgress& questProgress);
	virtual int UpdateQuestProgress(int userID, UserQuestProgress& questProgress);
	virtual int GetQuestTaskProgress(int userID, int questID, int taskID, UserQuestTaskProgress& taskProgress);
	virtual int UpdateQuestTaskProgress(int userID, int questID, UserQuestTaskProgress& taskProgress);
	virtual bool IsQuestTaskFinished(int userID, int questID, int taskID);
	virtual int GetQuestStat(int userID, int flag, UserQuestStat& stat);
	virtual int UpdateQuestStat(int userID, int flag, UserQuestStat& stat);
	virtual int GetBingoProgress(int userID, UserBingo& bingo);
	virtual int UpdateBingoProgress(int userID, UserBingo& bingo);
	virtual int GetBingoSlot(int userID, std::vector<UserBingoSlot>& slots);
	virtual int UpdateBingoSlot(int userID, std::vector<UserBingoSlot>& slots, bool remove = false);
	virtual int GetBingoPrizeSlot(int userID, std::vector<UserBingoPrizeSlot>& prizes);
	virtual int UpdateBingoPrizeSlot(int userID, std::vector<UserBingoPrizeSlot>& prizes, bool remove = false);
	virtual int GetUserRank(int userID, CUserCharacter& character); // to review
	virtual int UpdateUserRank(int userID, CUserCharacter& character); // to review
	virtual int GetBanList(int userID, std::vector<std::string>& banList);
	virtual int UpdateBanList(int userID, std::string gameName, bool remove = false);
	virtual bool IsInBanList(int userID, int destUserID);
	virtual bool IsSurveyAnswered(int userID, int surveyID);
	virtual int SurveyAnswer(int userID, UserSurveyAnswer& answer);

	virtual int GetWeaponReleaseRows(int userID, std::vector<UserWeaponReleaseRow>& rows);
	virtual int GetWeaponReleaseRow(int userID, UserWeaponReleaseRow& row);
	virtual int UpdateWeaponReleaseRow(int userID, UserWeaponReleaseRow& row);

	virtual int GetWeaponReleaseCharacters(int userID, std::vector<UserWeaponReleaseCharacter>& characters, int& totalCount);
	virtual int GetWeaponReleaseCharacter(int userID, UserWeaponReleaseCharacter& character);
	virtual int UpdateWeaponReleaseCharacter(int userID, UserWeaponReleaseCharacter& character);

	virtual int SetWeaponReleaseCharacter(int userID, int weaponSlot, int slot, int character, bool opened);

	virtual int GetAddons(int userID, std::vector<int>& addons);
	virtual int SetAddons(int userID, std::vector<int>& addons);

	virtual int GetUsersAssociatedWithIP(const std::string& ip, std::vector<CUserData>& userData);
	virtual int GetUsersAssociatedWithHWID(const std::vector<unsigned char>& hwid, std::vector<CUserData>& userData);

	// clan related
	virtual int CreateClan(ClanCreateConfig& clanCfg);
	virtual int JoinClan(int userID, int clanID, std::string& clanName);
	virtual int CancelJoin(int userID, int clanID);
	virtual int LeaveClan(int userID);
	virtual int DissolveClan(int userID);
	virtual int GetClanList(std::vector<ClanList_s>& clans, std::string clanName, int flag, int gameModeID, int playTime, int pageID, int& pageMax);
	virtual int GetClanInfo(int clanID, Clan_s& clan);
	virtual int AddClanStorageItem(int userID, int pageID, CUserInventoryItem& item);
	virtual int DeleteClanStorageItem(int userID, int pageID, int slot);
	virtual int GetClanStorageItem(int userID, int pageID, int slot, CUserInventoryItem& item);
	virtual int GetClanStorageLastItems(int userID, std::vector<RewardItem>& items); // TODO
	virtual int GetClanStoragePage(int userID, ClanStoragePage& clanStoragePage);
	virtual int GetClanStorageHistory(int userID, ClanStorageHistory& clanStorageHistory);
	virtual int GetClanStorageAccessGrade(int userID, std::vector<int>& accessGrade);
	virtual int UpdateClanStorageAccessGrade(int userID, int pageID, int accessGrade);
	virtual int GetClanUserList(int id, bool byUser, std::vector<ClanUser>& users);
	virtual int GetClanMemberList(int userID, std::vector<ClanUser>& users);
	virtual int GetClanMemberJoinUserList(int userID, std::vector<ClanUserJoinRequest>& users);
	virtual int GetClan(int userID, int flag, Clan_s& clan);
	virtual int GetClanMember(int userID, ClanUser& clanUser);
	virtual int UpdateClan(int userID, int flag, Clan_s clan);
	virtual int UpdateClanMemberGrade(int userID, const std::string& userName, int newGrade, ClanUser& targetMember);
	virtual int ClanReject(int userID, const std::string& userName);
	virtual int ClanRejectAll(int userID);
	virtual int ClanApprove(int userID, const std::string& userName);
	virtual int IsClanWithMarkExists(int markID);
	virtual int ClanInvite(int userID, const std::string& gameName, IUser*& destUser, int& clanID);
	virtual int ClanKick(int userID, const std::string& userName);
	virtual int ClanMasterDelegate(int userID, const std::string& userName);
	virtual int IsClanExists(const std::string& clanName);

	// quest event related
	virtual int GetQuestEventProgress(int userID, int questID, UserQuestProgress& questProgress);
	virtual int UpdateQuestEventProgress(int userID, const UserQuestProgress& questProgress);
	virtual int GetQuestEventTaskProgress(int userID, int questID, int taskID, UserQuestTaskProgress& taskProgress);
	virtual int UpdateQuestEventTaskProgress(int userID, int questID, const UserQuestTaskProgress& taskProgress);
	virtual bool IsQuestEventTaskFinished(int userID, int questID, int taskID);

	virtual int IsUserExists(int userID);
	virtual int IsUserExists(const std::string& userName, bool searchByUserName = true);

	// suspect system
	virtual int SuspectAddAction(const std::vector<unsigned char>& hwid, int actionID);
	virtual int IsUserSuspect(int userID);

	virtual void OnMinuteTick(time_t curTime);
	virtual void OnDayTick();
	virtual void OnWeekTick();

	virtual std::map<int, UserBan> GetUserBanList();
	virtual std::vector<int> GetUsers(int lastLoginTime = 0);

	virtual int UpdateIPBanList(const std::string& ip, bool remove = false);
	virtual std::vector<std::string> GetIPBanList();
	virtual bool IsIPBanned(const std::string& ip);

	virtual int UpdateHWIDBanList(const std::vector<unsigned char>& hwid, bool remove = false);
	virtual std::vector<std::vector<unsigned char>> GetHWIDBanList();
	virtual bool IsHWIDBanned(std::vector<unsigned char>& hwid);

	virtual void ResetQuestEvent(int questID);

	virtual void CreateTransaction();
	virtual bool CommitTransaction();

private:
	void ExecCalcStart();
	void ExecCalcEnd(const std::string& funcName);

	std::chrono::high_resolution_clock::time_point m_StartTime;

	// pointer to the real database
	IUserDatabase* m_pDatabase;
};

extern CUserDatabaseProxy g_UserDatabase;

#endif