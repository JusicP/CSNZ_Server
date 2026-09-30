#pragma once

#include <string>
#include <vector>

#include "imanager.h"
#include "definitions.h"

class IUser;
class IExtendedSocket;
class CUserInventoryItem;
class CUserFastBuy;

class IUserDatabase : public IBaseManager
{
public:
	virtual int Login(const std::string& userName, const std::string& password, IExtendedSocket* socket, UserBan& ban, UserRestoreData* restoreData) = 0;
	virtual int AddToRestoreList(int userID, int channelServerID, int channelID) = 0;
	virtual int Register(const std::string& userName, const std::string& password, const std::string& ip) = 0;
	virtual int GetUserSessions(std::vector<UserSession>& sessions) = 0;
	virtual int DropSession(int userID) = 0;
	virtual int DropSessions() = 0;

	// user related
	virtual int AddInventoryItem(int userID, CUserInventoryItem& item) = 0;
	virtual int AddInventoryItems(int userID, std::vector<CUserInventoryItem>& items) = 0;
	virtual int UpdateInventoryItem(int userID, const CUserInventoryItem& item, int flag) = 0;
	virtual int UpdateInventoryItems(int userID, std::vector<CUserInventoryItem>& items, int flag) = 0;
	virtual int GetInventoryItems(int userID, std::vector<CUserInventoryItem>& items) = 0;
	virtual int GetInventoryItemsByID(int userID, int itemID, std::vector<CUserInventoryItem>& items) = 0;
	virtual int GetInventoryItemBySlot(int userID, int slot, CUserInventoryItem& item) = 0;
	virtual int GetFirstItemByItemID(int userID, int itemID, CUserInventoryItem& item) = 0;
	virtual int GetFirstActiveItemByItemID(int userID, int itemID, CUserInventoryItem& item) = 0;
	virtual int GetFirstExtendableItemByItemID(int userID, int itemID, CUserInventoryItem& item) = 0;
	virtual int GetInventoryItemsCount(int userID) = 0;
	virtual int IsInventoryFull(int userID) = 0;
	virtual int GetUserData(int userID, CUserData& data) = 0;
	virtual int UpdateUserData(int userID, CUserData data) = 0;
	virtual int CreateCharacter(int userID, const std::string& gameName) = 0;
	virtual int DeleteCharacter(int userID) = 0;
	virtual int GetCharacter(int userID, CUserCharacter& character) = 0;
	virtual int UpdateCharacter(int userID, CUserCharacter& character) = 0;
	virtual int GetCharacterExtended(int userID, CUserCharacterExtended& character) = 0;
	virtual int UpdateCharacterExtended(int userID, CUserCharacterExtended& character) = 0;
	virtual int GetUserBan(int userID, UserBan& ban) = 0;
	virtual int UpdateUserBan(int userID, UserBan ban) = 0;
	virtual int GetLoadouts(int userID, std::vector<std::vector<CUserLoadout>>& loadouts) = 0;
	virtual int UpdateLoadout(int userID, int groupID, int loadoutID, int slot, int itemID) = 0;
	virtual int UpdateLoadoutName(int userID, int groupID, int loadoutID, std::string name) = 0;
	virtual int GetFastBuy(int userID, std::vector<CUserFastBuy>& fastBuy) = 0;
	virtual int UpdateFastBuy(int userID, int slot, const std::string& name, const std::vector<int>& items) = 0;
	virtual int GetBuyMenu(int userID, std::vector<CUserBuyMenu>& buyMenu) = 0;
	virtual int UpdateBuyMenu(int userID, int subMenuID, int subMenuSlot, int itemID) = 0;
	virtual int GetBookmark(int userID, std::vector<std::vector<int>>& bookmark) = 0;
	virtual int UpdateBookmark(int userID, int groupID, int bookmarkID, int itemID) = 0;
	virtual int GetCostumeLoadout(int userID, CUserCostumeLoadout& loadout) = 0;
	virtual int UpdateCostumeLoadout(int userID, CUserCostumeLoadout& loadout, int zbSlot) = 0;
	virtual int GetRewardNotices(int userID, std::vector<int>& notices) = 0;
	virtual int UpdateRewardNotices(int userID, int rewardID) = 0;
	virtual int GetExpiryNotices(int userID, std::vector<int>& notices) = 0;
	virtual int UpdateExpiryNotices(int userID, int itemID) = 0;
	virtual int GetDailyRewards(int userID, UserDailyRewards& dailyRewards) = 0;
	virtual int UpdateDailyRewards(int userID, UserDailyRewards& dailyRewards) = 0;
	virtual int GetQuestsProgress(int userID, std::vector<UserQuestProgress>& questsProgress) = 0;
	virtual int GetQuestProgress(int userID, int questID, UserQuestProgress& questProgress) = 0;
	virtual int UpdateQuestProgress(int userID, UserQuestProgress& questProgress) = 0;
	virtual int GetQuestTaskProgress(int userID, int questID, int taskID, UserQuestTaskProgress& taskProgress) = 0;
	virtual int UpdateQuestTaskProgress(int userID, int questID, UserQuestTaskProgress& taskProgress) = 0;
	virtual bool IsQuestTaskFinished(int userID, int questID, int taskID) = 0;
	virtual int GetQuestStat(int userID, int flag, UserQuestStat& stat) = 0;
	virtual int UpdateQuestStat(int userID, int flag, UserQuestStat& stat) = 0;
	virtual int GetBingoProgress(int userID, UserBingo& bingo) = 0;
	virtual int UpdateBingoProgress(int userID, UserBingo& bingo) = 0;
	virtual int GetBingoSlot(int userID, std::vector<UserBingoSlot>& slots) = 0;
	virtual int UpdateBingoSlot(int userID, std::vector<UserBingoSlot>& slots, bool remove = false) = 0;
	virtual int GetBingoPrizeSlot(int userID, std::vector<UserBingoPrizeSlot>& prizes) = 0;
	virtual int UpdateBingoPrizeSlot(int userID, std::vector<UserBingoPrizeSlot>& prizes, bool remove = false) = 0;
	virtual int GetUserRank(int userID, CUserCharacter& character) = 0; // to review
	virtual int UpdateUserRank(int userID, CUserCharacter& character) = 0; // to review
	virtual int GetBanList(int userID, std::vector<std::string>& banList) = 0;
	virtual int UpdateBanList(int userID, std::string gameName, bool remove = false) = 0;
	virtual bool IsInBanList(int userID, int destUserID) = 0;
	virtual bool IsSurveyAnswered(int userID, int surveyID) = 0;
	virtual int SurveyAnswer(int userID, UserSurveyAnswer& answer) = 0;

