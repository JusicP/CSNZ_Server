PRAGMA user_version = 2;
CREATE TABLE IF NOT EXISTS "UserDist" (
	"userIDNext" INT,
	"clanIDNext" INT
);
INSERT INTO "UserDist" VALUES (
	1, 1 );
CREATE TABLE IF NOT EXISTS "User" (
	"userID"			INT NOT NULL,
	"userName"			VARCHAR(32),
	"password"			VARCHAR(32),
	"registerTime"		TIMESTAMP DEFAULT 0,
	"registerIP"		VARCHAR(32),
	"firstLogonTime"	TIMESTAMP DEFAULT 0,
	"lastLogonTime"		TIMESTAMP DEFAULT 0,
	"lastIP"			VARCHAR(32) DEFAULT 0,
	"lastHWID"			BLOB DEFAULT '',
	PRIMARY KEY("userID")
);
CREATE TABLE IF NOT EXISTS "UserCharacter" (
	"userID"			INT NOT NULL,
	"gameName"			VARCHAR(32),
	"exp"				BIGINT DEFAULT 0,
	"level"				INT DEFAULT 1,
	"points"			BIGINT DEFAULT 0,
	"cash"				BIGINT DEFAULT 0,
	"battles"			INT DEFAULT 0,
	"win"				INT DEFAULT 0,
	"kills"				INT DEFAULT 0,
	"deaths"			INT DEFAULT 0,
	"nation"			INT DEFAULT 0,
	"city"				INT DEFAULT 0,
	"town"				INT DEFAULT 0,
	"leagueID"			INT DEFAULT 0,
	"passwordBoxes"		INT DEFAULT 0,
	"mileagePoints"		INT DEFAULT 0,
	"honorPoints"		INT DEFAULT 0,
	"prefixID"			INT DEFAULT 0,
	"achievementList"	TEXT DEFAULT '',
	"titles"			VARCHAR(32) DEFAULT '0,0,0,0,0',
	"clanID"			INT DEFAULT 0,
	"tournament"		INT DEFAULT 0,
	FOREIGN KEY("userID") REFERENCES "User"("userID") ON DELETE CASCADE,
	PRIMARY KEY("userID")
);
CREATE TABLE IF NOT EXISTS "UserCharacterExtended" (
	"userID"					INT NOT NULL,
	"gameMaster"				BOOLEAN DEFAULT 0,
	"killsToGetGachaponItem"	INT DEFAULT 0,
	"nextInventorySlot"			INT DEFAULT 1,
	"config"					BLOB DEFAULT '',
	"curLoadout"				INT DEFAULT 0,
	"characterID"				INT DEFAULT 0,
	"banSettings"				INT DEFAULT 2,
	"_2ndPassword"				BLOB DEFAULT '',
	"securityQuestion"			INT DEFAULT 0,
	"securityAnswer"			BLOB DEFAULT '',
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE,
	PRIMARY KEY("userID")
);
CREATE TABLE IF NOT EXISTS "UserSession" (
	"userID"		INT NOT NULL,
	"ip"			VARCHAR(32),
	"uuid"			VARCHAR(32),
	"hwid"			BLOB DEFAULT '',
	"status"		INT DEFAULT 0,
	"sessionTime"	INT DEFAULT 0,
	FOREIGN KEY("userID") REFERENCES "User"("userID") ON DELETE CASCADE,
	PRIMARY KEY("userID")
);
CREATE TABLE IF NOT EXISTS "UserSessionHistory" (
	"userID"		INT NOT NULL,
	"ip"			VARCHAR(32),
	"hwid"			BLOB DEFAULT '',
	"sessionDate"	INT DEFAULT 0,
	"sessionTime"	INT DEFAULT 0,
	FOREIGN KEY("userID") REFERENCES "User"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserInventory" (
	"userID"			INT NOT NULL,
	"slot"				INT NOT NULL,
	"itemID"			INT,
	"count"				INT,
	"status"			INT,
	"inUse"				INT,
	"obtainDate"		INT,
	"expiryDate"		INT,
	"isClanItem"    	INT,
	"enhancementLevel"  INT,
	"enhancementExp"    INT,
	"enhanceValue"		INT,
	"paintID"    		INT,
	"paintIDList"    	TEXT DEFAULT '',
	"partSlot1"    		INT,
	"partSlot2"   	 	INT,
	"lockStatus"		INT,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserBan" (
	"userID"	INT NOT NULL,
	"type"		INT,
	"reason"	VARCHAR(64),
	"term"		INT,
	FOREIGN KEY("userID") REFERENCES "User"("userID") ON DELETE CASCADE,
	PRIMARY KEY("userID")
);
CREATE TABLE IF NOT EXISTS "UserLoadout" (
	"userID"	INT NOT NULL,
	"loadoutID"	INT,
	"slot0"		INT DEFAULT 0,
	"slot1"		INT DEFAULT 0,
	"slot2"		INT DEFAULT 0,
	"slot3"		INT DEFAULT 0,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserBuyMenu" (
	"userID"	INT NOT NULL,
	"buyMenuID"	INT,
	"slot1"		INT DEFAULT 0,
	"slot2"		INT DEFAULT 0,
	"slot3"		INT DEFAULT 0,
	"slot4"		INT DEFAULT 0,
	"slot5"		INT DEFAULT 0,
	"slot6"		INT DEFAULT 0,
	"slot7"		INT DEFAULT 0,
	"slot8"		INT DEFAULT 0,
	"slot9"		INT DEFAULT 0,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserFastBuy" (
	"userID"	INT NOT NULL,
	"fastBuyID"	INT,
	"name"		VARCHAR(32),
	"items"		TEXT,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserBookmark" (
	"userID"		INT NOT NULL,
	"bookmarkID"	INT,
	"itemID"		INT NOT NULL,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserQuestStat" (
	"userID"						INT NOT NULL,
	"continiousSpecialQuest"		INT,
	"dailyMissionsCompletedToday"	INT,
	"dailyMissionsCleared"			INT,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE,
	PRIMARY KEY("userID")
);
CREATE TABLE IF NOT EXISTS "UserQuestProgress" (
	"userID"	INT NOT NULL,
	"questID"	INT,
	"status"	INT,
	"favourite"	BOOLEAN,
	"started"	BOOLEAN,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserQuestTaskProgress" (
	"userID"	INT NOT NULL,
	"questID"	INT,
	"taskID"	INT,
	"unitsDone"	INT,
	"taskVar"	INT,
	"finished"	BOOLEAN,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserCostumeLoadout" (
	"userID"	INT NOT NULL,
	"head"		INT,
	"back"		INT,
	"arm"		INT,
	"pelvis"	INT,
	"face"		INT,
	"tattoo"	INT,
	"pet"		INT,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserZBCostumeLoadout" (
	"userID"	INT NOT NULL,
	"slot"		INT,
	"itemID"	INT,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserRewardNotice" (
	"userID"	INT NOT NULL,
	"rewardID"	INT,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserExpiryNotice" (
	"userID"	INT NOT NULL,
	"itemID"	INT,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserDailyReward" (
	"userID"		INT NOT NULL,
	"day"			INT,
	"canGetReward"	INT,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserDailyRewardItems" (
	"userID"	INT NOT NULL,
	"itemID"	INT,
	"count"		INT,
	"duration"	INT,
	"eventFlag"	INT,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserMiniGameBingo" (
	"userID"	INT NOT NULL,
	"status"	INT,
	"canPlay"	BOOLEAN,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserMiniGameBingoSlot" (
	"userID"	INT NOT NULL,
	"number"	INT,
	"opened"	BOOLEAN,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserMiniGameBingoPrizeSlot" (
	"userID"	INT NOT NULL,
	"idx"	    INT,
	"opened"	BOOLEAN,
	"itemID"	INT,
	"count"	    INT,
	"duration"	INT,
	"relatesTo"	VARCHAR(32),
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserRank" (
	"userID"	INT NOT NULL,
	"tierOri"	INT DEFAULT 71,
	"tierZM"	INT DEFAULT 71,
	"tierZPVE"	INT DEFAULT 71,
	"tierDM"	INT DEFAULT 71,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE,
	PRIMARY KEY("userID")
);
CREATE TABLE IF NOT EXISTS "UserQuestEventProgress" (
	"userID"	INT NOT NULL,
	"questID"	INT,
	"status"	INT,
	"favourite"	BOOLEAN,
	"started"	BOOLEAN,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserQuestEventTaskProgress" (
	"userID"	INT NOT NULL,
	"questID"	INT,
	"taskID"	INT,
	"unitsDone"	INT,
	"taskVar"	INT,
	"finished"	BOOLEAN,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserBanList" (
	"userID"	INT NOT NULL,
	"gameName"	VARCHAR(32) NOT NULL,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserRestore" (
	"userID" 	    	INT NOT NULL,
	"channelServerID" 	INT NOT NULL,
	"channelID" 	    INT NOT NULL,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE,
	PRIMARY KEY("userID")
);
CREATE TABLE IF NOT EXISTS "Clan" (
	"clanID"	   INT NOT NULL,
	"masterUserID" INT NOT NULL,
	"name" 		   VARCHAR(32),
	"description"  VARCHAR(255),
	"notice"	   VARCHAR(255),
	"gameModeID"   INT,
	"mapID"        INT,
	"time"		   INT,
	"memberCount"  INT,
	"expBonus"	   INT,
	"pointBonus"   INT,
	"region"	   INT,
	"joinMethod"   INT,
	"score"		   INT,
	"markID"       INT,
	"markColor"	   INT,
	"markChangeCount"	   INT,
	"maxMemberCount"	INT,
	PRIMARY KEY("clanID")
);
CREATE TABLE IF NOT EXISTS "ClanMember" (
	"clanID"	   INT NOT NULL,
	"userID" 	   INT NOT NULL,
	"memberGrade"  INT,
	PRIMARY KEY("userID"),
	FOREIGN KEY("clanID") REFERENCES "Clan"("clanID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "ClanMemberRequest" (
	"clanID"	     INT NOT NULL,
	"userID" 	   	 INT NOT NULL,
	"inviterUserID"  INT,
	"date" 	  	     INT NOT NULL,
	PRIMARY KEY("userID"),
	FOREIGN KEY("clanID") REFERENCES "Clan"("clanID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "ClanStoragePage" (
	"clanID"	   INT NOT NULL,
	"pageID" 	   INT,
	"accessGrade"  INT,
	FOREIGN KEY("clanID") REFERENCES "Clan"("clanID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "ClanStorageItem" (
	"clanID"	   INT NOT NULL,
	"pageID" 	   INT,
	"slot"		   INT,
	"itemID"	   INT NOT NULL,
	"itemCount"	   INT NOT NULL,
	"itemDuration" INT,
	"itemEnhValue" INT,
	FOREIGN KEY("clanID") REFERENCES "Clan"("clanID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "ClanChronicle" (
	"clanID"	    INT NOT NULL,
	"date" 	  	    INT NOT NULL,
	"type" 			INT,
	"string"	    VARCHAR(32),
	FOREIGN KEY("clanID") REFERENCES "Clan"("clanID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "ClanInvite" (
	"clanID"		INT NOT NULL,
	"userID"		INT NOT NULL,
	"destUserID"	INT NOT NULL,
	FOREIGN KEY("clanID") REFERENCES "Clan"("clanID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "RelationProducts" (
	"relationProductID"	   INT NOT NULL,
	"itemID"			   INT,
	"itemCount"			   INT,
	"itemDuration"		   INT,
	"isPoints"			   INT,
	"price"				   INT,
	"additionalPoints"	   INT,
	"additionalClanPoints" INT,
	"adType"			   INT,
	"limitMax"			   INT,
	"buyCount"			   INT,
	PRIMARY KEY("relationProductID")
);
CREATE TABLE IF NOT EXISTS "UserSurveyAnswer" (
	"surveyID"		INT,
	"questionID"	INT,
	"userID"		INT,
	"answer"		VARCHAR(255)
);
CREATE TABLE IF NOT EXISTS "SuspectAction" (
	"hwid"	    	BLOB,
	"actionID" 	    INT,
	"timeStamp"	    INT
);
CREATE TABLE IF NOT EXISTS "TimeConfig" (
	"nextDayResetTime"	    INT,
	"nextWeekResetTime" 	INT
);
INSERT INTO "TimeConfig" VALUES (
	0, 0
);
CREATE TABLE IF NOT EXISTS "UserMiniGameWeaponReleaseItemProgress" (
	"userID"	INT NOT NULL,
	"slot"		INT,
	"character"	CHAR,
	"opened"	BOOLEAN,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "UserMiniGameWeaponReleaseCharacters" (
	"userID"	INT NOT NULL,
	"character"	CHAR,
	"count"		INT,
	FOREIGN KEY("userID") REFERENCES "UserCharacter"("userID") ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS "IPBanList" (
	"ip"	VARCHAR(32) UNIQUE
);
CREATE TABLE IF NOT EXISTS "HWIDBanList" (
	"hwid"	BLOB UNIQUE
);