#pragma once

#include "imanager.h"
#include "definitions.h"

#include <string>
#include <vector>

class CChannel;
class CUserInventoryItem;
class IUser;
class CSendPacket;
class IExtendedSocket;
class IRoom;
class CRoomSettings;
class CQuestTask;
class CQuest;
class CGameMatch;
class CUserFastBuy;

class IPacketManager : public IBaseManager
{
public:
	virtual CSendPacket* CreatePacket(IExtendedSocket* socket, int msgID) = 0;

	virtual void SendUMsgNoticeMsgBoxToUuid(IExtendedSocket* socket, const std::string& text) = 0;
	virtual void SendUMsgNoticeMessageInChat(IExtendedSocket* socket, const std::string& text) = 0;
	virtual void SendUMsgSystemReply(IExtendedSocket* socket, int type, const std::string& msg, const std::vector<std::string>& additionalText = {}) = 0;
	virtual void SendUMsgUserMessage(IExtendedSocket* socket, int type, const std::string& senderName, const std::string& text, int whisperType = 0) = 0;
	virtual void SendUMsgNotice(IExtendedSocket* socket, const Notice_s& notice, bool unk = 1) = 0;
	virtual void SendUMsgExpiryNotice(IExtendedSocket* socket, const std::vector<int>& expiryItems) = 0;
	virtual void SendUMsgRewardNotice(IExtendedSocket* socket, const RewardNotice& reward, std::string title = "", std::string description = "", bool inGame = false, bool scen = false) = 0;
	virtual void SendUMsgRewardSelect(IExtendedSocket* socket, Reward* reward) = 0;

	virtual void SendServerList(IExtendedSocket* socket) = 0;

	virtual void SendStatistic(IExtendedSocket* socket) = 0;

	virtual void SendInventoryAdd(IExtendedSocket* socket, const std::vector<CUserInventoryItem>& items, int curSlot = 0) = 0;
	virtual void SendInventoryRemove(IExtendedSocket* socket, const std::vector<CUserInventoryItem>& items, bool gameSlot = true) = 0;

	virtual void SendDefaultItems(IExtendedSocket* socket, const std::vector<CUserInventoryItem>& items) = 0;

	virtual void SendVersion(IExtendedSocket* socket, int result) = 0;

	virtual void SendUserStart(IExtendedSocket* socket, int userID, const std::string& userName, const std::string& gameName, bool firstConnect) = 0;
	virtual void SendUserUpdateInfo(IExtendedSocket* socket, IUser* user, const CUserCharacter& character) = 0;
	virtual void SendUserSurvey(IExtendedSocket* socket, const Survey& survey) = 0;
	virtual void SendUserSurveyReply(IExtendedSocket* socket, int result) = 0;

	virtual void SendOption(IExtendedSocket* socket, std::vector<unsigned char>& config) = 0;
	virtual void SendOptionUnk(IExtendedSocket* socket) = 0;
	virtual void SendOptionUnk2(IExtendedSocket* socket) = 0;
	virtual void SendOptionUnk3(IExtendedSocket* socket) = 0;