	virtual int GetWeaponReleaseRows(int userID, std::vector<UserWeaponReleaseRow>& rows) = 0;
	virtual int GetWeaponReleaseRow(int userID, UserWeaponReleaseRow& row) = 0;
	virtual int UpdateWeaponReleaseRow(int userID, UserWeaponReleaseRow& row) = 0;

	virtual int GetWeaponReleaseCharacters(int userID, std::vector<UserWeaponReleaseCharacter>& characters, int& totalCount) = 0;
	virtual int GetWeaponReleaseCharacter(int userID, UserWeaponReleaseCharacter& character) = 0;
	virtual int UpdateWeaponReleaseCharacter(int userID, UserWeaponReleaseCharacter& character) = 0;

	virtual int SetWeaponReleaseCharacter(int userID, int weaponSlot, int slot, int character, bool opened) = 0;

	virtual int GetAddons(int userID, std::vector<int>& addons) = 0;
	virtual int SetAddons(int userID, std::vector<int>& addons) = 0;

	virtual int GetUsersAssociatedWithIP(const std::string& ip, std::vector<CUserData>& userData) = 0;
	virtual int GetUsersAssociatedWithHWID(const std::vector<unsigned char>& hwid, std::vector<CUserData>& userData) = 0;

	// clan related
	virtual int CreateClan(ClanCreateConfig& clanCfg) = 0;
	virtual int JoinClan(int userID, int clanID, std::string& clanName) = 0;
	virtual int CancelJoin(int userID, int clanID) = 0;
	virtual int LeaveClan(int userID) = 0;
	virtual int DissolveClan(int userID) = 0;
	virtual int GetClanList(std::vector<ClanList_s>& clans, std::string clanName, int flag, int gameModeID, int playTime, int pageID, int& pageMax) = 0;
	virtual int GetClanInfo(int clanID, Clan_s& clan) = 0;
	virtual int AddClanStorageItem(int userID, int pageID, CUserInventoryItem& item) = 0;
	virtual int DeleteClanStorageItem(int userID, int pageID, int slot) = 0;
	virtual int GetClanStorageItem(int userID, int pageID, int slot, CUserInventoryItem& item) = 0;
	virtual int GetClanStorageLastItems(int userID, std::vector<RewardItem>& items) = 0; // TODO
	virtual int GetClanStoragePage(int userID, ClanStoragePage& clanStoragePage) = 0;
	virtual int GetClanStorageHistory(int userID, ClanStorageHistory& clanStorageHistory) = 0;
	virtual int GetClanStorageAccessGrade(int userID, std::vector<int>& accessGrade) = 0;
	virtual int UpdateClanStorageAccessGrade(int userID, int pageID, int accessGrade) = 0;
	virtual int GetClanUserList(int id, bool byUser, std::vector<ClanUser>& users) = 0;
	virtual int GetClanMemberList(int userID, std::vector<ClanUser>& users) = 0;
	virtual int GetClanMemberJoinUserList(int userID, std::vector<ClanUserJoinRequest>& users) = 0;
	virtual int GetClan(int userID, int flag, Clan_s& clan) = 0;
	virtual int GetClanMember(int userID, ClanUser& clanUser) = 0;
	virtual int UpdateClan(int userID, int flag, Clan_s clan) = 0;
	virtual int UpdateClanMemberGrade(int userID, const std::string& userName, int newGrade, ClanUser& targetMember) = 0;
	virtual int ClanReject(int userID, const std::string& userName) = 0;
	virtual int ClanRejectAll(int userID) = 0;
	virtual int ClanApprove(int userID, const std::string& userName) = 0;
	virtual int IsClanWithMarkExists(int markID) = 0;
	virtual int ClanInvite(int userID, const std::string& gameName, IUser*& destUser, int& clanID) = 0;
	virtual int ClanKick(int userID, const std::string& userName) = 0;
	virtual int ClanMasterDelegate(int userID, const std::string& userName) = 0;
	virtual int IsClanExists(const std::string& clanName) = 0;

