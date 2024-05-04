#ifdef DB_SQLITE
#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include "manager.h"
#include "interface/iuserdatabase.h"

class IUser;
class CUserInventory;
class CUserInventoryItem;
class CUserLoadout;
class CUserCostumeLoadout;
class CUserBuyMenu;
class CUserFastBuy;
struct RewardItem;
struct UserQuestProgress;
class CClan;

class CUserDatabaseSQLite : public CBaseManager<IUserDatabase>
{
public:
	CUserDatabaseSQLite();
	~CUserDatabaseSQLite();

	virtual bool Init();

	bool ExecuteScript(std::string scriptPath);

	int Login(const std::string& userName, const std::string& password, IExtendedSocket* socket, UserBan& ban, UserRestoreData* restoreData);
	int AddToRestoreList(int userID, int channelServerID, int channelID);
	int Register(const std::string& userName, const std::string& password, const std::string& ip);
	int GetUserSessions(std::vector<UserSession>& sessions);
	int DropSession(int userID);
	int DropSessions();

	// user related
	int AddInventoryItem(int userID, CUserInventoryItem& item);
	int AddInventoryItems(int userID, std::vector<CUserInventoryItem>& items); // TODO
	int UpdateInventoryItem(int userID, const CUserInventoryItem& item);
	int GetInventoryItems(int userID, std::vector<CUserInventoryItem>& items);
	int GetInventoryItemsByID(int userID, int itemID, std::vector<CUserInventoryItem>& items);
	int GetInventoryItemBySlot(int userID, int slot, CUserInventoryItem& item);
	int GetFirstActiveItemByItemID(int userID, int itemID, CUserInventoryItem& item);
	int IsInventoryFull(int userID);
	int GetUserData(int userID, CUserData& data);
	int UpdateUserData(int userID, CUserData data);
	int CreateCharacter(int userID, const std::string& gameName);
	int DeleteCharacter(int userID);
	int GetCharacter(int userID, CUserCharacter& character);
	int UpdateCharacter(int userID, CUserCharacter& character);
	int GetCharacterExtended(int userID, CUserCharacterExtended& character);
	int UpdateCharacterExtended(int userID, CUserCharacterExtended& character);
	int GetUserBan(int userID, UserBan& ban);
	int UpdateUserBan(int userID, UserBan ban);
	int GetLoadouts(int userID, CUserLoadout& loadout);
	int UpdateLoadout(int userID, int loadoutID, int slot, int itemID);
	int GetFastBuy(int userID, std::vector<CUserFastBuy>& fastBuy);
	int UpdateFastBuy(int userID, int slot, const std::string& name, const std::vector<int>& items);
	int GetBuyMenu(int userID, std::vector<CUserBuyMenu>& buyMenu);
	int UpdateBuyMenu(int userID, int subMenuID, int subMenuSlot, int itemID);
	int GetBookmark(int userID, std::vector<int>& bookmark);
	int UpdateBookmark(int userID, int bookmarkID, int itemID);
	int GetCostumeLoadout(int userID, CUserCostumeLoadout& loadout);
	int UpdateCostumeLoadout(int userID, CUserCostumeLoadout& loadout, int zbSlot);
	int GetRewardNotices(int userID, std::vector<int>& notices);
	int UpdateRewardNotices(int userID, int rewardID);
	int GetExpiryNotices(int userID, std::vector<int>& notices);
	int UpdateExpiryNotices(int userID, int itemID);
	int GetDailyRewards(int userID, UserDailyRewards& dailyRewards);
	int UpdateDailyRewards(int userID, UserDailyRewards& dailyRewards);
	int GetQuestsProgress(int userID, std::vector<UserQuestProgress>& questsProgress);
	int GetQuestProgress(int userID, int questID, UserQuestProgress& questProgress);
	int UpdateQuestProgress(int userID, UserQuestProgress& questProgress);
	int GetQuestTaskProgress(int userID, int questID, int taskID, UserQuestTaskProgress& taskProgress);
	int UpdateQuestTaskProgress(int userID, int questID, UserQuestTaskProgress& taskProgress);
	bool IsQuestTaskFinished(int userID, int questID, int taskID);
	int GetQuestStat(int userID, int flag, UserQuestStat& stat);
	int UpdateQuestStat(int userID, int flag, UserQuestStat& stat);
	int GetBingoProgress(int userID, UserBingo& bingo);
	int UpdateBingoProgress(int userID, UserBingo& bingo);
	int GetBingoSlot(int userID, std::vector<UserBingoSlot>& slots);
	int UpdateBingoSlot(int userID, std::vector<UserBingoSlot>& slots, bool remove = false);
	int GetBingoPrizeSlot(int userID, std::vector<UserBingoPrizeSlot>& prizes);
	int UpdateBingoPrizeSlot(int userID, std::vector<UserBingoPrizeSlot>& prizes, bool remove = false);
	int GetUserRank(int userID, CUserCharacter& character); // to review
	int UpdateUserRank(int userID, CUserCharacter& character); // to review
	int GetBanList(int userID, std::vector<std::string>& banList);
	int UpdateBanList(int userID, std::string gameName, bool remove = false);
	bool IsInBanList(int userID, int destUserID);
	bool IsSurveyAnswered(int userID, int surveyID);
	int SurveyAnswer(int userID, UserSurveyAnswer& answer);