	virtual void SendMetadataMaplist(IExtendedSocket* socket) = 0;
	virtual void SendMetadataClientTable(IExtendedSocket* socket) = 0;
	virtual void SendMetadataWeaponParts(IExtendedSocket* socket) = 0;
	virtual void SendMetadataModelist(IExtendedSocket* socket) = 0;
	virtual void SendMetadataMatchOption(IExtendedSocket* socket) = 0;
	virtual void SendMetadataItemBox(IExtendedSocket* socket, const std::vector<ItemBoxItem>& items) = 0;
	virtual void SendMetadataEncyclopedia(IExtendedSocket* socket) = 0;
	virtual void SendMetadataGameModeList(IExtendedSocket* socket) = 0;
	virtual void SendMetadataReinforceMaxLvl(IExtendedSocket* socket) = 0;
	virtual void SendMetadataReinforceMaxEXP(IExtendedSocket* socket) = 0;
	virtual void SendMetadataUnk8(IExtendedSocket* socket) = 0;
	virtual void SendMetadataProgressUnlock(IExtendedSocket* socket) = 0;
	virtual void SendMetadataWeaponPaint(IExtendedSocket* socket) = 0;
	virtual void SendMetadataUnk3(IExtendedSocket* socket) = 0;
	virtual void SendMetadataReinforceItemsExp(IExtendedSocket* socket) = 0;
	virtual void SendMetadataItemExpireTime(IExtendedSocket* socket) = 0;
	virtual void SendMetadataUnk20(IExtendedSocket* socket) = 0;
	virtual void SendMetadataUnk15(IExtendedSocket* socket) = 0;
	virtual void SendMetadataRandomWeaponList(IExtendedSocket* socket) = 0;
	virtual void SendMetadataHash(IExtendedSocket* socket) = 0;
	virtual void SendMetadataUnk31(IExtendedSocket* socket) = 0;
	virtual void SendMetadataHonorMoneyShop(IExtendedSocket* socket) = 0;
	virtual void SendMetadataScenarioTX_Common(IExtendedSocket* socket) = 0;
	virtual void SendMetadataScenarioTX_Dedi(IExtendedSocket* socket) = 0;
	virtual void SendMetadataShopItemList_Dedi(IExtendedSocket* socket) = 0;
	virtual void SendMetadataZBCompetitive(IExtendedSocket* socket) = 0;
	virtual void SendMetadataUnk43(IExtendedSocket* socket) = 0;
	virtual void SendMetadataUnk49(IExtendedSocket* socket) = 0;
	virtual void SendMetadataWeaponProp(IExtendedSocket* socket) = 0;
	virtual void SendMetadataPPSystem(IExtendedSocket* socket) = 0;
	virtual void SendMetadataCodisData(IExtendedSocket* socket) = 0;
	virtual void SendMetadataItem(IExtendedSocket* socket) = 0;
	virtual void SendMetadataModeEvent(IExtendedSocket* socket) = 0;

	virtual void SendGameMatchInfo(IExtendedSocket* socket) = 0;
	virtual void SendGameMatchUnk(IExtendedSocket* socket) = 0;
	virtual void SendGameMatchUnk9(IExtendedSocket* socket) = 0;
	virtual void SendGameMatchFailMessage(IExtendedSocket* socket, int type) = 0;

	virtual void SendReply(IExtendedSocket* socket, int type) = 0;

	virtual void SendItemUnk1(IExtendedSocket* socket) = 0;
	virtual void SendItemUnk3(IExtendedSocket* socket) = 0;
	virtual void SendItemEquipTattoo(IExtendedSocket* socket) = 0;
	virtual void SendItemDailyRewardsUpdate(IExtendedSocket* socket, const UserDailyRewards& dailyRewards) = 0;
	virtual void SendItemDailyRewardsSpinResult(IExtendedSocket* socket, const RewardItem& item) = 0;
	virtual void SendItemOpenDecoderResult(IExtendedSocket* socket, const ItemBoxOpenResult& result) = 0;
	virtual void SendItemOpenDecoderErrorReply(IExtendedSocket* socket, ItemBoxError code) = 0;
	virtual void SendItemEnhanceResult(IExtendedSocket* socket, const EnhResult& result) = 0;
	virtual void SendItemWeaponPaintReply(IExtendedSocket* socket) = 0;
	virtual void SendItemPartCheck(IExtendedSocket* socket, int slot, int partNum) = 0;
	virtual void SendItemGachapon(IExtendedSocket* socket, int gachaponItem) = 0;

	virtual void SendLobbyJoin(IExtendedSocket* socket, CChannel* channel) = 0;
	virtual void SendLobbyUserJoin(IExtendedSocket* socket, IUser* joinedUser) = 0;
	virtual void SendLobbyUserLeft(IExtendedSocket* socket, IUser* user) = 0;