	// quest event related
	virtual int GetQuestEventProgress(int userID, int questID, UserQuestProgress& questProgress) = 0;
	virtual int UpdateQuestEventProgress(int userID, const UserQuestProgress& questProgress) = 0;
	virtual int GetQuestEventTaskProgress(int userID, int questID, int taskID, UserQuestTaskProgress& taskProgress) = 0;
	virtual int UpdateQuestEventTaskProgress(int userID, int questID, const UserQuestTaskProgress& taskProgress) = 0;
	virtual bool IsQuestEventTaskFinished(int userID, int questID, int taskID) = 0;

	virtual int IsUserExists(int userID) = 0;
	virtual int IsUserExists(const std::string& userName, bool searchByUserName = true) = 0;

	// suspect system
	virtual int SuspectAddAction(const std::vector<unsigned char>& hwid, int actionID) = 0;
	virtual int IsUserSuspect(int userID) = 0;

	virtual void OnMinuteTick(time_t curTime) = 0;
	virtual void OnDayTick() = 0;
	virtual void OnWeekTick() = 0;

	virtual std::map<int, UserBan> GetUserBanList() = 0;
	virtual std::vector<int> GetUsers(int lastLoginTime = 0) = 0;

	virtual int UpdateIPBanList(const std::string& ip, bool remove = false) = 0;
	virtual std::vector<std::string> GetIPBanList() = 0;
	virtual bool IsIPBanned(const std::string& ip) = 0;

	virtual int UpdateHWIDBanList(const std::vector<unsigned char>& hwid, bool remove = false) = 0;
	virtual std::vector<std::vector<unsigned char>> GetHWIDBanList() = 0;
	virtual bool IsHWIDBanned(std::vector<unsigned char>& hwid) = 0;

	virtual bool IsClassModExist(int userID, int slot) = 0;
	virtual int AddUserClassModLoadOut(int userID, int itemslot, const ClassModInfo_t& info) = 0;
	virtual int GetUserClassModLoadOut(int userID, int slot, ClassModInfo_t& info) = 0;
	virtual int UpdateUserClassModLoadOut(int userID, int itemslot, const ClassModInfo_t& info) = 0;

	virtual void ResetQuestEvent(int questID) = 0;

	virtual void CreateTransaction() = 0;
	virtual bool CommitTransaction() = 0;
};