	int GetWeaponReleaseRows(int userID, std::vector<UserWeaponReleaseRow>& rows);
	int GetWeaponReleaseRow(int userID, UserWeaponReleaseRow& row);
	int UpdateWeaponReleaseRow(int userID, UserWeaponReleaseRow& row);

	int GetWeaponReleaseCharacters(int userID, std::vector<UserWeaponReleaseCharacter>& characters, int& totalCount);
	int GetWeaponReleaseCharacter(int userID, UserWeaponReleaseCharacter& character);
	int UpdateWeaponReleaseCharacter(int userID, UserWeaponReleaseCharacter& character);

	int SetWeaponReleaseCharacter(int userID, int weaponSlot, int slot, int character, bool opened);

	int GetAddons(int userID, std::vector<int>& addons);
	int SetAddons(int userID, std::vector<int>& addons);

	int GetUsersAssociatedWithIP(const std::string& ip, std::vector<CUserData>& userData);
	int GetUsersAssociatedWithHWID(const std::vector<unsigned char>& hwid, std::vector<CUserData>& userData);

	// clan related
	int CreateClan(ClanCreateConfig& clanCfg);
	int JoinClan(int userID, int clanID, std::string& clanName);
	int CancelJoin(int userID, int clanID);
	int LeaveClan(int userID);
	int DissolveClan(int userID);
	int GetClanList(std::vector<ClanList_s>& clans, std::string clanName, int flag, int gameModeID, int playTime, int pageID, int& pageMax);
	int GetClanInfo(int clanID, Clan_s& clan);
	int AddClanStorageItem(int userID, int pageID, CUserInventoryItem& item);
	int DeleteClanStorageItem(int userID, int pageID, int slot);
	int GetClanStorageItem(int userID, int pageID, int slot, CUserInventoryItem& item);
	int GetClanStorageLastItems(int userID, std::vector<RewardItem>& items); // TODO
	int GetClanStoragePage(int userID, ClanStoragePage& clanStoragePage);
	int GetClanStorageHistory(int userID, ClanStorageHistory& clanStorageHistory);
	int GetClanStorageAccessGrade(int userID, std::vector<int>& accessGrade);
	int UpdateClanStorageAccessGrade(int userID, int pageID, int accessGrade);
	int GetClanUserList(int id, bool byUser, std::vector<ClanUser>& users);
	int GetClanMemberList(int userID, std::vector<ClanUser>& users);
	int GetClanMemberJoinUserList(int userID, std::vector<ClanUserJoinRequest>& users);
	int GetClan(int userID, int flag, Clan_s& clan);
	int GetClanMember(int userID, ClanUser& clanUser);
	int UpdateClan(int userID, int flag, Clan_s clan);
	int UpdateClanMemberGrade(int userID, const std::string& userName, int newGrade, ClanUser& targetMember);
	int ClanReject(int userID, const std::string& userName);
	int ClanRejectAll(int userID);
	int ClanApprove(int userID, const std::string& userName);
	int IsClanWithMarkExists(int markID);
	int ClanInvite(int userID, const std::string& gameName, IUser*& destUser, int& clanID);
	int ClanKick(int userID, const std::string& userName);
	int ClanMasterDelegate(int userID, const std::string& userName);
	int IsClanExists(const std::string& clanName);

	// quest event related
	int GetQuestEventProgress(int userID, int questID, UserQuestProgress& questProgress);
	int UpdateQuestEventProgress(int userID, const UserQuestProgress& questProgress);
	int GetQuestEventTaskProgress(int userID, int questID, int taskID, UserQuestTaskProgress& taskProgress);
	int UpdateQuestEventTaskProgress(int userID, int questID, const UserQuestTaskProgress& taskProgress);
	bool IsQuestEventTaskFinished(int userID, int questID, int taskID);

	int IsUserExists(int userID);
	int IsUserExists(const std::string&, bool searchByUserName = true);

	// suspect system
	int SuspectAddAction(const std::vector<unsigned char>& hwid, int actionID);
	int IsUserSuspect(int userID);

	virtual void OnMinuteTick(time_t curTime);
	void OnDayTick();
	void OnWeekTick();

	std::map<int, UserBan> GetUserBanList();
	std::vector<int> GetUsers(int lastLoginTime = 0);

	int UpdateIPBanList(const std::string& ip, bool remove = false);
	std::vector<std::string> GetIPBanList();
	bool IsIPBanned(const std::string& ip);

	int UpdateHWIDBanList(const std::vector<unsigned char>& hwid, bool remove = false);
	std::vector<std::vector<unsigned char>> GetHWIDBanList();
	bool IsHWIDBanned(std::vector<unsigned char>& hwid);

	void PrintUserList();

	void LoadBackup(const std::string& backupDate);
	void PrintBackupList();
	void ResetQuestEvent(int eventID);

	void WriteUserStatistic(const std::string& fdate, const std::string& sdate);

	void CreateTransaction();
	bool CommitTransaction();

private:
	bool CheckForTables();
	bool UpgradeDatabase(int& currentDatabaseVer);
	bool ExecuteOnce();

	SQLite::Database m_Database;
	SQLite::Transaction* m_pTransaction;
	bool m_bInited;
};

#endif