	virtual void SendRoomListFull(IExtendedSocket* socket, const std::vector<IRoom*>& rooms) = 0;
	virtual void SendRoomListAdd(IExtendedSocket* socket, IRoom* room) = 0;
	virtual void SendRoomListUpdate(IExtendedSocket* socket, IRoom* room) = 0;
	virtual void SendRoomListRemove(IExtendedSocket* socket, int roomID) = 0;

	virtual void SendShopUpdate(IExtendedSocket* socket, const std::vector<Product>& products) = 0;
	virtual void SendShopBuyProductReply(IExtendedSocket* socket, int replyCode) = 0;
	virtual void SendShopReply(IExtendedSocket* socket, int replyCode) = 0;
	virtual void SendShopRecommendedProducts(IExtendedSocket* socket, const std::vector<std::vector<int>>& products) = 0;
	virtual void SendShopPopularProducts(IExtendedSocket* socket, const std::vector<int>& products) = 0;
	
	virtual void SendSearchRoomNotice(IExtendedSocket* socket, IRoom* room, const std::string& invitersGameName, const std::string& inviteMsg) = 0;

	virtual void SendRoomCreateAndJoin(IExtendedSocket* socket, IRoom* roomInfo) = 0;
	virtual void SendRoomPlayerJoin(IExtendedSocket* socket, IUser* user, RoomTeamNum num) = 0;
	virtual void SendRoomUpdateSettings(IExtendedSocket* socket, CRoomSettings* newSettings, int low = 0, int lowMid = 0, int highMid = 0, int high = 0) = 0;
	virtual void SendRoomSetUserTeam(IExtendedSocket* socket, IUser* user, int teamNum) = 0;
	virtual void SendRoomSetPlayerReady(IExtendedSocket* socket, IUser* user, RoomReadyStatus readyStatus) = 0;
	virtual void SendRoomSetHost(IExtendedSocket* socket, IUser* user) = 0;
	virtual void SendRoomPlayerLeave(IExtendedSocket* socket, int userId) = 0;
	virtual void SendRoomPlayerLeaveIngame(IExtendedSocket* socket) = 0;
	virtual void SendRoomInviteUserList(IExtendedSocket* socket, IUser* user) = 0;
	virtual void SendRoomGameResult(IExtendedSocket* socket, IRoom* room, CGameMatch* match) = 0;
	virtual void SendRoomKick(IExtendedSocket* socket, int userID) = 0;
	virtual void SendRoomInitiateVoteKick(IExtendedSocket* socket, int userID, int destUserID, int reason) = 0;
	virtual void SendRoomVoteKickResult(IExtendedSocket* socket, bool kick, int userID, int reason) = 0;
	virtual void SendRoomWeaponSurvey(IExtendedSocket* socket, const std::vector<int>& weapons) = 0;
	
	virtual void SendHostOnItemUse(IExtendedSocket* socket, int userId, int itemId) = 0;
	virtual void SendHostServerJoin(IExtendedSocket* socket, int ipAddress, int port, int userId) = 0;
	virtual void SendHostStop(IExtendedSocket* socket) = 0;
	virtual void SendHostLeaveResultWindow(IExtendedSocket* socket) = 0;
	virtual void SendHostUserInventory(IExtendedSocket* socket, int userId, const std::vector<CUserInventoryItem>& items) = 0;
	virtual void SendHostGameStart(IExtendedSocket* socket, int userId) = 0;
	virtual void SendHostZBAddon(IExtendedSocket* socket, int userID, const std::vector<int>& addons) = 0;
	virtual void SendHostJoin(IExtendedSocket* socket, IUser* host) = 0;
	virtual void SendHostFlyerFlock(IExtendedSocket* socket, int type) = 0;
	virtual void SendHostAdBalloon(IExtendedSocket* socket) = 0;
	virtual void SendHostRestart(IExtendedSocket* socket, int newHostUserID, bool host, CGameMatch* match) = 0;

	virtual void SendCharacter(IExtendedSocket* socket) = 0;

	virtual void SendEventAdd(IExtendedSocket* socket, int eventsFlag) = 0;
	virtual void SendEventUnk(IExtendedSocket* socket) = 0;
	virtual void SendEventMainMenuSkin(IExtendedSocket* socket, int skin) = 0;

	virtual void SendMiniGameBingoUpdate(IExtendedSocket* socket, const UserBingo& bingo, const std::vector<UserBingoSlot>& slots, const std::vector<UserBingoPrizeSlot>& prizes) = 0;
	virtual void SendMiniGameWeaponReleaseUpdate(IExtendedSocket* socket, const WeaponReleaseConfig& cfg, const std::vector<UserWeaponReleaseRow>& rows, const std::vector<UserWeaponReleaseCharacter>& characters, int totalCount) = 0;
	virtual void SendMiniGameWeaponReleaseSetCharacter(IExtendedSocket* socket, int status, int weaponSlot, int slot, int character, int charLeft) = 0;
	virtual void SendMiniGameWeaponReleaseUnk2(IExtendedSocket* socket) = 0;
	virtual void SendMiniGameWeaponReleaseIGNotice(IExtendedSocket* socket, char character) = 0;

	virtual void SendQuests(IExtendedSocket* socket, int userID, const std::vector<CQuest*>& quests, const std::vector<UserQuestProgress>& questsProgress, int infoFlag = 0xFFFF, int taskFlag = 0xFF, int rewardFlag = 0xFF, int statFlag = 0xFFFF) = 0;
	//void SendQuestUnk(IExtendedSocket* socket) = 0;
	virtual void SendQuestUpdateMainInfo(IExtendedSocket* socket, int flag, CQuest* quest, const UserQuestProgress& questProgress) = 0;
	virtual void SendQuestUpdateTaskInfo(IExtendedSocket* socket, int flag, int questID, CQuestTask* task, const UserQuestTaskProgress& taskProgress) = 0;
	virtual void SendQuestUpdateRewardInfo(IExtendedSocket* socket, int flag, int questID, const QuestReward_s& reward) = 0;
	virtual void SendQuestUpdateQuestStat(IExtendedSocket* socket, int flag, int honorPoints, const UserQuestStat& stat) = 0;

	virtual void SendFavoriteLoadout(IExtendedSocket* socket, int characterItemID, int currentLoadout, const CUserLoadout& loadouts) = 0;
	virtual void SendFavoriteFastBuy(IExtendedSocket* socket, const std::vector<CUserFastBuy>& fastbuy) = 0;
	virtual void SendFavoriteBuyMenu(IExtendedSocket* socket, const std::vector<CUserBuyMenu>& buyMenu) = 0;
	virtual void SendFavoriteBookmark(IExtendedSocket* socket, const std::vector<int>& bookmark) = 0;

	virtual void SendAlarm(IExtendedSocket* socket, const std::vector<Notice_s>& notices) = 0;

	virtual void SendQuestUnk1(IExtendedSocket* socket) = 0;
	virtual void SendQuestUnk11(IExtendedSocket* socket) = 0;
	virtual void SendQuestUnk12(IExtendedSocket* socket) = 0;
	virtual void SendQuestUnk13(IExtendedSocket* socket) = 0;

	virtual void SendUpdateInfoNicknameChangeReply(IExtendedSocket* socket, int replyCode) = 0;

	virtual void SendTitle(IExtendedSocket* socket, int id) = 0;

	virtual void SendUDPHostData(IExtendedSocket* socket, bool host, int userID, const std::string& ipAddress, int port) = 0;

	virtual void SendHostServerStop(IExtendedSocket* socket) = 0;
	virtual void SendHostServerTransfer(IExtendedSocket* socket, const std::string& ipAddress, int port) = 0;

	virtual void SendClanList(IExtendedSocket* socket, const std::vector<ClanList_s>& clans, int pageID, int pageMax) = 0;
	virtual void SendClanInfo(IExtendedSocket* socket, const Clan_s& clan) = 0;
	virtual void SendClanReply(IExtendedSocket* socket, int replyID, int replyCode, const char* errStr) = 0;
	virtual void SendClanJoinReply(IExtendedSocket* socket, int replyCode, const char* errStr) = 0;
	virtual void SendClanCreateUserList(IExtendedSocket* socket, const std::vector<ClanUser>& users) = 0;
	virtual void SendClanUpdateUserList(IExtendedSocket* socket, const ClanUser& user, bool remove = false) = 0;
	virtual void SendClanStoragePage(IExtendedSocket* socket, const ClanStoragePage& clanStoragePage) = 0;
	virtual void SendClanStorageHistory(IExtendedSocket* socket) = 0;
	virtual void SendClanStorageAccessGrade(IExtendedSocket* socket, const std::vector<int>& accessGrade) = 0;
	virtual void SendClanStorageReply(IExtendedSocket* socket, int replyCode, const char* errStr) = 0;
	virtual void SendClanCreateMemberUserList(IExtendedSocket* socket, const std::vector<ClanUser>& users) = 0;
	virtual void SendClanUpdateMemberUserList(IExtendedSocket* socket, const ClanUser& user, bool remove = false) = 0;
	virtual void SendClanCreateJoinUserList(IExtendedSocket* socket, const std::vector<ClanUserJoinRequest>& users) = 0;
	virtual void SendClanUpdateJoinUserList(IExtendedSocket* socket, const ClanUserJoinRequest& user, bool remove = false) = 0;
	virtual void SendClanDeleteJoinUserList(IExtendedSocket* socket) = 0;
	virtual void SendClanUpdate(IExtendedSocket* socket, int type, int memberGrade, const Clan_s& clan) = 0;
	virtual void SendClanUpdateNotice(IExtendedSocket* socket, const Clan_s& clan) = 0;
	virtual void SendClanMarkColor(IExtendedSocket* socket) = 0;
	virtual void SendClanMarkReply(IExtendedSocket* socket, int replyCode, const char* errStr) = 0;
	virtual void SendClanInvite(IExtendedSocket* socket, const std::string& inviterGameName, int clanID) = 0;
	virtual void SendClanMasterDelegate(IExtendedSocket* socket) = 0;
	virtual void SendClanKick(IExtendedSocket* socket) = 0;
	virtual void SendClanChatMessage(IExtendedSocket* socket, const std::string& gameName, const std::string& message) = 0;

	virtual void SendBanList(IExtendedSocket* socket, const std::vector<std::string>& banList) = 0;
	virtual void SendBanUpdateList(IExtendedSocket* socket, const std::string& gameName, bool remove = false) = 0;
	virtual void SendBanSettings(IExtendedSocket* socket, int settings) = 0;
	virtual void SendBanMaxSize(IExtendedSocket* socket, int maxSize) = 0;

	virtual void SendMessengerUserInfo(IExtendedSocket* socket, int userID, const CUserCharacter& character) = 0;

	virtual void SendRankReply(IExtendedSocket* socket, int replyCode) = 0;
	virtual void SendRankUserInfo(IExtendedSocket* socket, int userID, const CUserCharacter& character) = 0;

	virtual void SendAddonPacket(IExtendedSocket* socket, const std::vector<int>& addons) = 0;

	virtual void SendLeaguePacket(IExtendedSocket* socket) = 0;
	virtual void SendLeagueGaugePacket(IExtendedSocket* socket, int gameModeId) = 0;

	virtual void SendCrypt(IExtendedSocket* socket, int type, unsigned char* key, unsigned char* iv) = 0;

	virtual void SendUpdateInfo(IExtendedSocket* socket) = 0;

	virtual void SendPacketFromFile(IExtendedSocket* socket, const std::string& filename) = 0;
};
