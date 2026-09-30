#include "userdatabase_sqlite.h"
#include "userdatabase_shared.h"
#include "serverconfig.h"
#include "usermanager.h"
#include "packetmanager.h"
#include "itemmanager.h"
#include "questmanager.h"

#include "common/utils.h"

#include "user/userfastbuy.h"
#include "user/userinventoryitem.h"
#ifdef WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

using namespace std;

#define LAST_DB_VERSION 6

//#define OBFUSCATE(data) (string)AY_OBFUSCATE_KEY(data, 'F')
#undef OBFUSCATE
#define OBFUSCATE(data) (char*)AY_OBFUSCATE_KEY(data, 'F')

CUserDatabaseSQLite g_UserDatabase;

CUserDatabaseSQLite::CUserDatabaseSQLite()
try : CBaseManager(REAL_DATABASE_NAME, true, true), m_Database(OBFUSCATE("UserDatabase.db3"), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
	m_bInited = false;
	m_pTransaction = NULL;
}
catch (exception& e)
{
	Logger().Fatal(OBFUSCATE("CUserDatabaseSQLite(): SQLite exception: %s\n"), e.what());
}

CUserDatabaseSQLite::~CUserDatabaseSQLite()
{
}

bool CUserDatabaseSQLite::Init()
{
	if (!m_bInited)
	{
		// set synchronous to OFF to speed up db execution
		m_Database.exec(OBFUSCATE("PRAGMA synchronous=OFF"));
		m_Database.exec(OBFUSCATE("PRAGMA foreign_keys=ON"));

		if (!CheckForTables())
			return false;

		DropSessions();

		ExecuteOnce();

		m_bInited = true;
	}

	return true;
}

// check if tables we need exist, if not create them
bool CUserDatabaseSQLite::CheckForTables()
{
	try
	{
		int dbVer = m_Database.execAndGet(OBFUSCATE("PRAGMA user_version"));
		if (dbVer)
		{
			if (!UpgradeDatabase(dbVer))
			{
				return false;
			}

			if (dbVer != LAST_DB_VERSION)
			{
				Logger().Fatal("CUserDatabaseSQLite::CheckForTables: database version mismatch, got: %d, expected: %d\n", dbVer, LAST_DB_VERSION);
				return false;
			}
		}
		else
		{
			if (!ExecuteScript(OBFUSCATE("Data/SQL/main.sql")))
			{
				Logger().Fatal(OBFUSCATE("CUserDatabaseSQLite::CheckForTables: failed to execute main SQL script. Server may not work properly\n"));
				return false;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Fatal(OBFUSCATE("CUserDatabaseSQLite::CheckForTables: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return false;
	}

	return true;
}	

// upgrades user database to last version
bool CUserDatabaseSQLite::UpgradeDatabase(int& currentDatabaseVer)
{
	bool upgrade = false;
	for (int i = currentDatabaseVer; i < LAST_DB_VERSION; i++)
	{
		if (!ExecuteScript(va(OBFUSCATE("Data/SQL/Update_%d.sql"), i + 1)))
		{
			Logger().Fatal(OBFUSCATE("CUserDatabaseSQLite::UpgradeDatabase: file Update_%d.sql doesn't exist or sql execute returned error, current db ver: %d, last db ver: %d. Script not applied.\n"), i + 1, currentDatabaseVer, LAST_DB_VERSION);
			return false;
		}

		upgrade = true;
	}

	if (upgrade)
	{
		m_Database.exec(va(OBFUSCATE("PRAGMA user_version = %d"), LAST_DB_VERSION));
		currentDatabaseVer = LAST_DB_VERSION;
	}

	return true;
}

// executes sql script
// returns 1 on success, 0 on failure
bool CUserDatabaseSQLite::ExecuteScript(string scriptPath)
{
	FILE* file = fopen(scriptPath.c_str(), OBFUSCATE("rb"));
	if (!file)
		return false;

	try
	{
		fseek(file, 0, SEEK_END);
		long lSize = ftell(file);
		if (lSize <= 0)
		{
			fclose(file);
			return false;
		}
		
		rewind(file);

		char* buffer = (char*)malloc(sizeof(char) * lSize + 1);
		if (buffer == NULL)
		{
			fclose(file);
			return false;
		}

		buffer[lSize] = '\0';

		size_t result = fread(buffer, 1, lSize, file);
		if (result != lSize)
		{
			fclose(file);
			return false;
		}

		SQLite::Transaction transaction(m_Database);

		char* token = strtok(buffer, OBFUSCATE(";"));
		while (token)
		{
			m_Database.exec(token);
			token = strtok(NULL, OBFUSCATE(";"));
		}

		transaction.commit();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::ExecuteScript(%s): database internal error: %s, %d\n"), scriptPath.c_str(), e.what(), m_Database.getErrorCode());

		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}

// executes scripts from the files specified in the script list file and moves that file to ExecuteHistory folder
// returns 1 on success, 0 on failure
bool CUserDatabaseSQLite::ExecuteOnce()
{
	FILE* file = fopen(OBFUSCATE("Data/SQL/executeonce.txt"), OBFUSCATE("rb"));
	if (!file)
		return false;

	fseek(file, 0, SEEK_END);
	long lSize = ftell(file);
	rewind(file);

	char* buffer = (char*)malloc(sizeof(char) * lSize + 1);
	if (buffer == NULL)
	{
		fclose(file);
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::ExecuteOnce: failed to allocate buffer\n"));
		return false;
	}

	buffer[lSize] = '\0';

	size_t result = fread(buffer, 1, lSize, file);
	if (result != lSize)
	{
		fclose(file);
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::ExecuteOnce: failed to read file\n"));
		return false;
	}

	char* token = strtok(buffer, "\n");
	while (token)
	{
		Logger().Info("CUserDatabaseSQLite::ExecuteOnce: executing %s\n", token);
		ExecuteScript(token);
		token = strtok(NULL, "\n");
	}

	fclose(file);

#ifdef WIN32
	_mkdir("Data/SQL/ExecutedHistory");
#else
	mkdir("Data/SQL/ExecutedHistory", 0777);
#endif
	time_t currTime = time(NULL);
	struct tm* pTime = localtime(&currTime);

	char name[MAX_PATH];
	snprintf(name, sizeof(name) / sizeof(char),
		"Data/SQL/ExecutedHistory/executeonce_%d-%.2d-%.2d %.2d-%.2d-%.2d.txt",
		pTime->tm_year + 1900,
		pTime->tm_mon + 1,
		pTime->tm_mday,
		pTime->tm_hour,
		pTime->tm_min,
		pTime->tm_sec
	);

	rename("Data/SQL/executeonce.txt", name);
	return true;
}

// creates a new session for user
// returns > 0 == userID, 0 == database error, -1 == no such user or not in restore list, -2 == user already logged in, -4 == user banned
int CUserDatabaseSQLite::Login(const string& userName, const string& password, IExtendedSocket* socket, UserBan& ban, UserRestoreData* restoreData)
{
	try
	{
		int userID = 0;
		if (restoreData)
		{
			// get userID if transferring server
			SQLite::Statement queryGetUser(m_Database, OBFUSCATE("SELECT userID FROM User WHERE userName = ? LIMIT 1"));
			queryGetUser.bind(1, userName);

			if (!queryGetUser.executeStep())
			{
				return LOGIN_NO_SUCH_USER;
			}

			userID = queryGetUser.getColumn(0);
		}
		else
		{
			SQLite::Statement queryGetUser(m_Database, OBFUSCATE("SELECT userID FROM User WHERE userName = ? AND password = ? LIMIT 1"));
			queryGetUser.bind(1, userName);
			queryGetUser.bind(2, password);

			if (!queryGetUser.executeStep())
			{
				return LOGIN_NO_SUCH_USER;
			}

			userID = queryGetUser.getColumn(0);
		}

		GetUserBan(userID, ban);
		if (ban.banType)
		{
			return LOGIN_USER_BANNED;
		}

		// check if user present in UserSession table
		SQLite::Statement queryGetUserSession(m_Database, OBFUSCATE("SELECT userID FROM UserSession WHERE userID = ? LIMIT 1"));
		queryGetUserSession.bind(1, userID);
		if (queryGetUserSession.executeStep())
		{
			IUser* user = g_UserManager.GetUserById(userID);
			if (user)
			{
				g_UserManager.DisconnectUser(user);
			}
			else
			{
				DropSession(userID);
			}
		}

		if (restoreData)
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT channelServerID, channelID FROM UserRestore WHERE userID = ? LIMIT 1"));
			query.bind(1, userID);

			if (!query.executeStep())
			{
				return LOGIN_NO_SUCH_USER;
			}

			restoreData->channelServerID = query.getColumn(0);
			restoreData->channelID = query.getColumn(1);

			{
				SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserRestore WHERE userID = ?"));
				query.bind(1, userName);
				query.exec();
			}
		}

		// create user session in db
		SQLite::Statement queryInsertUserSession(m_Database, OBFUSCATE("INSERT INTO UserSession VALUES (?, ?, ?, ?, ?, ?)"));
		queryInsertUserSession.bind(1, userID);
		queryInsertUserSession.bind(2, socket->GetIP());
		queryInsertUserSession.bind(3, ""); // TODO: remove
		queryInsertUserSession.bind(4, socket->GetHWID().data(), (int)socket->GetHWID().size());
		queryInsertUserSession.bind(5, UserStatus::STATUS_MENU);
		queryInsertUserSession.bind(6, 0);
		queryInsertUserSession.exec();

		return userID;
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::Login: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}
}

int CUserDatabaseSQLite::AddToRestoreList(int userID, int channelServerID, int channelID)
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserRestore WHERE userID = ?"));
			query.bind(1, userID);
			query.exec();
		}

		SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserRestore VALUES (?, ?, ?)"));
		query.bind(1, userID);
		query.bind(2, channelServerID);
		query.bind(3, channelID);
		query.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::AddToRestoreList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// creates a new user
// returns 0 == database error, 1 on success, -1 == user with the same username already exists, -4 == ip limit
int CUserDatabaseSQLite::Register(const string& userName, const string& password, const string& ip)
{
	try
	{
		SQLite::Statement queryuser(m_Database, OBFUSCATE("SELECT userID FROM User WHERE userName = ? LIMIT 1"));
		queryuser.bind(1, userName);
		queryuser.executeStep();

		if (queryuser.hasRow())
		{
			queryuser.reset();

			return -1;
		}

		// check registrations per IP limit
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT COUNT(1) FROM User WHERE registerIP = ?"));
			query.bind(1, ip);
			if (query.executeStep() && (int)query.getColumn(0) >= g_pServerConfig->maxRegistrationsPerIP)
			{
				return -4;
			}
		}


		SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO User VALUES ((SELECT userIDNext FROM UserDist), ?, ?, ?, ?, 0, 0, 0, 0)"));
		query.bind(1, userName);
		query.bind(2, password);
		query.bind(3, g_pServerInstance->GetCurrentTime());
		query.bind(4, ip);
		query.exec();

		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserDist SET userIDNext = userIDNext + 1"));
			query.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::Register: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// get info about all sessions
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetUserSessions(std::vector<UserSession>& sessions)
{
	try
	{
		SQLite::Statement query(m_Database, "SELECT UserSession.userID, UserSession.ip, UserSession.sessionTime, User.userName, UserCharacter.gameName "
			"FROM UserSession "
			"INNER JOIN User "
			"ON UserSession.userID = User.userID "
			"INNER JOIN UserCharacter "
			"ON UserSession.userID = UserCharacter.userID");
		while (query.executeStep())
		{
			UserSession session;
			session.userID = query.getColumn(0);
			session.ip = query.getColumn(1).getString();
			session.uptime = query.getColumn(2);
			//session.roomID = query.getColumn(3);
			session.userName = query.getColumn(3).getString();
			session.gameName = query.getColumn(4).getString();
			sessions.push_back(session);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetUserSessions: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// removes user session from database
// returns 0 == database error, 1 on success, -1 == user with such userID is not logged in
int CUserDatabaseSQLite::DropSession(int userID)
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserSessionHistory SELECT UserSession.userID, UserSession.ip, UserSession.hwid, (SELECT lastLogonTime FROM User WHERE userID = UserSession.userID LIMIT 1), UserSession.sessionTime FROM UserSession WHERE UserSession.userID = ?"));
			query.bind(1, userID);
			query.exec();
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserSession WHERE userID = ?"));
			query.bind(1, userID);
			query.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::DropSession: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// removes users session from database
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::DropSessions()
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserSessionHistory SELECT UserSession.userID, UserSession.ip, UserSession.hwid, User.lastLogonTime, UserSession.sessionTime FROM UserSession, User WHERE User.userID = UserSession.userID"));
			query.exec();
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserSession"));
			query.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::DropSessions: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

void CUserDatabaseSQLite::PrintUserList()
{
	try
	{
		static SQLite::Statement query(m_Database, OBFUSCATE("SELECT userID, userName FROM User"));
		cout << OBFUSCATE("Database user list:\n");
		while (query.executeStep())
		{
			cout << OBFUSCATE("UserID: ") << query.getColumn(0) << OBFUSCATE(", Username: ") << query.getColumn(1) << OBFUSCATE("\n");
		}
		query.reset();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::PrintUserList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
	}
}

void CUserDatabaseSQLite::LoadBackup(const string& backupDate)
{
	try
	{
		m_Database.backup(va(OBFUSCATE("UserDatabase_%s.db3"), backupDate.c_str()), SQLite::Database::BackupType::Load);

		CheckForTables();

		Logger().Info(OBFUSCATE("User database backup loaded successfully\n"));
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::LoadBackup: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
	}
}

void CUserDatabaseSQLite::PrintBackupList()
{
#ifdef WIN32
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;

	Logger().Info(OBFUSCATE("SQLite user database backup list:\n"));
	if ((hFind = FindFirstFile(OBFUSCATE("UserDatabase_*.db3"), &FindFileData)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			Logger().Info(OBFUSCATE("%s\n"), FindFileData.cFileName);
		} while (FindNextFile(hFind, &FindFileData));

		FindClose(hFind);
	}
#else
	Logger().Warn(OBFUSCATE("CUserDatabaseSQLite::PrintBackupList: not implemented\n"));
#endif
}

void CUserDatabaseSQLite::ResetQuestEvent(int questID)
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestEventProgress WHERE questID = ?"));
			query.bind(1, questID);
			query.exec();
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestEventTaskProgress WHERE questID = ?"));
			query.bind(1, questID);
			query.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::ResetQuestEvent: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
	}
}

void CUserDatabaseSQLite::WriteUserStatistic(const string& fdate, const string& sdate)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM User"));
		while (query.executeStep())
		{
			int registeredUsers = 0;
			int sessions = 0;
			int averageTimePlayed = 0;
			int longestSessionTime = 0;

			// TODO: ...
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::WriteUserStatistic: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
	}
}

// adds new user inventory item
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::AddInventoryItem(int userID, CUserInventoryItem& item)
{
	try
	{
		if (item.m_nIsClanItem)
		{
			{
				SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM UserInventory WHERE userID = ? AND itemID = ? AND expiryDate = 0 LIMIT 1)"));
				query.bind(1, userID);
				query.bind(2, item.m_nItemID);

				if (query.executeStep())
				{
					if ((int)query.getColumn(0))
						return -1; // user already has permanent item
				}
			}

			{
				SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM UserInventory WHERE userID = ? AND itemID = ? AND isClanItem = 1 LIMIT 1)"));
				query.bind(1, userID);
				query.bind(2, item.m_nItemID);

				if (query.executeStep())
				{
					if ((int)query.getColumn(0))
						return -2; // user already has this clan item
				}
			}
		}

		int slot = -1;

		// find free slot
		SQLite::Statement queryGetFreeInvSlot(m_Database, OBFUSCATE("SELECT slot FROM UserInventory WHERE userID = ? AND itemID = 0 LIMIT 1"));
		queryGetFreeInvSlot.bind(1, userID);

		if (queryGetFreeInvSlot.executeStep())
		{
			slot = queryGetFreeInvSlot.getColumn(0);

			SQLite::Statement queryUpdateItem(m_Database, OBFUSCATE("UPDATE UserInventory SET itemID = ?, count = ?, status = ?, inUse = ?, obtainDate = ?, expiryDate = ?, isClanItem = ?, enhancementLevel = ?, enhancementExp = ?, enhanceValue = ?, paintID = ?, paintIDList = ?, partSlot1 = ?, partSlot2 = ?, lockStatus = ? WHERE userID = ? AND slot = ?"));
			queryUpdateItem.bind(1, item.m_nItemID);
			queryUpdateItem.bind(2, item.m_nCount);
			queryUpdateItem.bind(3, item.m_nStatus);
			queryUpdateItem.bind(4, item.m_nInUse);
			queryUpdateItem.bind(5, item.m_nObtainDate);
			queryUpdateItem.bind(6, item.m_nExpiryDate);
			queryUpdateItem.bind(7, item.m_nIsClanItem);
			queryUpdateItem.bind(8, item.m_nEnhancementLevel);
			queryUpdateItem.bind(9, item.m_nEnhancementExp);
			queryUpdateItem.bind(10, item.m_nEnhanceValue);
			queryUpdateItem.bind(11, item.m_nPaintID);
			queryUpdateItem.bind(12, serialize_array_int(item.m_nPaintIDList));
			queryUpdateItem.bind(13, item.m_nPartSlot1);
			queryUpdateItem.bind(14, item.m_nPartSlot2);
			queryUpdateItem.bind(15, item.m_nLockStatus);
			queryUpdateItem.bind(16, userID);
			queryUpdateItem.bind(17, slot);

			queryUpdateItem.exec();
		}
		else
		{
			SQLite::Statement queryGetNextInvSlot(m_Database, OBFUSCATE("SELECT nextInventorySlot FROM UserCharacterExtended WHERE userID = ? LIMIT 1"));
			queryGetNextInvSlot.bind(1, userID);
			queryGetNextInvSlot.executeStep();

			slot = queryGetNextInvSlot.getColumn(0);

			SQLite::Statement queryInsertNewInvItem(m_Database, OBFUSCATE("INSERT INTO UserInventory VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
			queryInsertNewInvItem.bind(1, userID);
			queryInsertNewInvItem.bind(2, slot);
			queryInsertNewInvItem.bind(3, item.m_nItemID);
			queryInsertNewInvItem.bind(4, item.m_nCount);
			queryInsertNewInvItem.bind(5, item.m_nStatus);
			queryInsertNewInvItem.bind(6, item.m_nInUse);
			queryInsertNewInvItem.bind(7, item.m_nObtainDate);
			queryInsertNewInvItem.bind(8, item.m_nExpiryDate);
			queryInsertNewInvItem.bind(9, item.m_nIsClanItem);
			queryInsertNewInvItem.bind(10, item.m_nEnhancementLevel);
			queryInsertNewInvItem.bind(11, item.m_nEnhancementExp);
			queryInsertNewInvItem.bind(12, item.m_nEnhanceValue);
			queryInsertNewInvItem.bind(13, item.m_nPaintID);
			queryInsertNewInvItem.bind(14, serialize_array_int(item.m_nPaintIDList));
			queryInsertNewInvItem.bind(15, item.m_nPartSlot1);
			queryInsertNewInvItem.bind(16, item.m_nPartSlot2);
			queryInsertNewInvItem.bind(17, item.m_nLockStatus);

			queryInsertNewInvItem.exec();
		}

		item.m_nSlot = slot;

		SQLite::Statement queryUpdateNextInvSlot(m_Database, OBFUSCATE("UPDATE UserCharacterExtended SET nextInventorySlot = nextInventorySlot + 1 WHERE userID = ?"));
		queryUpdateNextInvSlot.bind(1, userID);
		queryUpdateNextInvSlot.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::AddInventoryItem: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// adds new user inventory items
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::AddInventoryItems(int userID, std::vector<CUserInventoryItem>& items)
{
	try
	{
		std::vector<std::vector<CUserInventoryItem>> itemsInsert;
		itemsInsert.resize(ceil((double)items.size() / 1927));

		std::vector<std::vector<CUserInventoryItem>> itemsUpdate;
		itemsUpdate.resize(ceil((double)items.size() / 1092));

		int itemsInsertIndex = 0;
		int itemsUpdateIndex = 0;

		std:vector<int> freeSlots;

		// find free slots
		SQLite::Statement queryGetFreeInvSlot(m_Database, OBFUSCATE("SELECT slot FROM UserInventory WHERE userID = ? AND itemID = 0 LIMIT ?"));
		queryGetFreeInvSlot.bind(1, userID);
		queryGetFreeInvSlot.bind(2, (int)items.size());

		while (queryGetFreeInvSlot.executeStep())
		{
			freeSlots.push_back((int)queryGetFreeInvSlot.getColumn(0));
		}

		SQLite::Statement queryGetNextInvSlot(m_Database, OBFUSCATE("SELECT nextInventorySlot FROM UserCharacterExtended WHERE userID = ? LIMIT 1"));
		queryGetNextInvSlot.bind(1, userID);
		queryGetNextInvSlot.executeStep();

		int slot = (int)queryGetNextInvSlot.getColumn(0);

		for (auto& item : items)
		{
			if (freeSlots.size())
			{
				item.m_nSlot = freeSlots.front();
				freeSlots.erase(freeSlots.begin());

				if (itemsUpdate[itemsUpdateIndex].size() == 1092) // (1092 * 30 binds) + 1 (userID bind) == 32761 < SQLITE_MAX_VARIABLE_NUMBER (32766)
					itemsUpdateIndex++;

				itemsUpdate[itemsUpdateIndex].push_back(item);
			}
			else
			{
				item.m_nSlot = slot++;

				if (itemsInsert[itemsInsertIndex].size() == 1927) // (1927 * 17 binds) == 32759 < SQLITE_MAX_VARIABLE_NUMBER (32766)
					itemsInsertIndex++;

				itemsInsert[itemsInsertIndex].push_back(item);
			}
		}

		int index = 1;

		if (itemsInsert[0].size())
		{
			itemsInsert.resize(itemsInsertIndex + 1);

			std::string queryInsertNewInvItemStr;
			int itemsInsertSize = 0;

			for (int i = 0; i <= itemsInsertIndex; i++)
			{
				itemsInsertSize = (int)itemsInsert[i].size();

				queryInsertNewInvItemStr += OBFUSCATE("INSERT INTO UserInventory VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
				for (int j = 1; j < itemsInsertSize; j++)
					queryInsertNewInvItemStr += OBFUSCATE(", (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

				SQLite::Statement queryInsertNewInvItem(m_Database, queryInsertNewInvItemStr);

				for (auto& item : itemsInsert[i])
				{
					queryInsertNewInvItem.bind(index++, userID);
					queryInsertNewInvItem.bind(index++, item.m_nSlot);
					queryInsertNewInvItem.bind(index++, item.m_nItemID);
					queryInsertNewInvItem.bind(index++, item.m_nCount);
					queryInsertNewInvItem.bind(index++, item.m_nStatus);
					queryInsertNewInvItem.bind(index++, item.m_nInUse);
					queryInsertNewInvItem.bind(index++, item.m_nObtainDate);
					queryInsertNewInvItem.bind(index++, item.m_nExpiryDate);
					queryInsertNewInvItem.bind(index++, item.m_nIsClanItem);
					queryInsertNewInvItem.bind(index++, item.m_nEnhancementLevel);
					queryInsertNewInvItem.bind(index++, item.m_nEnhancementExp);
					queryInsertNewInvItem.bind(index++, item.m_nEnhanceValue);
					queryInsertNewInvItem.bind(index++, item.m_nPaintID);
					queryInsertNewInvItem.bind(index++, serialize_array_int(item.m_nPaintIDList));
					queryInsertNewInvItem.bind(index++, item.m_nPartSlot1);
					queryInsertNewInvItem.bind(index++, item.m_nPartSlot2);
					queryInsertNewInvItem.bind(index++, item.m_nLockStatus);
				}

				queryInsertNewInvItem.exec();
				index = 1;
				queryInsertNewInvItemStr.clear();
			}

			SQLite::Statement queryUpdateNextInvSlot(m_Database, OBFUSCATE("UPDATE UserCharacterExtended SET nextInventorySlot = nextInventorySlot + ? WHERE userID = ?"));
			queryUpdateNextInvSlot.bind(1, (1927 * itemsInsertIndex) + (int)itemsInsert[itemsInsertIndex].size());
			queryUpdateNextInvSlot.bind(2, userID);
			queryUpdateNextInvSlot.exec();
		}

		if (itemsUpdate[0].size())
		{
			itemsUpdate.resize(itemsUpdateIndex + 1);

			std::string queryUpdateItemStr;
			int itemsUpdateSize = 0;

			for (int i = 0; i <= itemsUpdateIndex; i++)
			{
				itemsUpdateSize = (int)itemsUpdate[i].size();

				queryUpdateItemStr += OBFUSCATE("UPDATE UserInventory SET itemID = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE itemID END, count = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE count END, status = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE status END, inUse = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE inUse END, obtainDate = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE obtainDate END, expiryDate = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE expiryDate END, isClanItem = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE isClanItem END, enhancementLevel = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE enhancementLevel END, enhancementExp = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE enhancementExp END, enhanceValue = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE enhanceValue END, paintID = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE paintID END, paintIDList = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE paintIDList END, partSlot1 = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE partSlot1 END, partSlot2 = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE partSlot2 END, lockStatus = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE lockStatus END WHERE userID = ?");

				SQLite::Statement queryUpdateItem(m_Database, queryUpdateItemStr);

				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nItemID);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nCount);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nStatus);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nInUse);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nObtainDate);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nExpiryDate);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nIsClanItem);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nEnhancementLevel);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nEnhancementExp);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nEnhanceValue);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nPaintID);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, serialize_array_int(item.m_nPaintIDList));
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nPartSlot1);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nPartSlot2);
				}
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nLockStatus);
				}

				queryUpdateItem.bind(index++, userID);

				queryUpdateItem.exec();
				index = 1;
				queryUpdateItemStr.clear();
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::AddInventoryItems: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

string UpdateInventoryItemString(int flag)
{
	ostringstream query;
	query << OBFUSCATE("UPDATE UserInventory SET");
	if (flag & UITEM_FLAG_ITEMID)
		query << OBFUSCATE(" itemID = ?,");
	if (flag & UITEM_FLAG_COUNT)
		query << OBFUSCATE(" count = ?,");
	if (flag & UITEM_FLAG_STATUS)
		query << OBFUSCATE(" status = ?,");
	if (flag & UITEM_FLAG_INUSE)
		query << OBFUSCATE(" inUse = ?,");
	if (flag & UITEM_FLAG_OBTAINDATE)
		query << OBFUSCATE(" obtainDate = ?,");
	if (flag & UITEM_FLAG_EXPIRYDATE)
		query << OBFUSCATE(" expiryDate = ?,");
	if (flag & UITEM_FLAG_ISCLANITEM)
		query << OBFUSCATE(" isClanItem = ?,");
	if (flag & UITEM_FLAG_ENHANCEMENTLEVEL)
		query << OBFUSCATE(" enhancementLevel = ?,");
	if (flag & UITEM_FLAG_ENHANCEMENTEXP)
		query << OBFUSCATE(" enhancementExp = ?,");
	if (flag & UITEM_FLAG_ENHANCEVALUE)
		query << OBFUSCATE(" enhanceValue = ?,");
	if (flag & UITEM_FLAG_PAINTID)
		query << OBFUSCATE(" paintID = ?,");
	if (flag & UITEM_FLAG_PAINTIDLIST)
		query << OBFUSCATE(" paintIDList = ?,");
	if (flag & UITEM_FLAG_PARTSLOT1)
		query << OBFUSCATE(" partSlot1 = ?,");
	if (flag & UITEM_FLAG_PARTSLOT2)
		query << OBFUSCATE(" partSlot2 = ?,");
	if (flag & UITEM_FLAG_LOCKSTATUS)
		query << OBFUSCATE(" lockStatus = ?,");

	return query.str();
}

// updates user inventory item data
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateInventoryItem(int userID, const CUserInventoryItem& item, int flag)
{
	try
	{
		// format query
		string query = UpdateInventoryItemString(flag);
		query[query.size() - 1] = ' ';
		query += OBFUSCATE("WHERE userID = ? AND slot = ?"); // TODO: use stringstream

		SQLite::Statement statement(m_Database, query);
		int index = 1;

		if (flag & UITEM_FLAG_ITEMID)
		{
			statement.bind(index++, item.m_nItemID);
		}
		if (flag & UITEM_FLAG_COUNT)
		{
			statement.bind(index++, item.m_nCount);
		}
		if (flag & UITEM_FLAG_STATUS)
		{
			statement.bind(index++, item.m_nStatus);
		}
		if (flag & UITEM_FLAG_INUSE)
		{
			statement.bind(index++, item.m_nInUse);
		}
		if (flag & UITEM_FLAG_OBTAINDATE)
		{
			statement.bind(index++, item.m_nObtainDate);
		}
		if (flag & UITEM_FLAG_EXPIRYDATE)
		{
			statement.bind(index++, item.m_nExpiryDate);
		}
		if (flag & UITEM_FLAG_ISCLANITEM)
		{
			statement.bind(index++, item.m_nIsClanItem);
		}
		if (flag & UITEM_FLAG_ENHANCEMENTLEVEL)
		{
			statement.bind(index++, item.m_nEnhancementLevel);
		}
		if (flag & UITEM_FLAG_ENHANCEMENTEXP)
		{
			statement.bind(index++, item.m_nEnhancementExp);
		}
		if (flag & UITEM_FLAG_ENHANCEVALUE)
		{
			statement.bind(index++, item.m_nEnhanceValue);
		}
		if (flag & UITEM_FLAG_PAINTID)
		{
			statement.bind(index++, item.m_nPaintID);
		}
		if (flag & UITEM_FLAG_PAINTIDLIST)
		{
			statement.bind(index++, serialize_array_int(item.m_nPaintIDList));
		}
		if (flag & UITEM_FLAG_PARTSLOT1)
		{
			statement.bind(index++, item.m_nPartSlot1);
		}
		if (flag & UITEM_FLAG_PARTSLOT2)
		{
			statement.bind(index++, item.m_nPartSlot2);
		}
		if (flag & UITEM_FLAG_LOCKSTATUS)
		{
			statement.bind(index++, item.m_nLockStatus);
		}

		statement.bind(index++, userID);
		statement.bind(index++, item.m_nSlot);

		statement.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateInventoryItem: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user inventory items data
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateInventoryItems(int userID, std::vector<CUserInventoryItem>& items, int flag)
{
	try
	{
		int flagBinds = 0;
		for (int i = 0; i < 15; i++)
		{
			if (flag & (1<<i))
				flagBinds += 2;
		}

		if (!flagBinds)
			return 0;

		int maxSize = (int)floor((double)32765 / flagBinds); // (maxSize * flagBinds) + 1 (userID bind) must be <= SQLITE_MAX_VARIABLE_NUMBER (32766)
		// Example:
		// flagBinds = 2
		// maxSize = floor((double)32765 / 2) = 16382
		// (16382 * 2) + 1 = 32765 < SQLITE_MAX_VARIABLE_NUMBER (32766)

		std::vector<std::vector<CUserInventoryItem>> itemsUpdate;
		itemsUpdate.resize(ceil((double)items.size() / maxSize));

		int itemsUpdateIndex = 0;

		for (auto &item : items)
		{
			if (itemsUpdate[itemsUpdateIndex].size() == maxSize)
				itemsUpdateIndex++;

			itemsUpdate[itemsUpdateIndex].push_back(item);
		}

		std::string queryUpdateItemStr;
		int index = 1;
		int itemsUpdateSize = 0;

		for (int i = 0; i <= itemsUpdateIndex; i++)
		{
			itemsUpdateSize = (int)itemsUpdate[i].size();

			queryUpdateItemStr += OBFUSCATE("UPDATE UserInventory SET");
			if (flag & UITEM_FLAG_ITEMID)
			{
				queryUpdateItemStr += OBFUSCATE(" itemID = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE itemID END,");
			}
			if (flag & UITEM_FLAG_COUNT)
			{
				queryUpdateItemStr += OBFUSCATE(" count = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE count END,");
			}
			if (flag & UITEM_FLAG_STATUS)
			{
				queryUpdateItemStr += OBFUSCATE(" status = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE status END,");
			}
			if (flag & UITEM_FLAG_INUSE)
			{
				queryUpdateItemStr += OBFUSCATE(" inUse = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE inUse END,");
			}
			if (flag & UITEM_FLAG_OBTAINDATE)
			{
				queryUpdateItemStr += OBFUSCATE(" obtainDate = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE obtainDate END,");
			}
			if (flag & UITEM_FLAG_EXPIRYDATE)
			{
				queryUpdateItemStr += OBFUSCATE(" expiryDate = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE expiryDate END,");
			}
			if (flag & UITEM_FLAG_ISCLANITEM)
			{
				queryUpdateItemStr += OBFUSCATE(" isClanItem = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE isClanItem END,");
			}
			if (flag & UITEM_FLAG_ENHANCEMENTLEVEL)
			{
				queryUpdateItemStr += OBFUSCATE(" enhancementLevel = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE enhancementLevel END,");
			}
			if (flag & UITEM_FLAG_ENHANCEMENTEXP)
			{
				queryUpdateItemStr += OBFUSCATE(" enhancementExp = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE enhancementExp END,");
			}
			if (flag & UITEM_FLAG_ENHANCEVALUE)
			{
				queryUpdateItemStr += OBFUSCATE(" enhanceValue = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE enhanceValue END,");
			}
			if (flag & UITEM_FLAG_PAINTID)
			{
				queryUpdateItemStr += OBFUSCATE(" paintID = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE paintID END,");
			}
			if (flag & UITEM_FLAG_PAINTIDLIST)
			{
				queryUpdateItemStr += OBFUSCATE(" paintIDList = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE paintIDList END,");
			}
			if (flag & UITEM_FLAG_PARTSLOT1)
			{
				queryUpdateItemStr += OBFUSCATE(" partSlot1 = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE partSlot1 END,");
			}
			if (flag & UITEM_FLAG_PARTSLOT2)
			{
				queryUpdateItemStr += OBFUSCATE(" partSlot2 = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE partSlot2 END,");
			}
			if (flag & UITEM_FLAG_LOCKSTATUS)
			{
				queryUpdateItemStr += OBFUSCATE(" lockStatus = CASE");
				for (int j = 0; j < itemsUpdateSize; j++)
				{
					queryUpdateItemStr += OBFUSCATE(" WHEN slot = ? THEN ?");
				}
				queryUpdateItemStr += OBFUSCATE(" ELSE lockStatus END,");
			}
			queryUpdateItemStr[queryUpdateItemStr.size() - 1] = ' ';
			queryUpdateItemStr += OBFUSCATE("WHERE userID = ?");

			SQLite::Statement queryUpdateItem(m_Database, queryUpdateItemStr);

			if (flag & UITEM_FLAG_ITEMID)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nItemID);
				}
			}
			if (flag & UITEM_FLAG_COUNT)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nCount);
				}
			}
			if (flag & UITEM_FLAG_STATUS)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nStatus);
				}
			}
			if (flag & UITEM_FLAG_INUSE)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nInUse);
				}
			}
			if (flag & UITEM_FLAG_OBTAINDATE)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nObtainDate);
				}
			}
			if (flag & UITEM_FLAG_EXPIRYDATE)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nExpiryDate);
				}
			}
			if (flag & UITEM_FLAG_ISCLANITEM)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nIsClanItem);
				}
			}
			if (flag & UITEM_FLAG_ENHANCEMENTLEVEL)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nEnhancementLevel);
				}
			}
			if (flag & UITEM_FLAG_ENHANCEMENTEXP)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nEnhancementExp);
				}
			}
			if (flag & UITEM_FLAG_ENHANCEVALUE)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nEnhanceValue);
				}
			}
			if (flag & UITEM_FLAG_PAINTID)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nPaintID);
				}
			}
			if (flag & UITEM_FLAG_PAINTIDLIST)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, serialize_array_int(item.m_nPaintIDList));
				}
			}
			if (flag & UITEM_FLAG_PARTSLOT1)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nPartSlot1);
				}
			}
			if (flag & UITEM_FLAG_PARTSLOT2)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nPartSlot2);
				}
			}
			if (flag & UITEM_FLAG_LOCKSTATUS)
			{
				for (auto& item : itemsUpdate[i])
				{
					queryUpdateItem.bind(index++, item.m_nSlot);
					queryUpdateItem.bind(index++, item.m_nLockStatus);
				}
			}

			queryUpdateItem.bind(index++, userID);

			queryUpdateItem.exec();
			index = 1;
			queryUpdateItemStr.clear();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateInventoryItem: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user inventory
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetInventoryItems(int userID, vector<CUserInventoryItem>& items)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ?"));
		query.bind(1, userID);

		while (query.executeStep())
		{
			CUserInventoryItem item = query.getColumns<CUserInventoryItem, 17>();
			items.push_back(item);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetInventoryItems: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user inventory items by itemID
// returns -1 == database error, 0 == no such item, 1 on success
int CUserDatabaseSQLite::GetInventoryItemsByID(int userID, int itemID, vector<CUserInventoryItem>& items)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND itemID = ?"));
		query.bind(1, userID);
		query.bind(2, itemID);

		while (query.executeStep())
		{
			CUserInventoryItem item = query.getColumns<CUserInventoryItem, 17>();
			items.push_back(item);
		}

		if (items.empty())
			return 0;
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetInventoryItemByID: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return -1;
	}

	return 1;
}

// gets user inventory item by slot
// returns -1 == database error, 0 == no such slot, 1 on success
int CUserDatabaseSQLite::GetInventoryItemBySlot(int userID, int slot, CUserInventoryItem& item)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND slot = ? LIMIT 1"));
		query.bind(1, userID);
		query.bind(2, slot);

		if (!query.executeStep())
		{
			return 0;
		}

		item = query.getColumns<CUserInventoryItem, 17>();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetInventoryItemBySlot: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return -1;
	}

	return 1;
}

// gets first user inventory item by itemID
// returns -1 == database error, 0 == no such item, 1 on success
int CUserDatabaseSQLite::GetFirstItemByItemID(int userID, int itemID, CUserInventoryItem& item)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND itemID = ? LIMIT 1"));
		query.bind(1, userID);
		query.bind(2, itemID);

		if (!query.executeStep())
		{
			return 0;
		}

		item = query.getColumns<CUserInventoryItem, 17>();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetFirstItemByItemID: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return -1;
	}

	return 1;
}

// gets first active user inventory item by itemID
// returns -1 == database error, 0 == no such item, 1 on success
int CUserDatabaseSQLite::GetFirstActiveItemByItemID(int userID, int itemID, CUserInventoryItem& item)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND itemID = ? AND status = 1 AND inUse = 1 LIMIT 1"));
		query.bind(1, userID);
		query.bind(2, itemID);

		if (!query.executeStep())
		{
			return 0;
		}

		item = query.getColumns<CUserInventoryItem, 17>();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetFirstActiveItemByItemID: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return -1;
	}

	return 1;
}

// gets first user extendable inventory item by itemID
// returns -1 == database error, 0 == no such item, 1 on success
int CUserDatabaseSQLite::GetFirstExtendableItemByItemID(int userID, int itemID, CUserInventoryItem& item)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND itemID = ? AND expiryDate != 0 AND enhanceValue = 0 AND partSlot1 = 0 AND partSlot2 = 0 LIMIT 1"));
		query.bind(1, userID);
		query.bind(2, itemID);

		if (!query.executeStep())
		{
			return 0;
		}

		item = query.getColumns<CUserInventoryItem, 17>();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetFirstExtendableItemByItemID: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return -1;
	}

	return 1;
}

// gets user inventory items count
// returns -1 == database error, items count on success
int CUserDatabaseSQLite::GetInventoryItemsCount(int userID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT COUNT(1) FROM UserInventory WHERE userID = ? AND itemID != 0"));
		query.bind(1, userID);
		query.executeStep();

		return (int)query.getColumn(0);
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetInventoryItemsCount: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return -1;
	}
}

// checks if user inventory is full
// returns 0 == inventory is not full, 1 == database error or inventory is full
int CUserDatabaseSQLite::IsInventoryFull(int userID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT COUNT(1) FROM UserInventory WHERE userID = ? AND itemID != 0"));
		query.bind(1, userID);
		query.executeStep();

		if ((int)query.getColumn(0) >= g_pServerConfig->inventorySlotMax)
		{
			return 1;
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsInventoryFull: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 1;
	}

	return 0;
}

string GetUserDataString(int flag, bool update)
{
	ostringstream query;
	query << (update ? OBFUSCATE("UPDATE User SET") : OBFUSCATE("SELECT"));
	if (flag & UDATA_FLAG_USERNAME)
		query << (update ? OBFUSCATE(" userName = ?,") : OBFUSCATE(" userName,"));
	if (flag & UDATA_FLAG_PASSWORD)
		query << (update ? OBFUSCATE(" password = ?,") : OBFUSCATE(" password,"));
	if (flag & UDATA_FLAG_REGISTERTIME)
		query << (update ? OBFUSCATE(" registerTime = ?,") : OBFUSCATE(" registerTime,"));
	if (flag & UDATA_FLAG_REGISTERIP)
		query << (update ? OBFUSCATE(" registerIP = ?,") : OBFUSCATE(" registerIP,"));
	if (flag & UDATA_FLAG_FIRSTLOGONTIME)
		query << (update ? OBFUSCATE(" firstLogonTime = ?,") : OBFUSCATE(" firstLogonTime,"));
	if (flag & UDATA_FLAG_LASTLOGONTIME)
		query << (update ? OBFUSCATE(" lastLogonTime = ?,") : OBFUSCATE(" lastLogonTime,"));
	if (flag & UDATA_FLAG_LASTIP)
		query << (update ? OBFUSCATE(" lastIP = ?,") : OBFUSCATE(" lastIP,"));
	if (flag & UDATA_FLAG_LASTHWID)
		query << (update ? OBFUSCATE(" lastHWID = ?,") : OBFUSCATE(" lastHWID,"));

	return query.str();
}

// gets base user data
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetUserData(int userID, CUserData& data)
{
	try
	{
		// format query
		string query = GetUserDataString(data.flag, false);
		query[query.size() - 1] = ' ';
		query += OBFUSCATE("FROM User WHERE userID = ? LIMIT 1"); // TODO: use stringstream

		SQLite::Statement statement(m_Database, query);
		statement.bind(1, userID);

		if (statement.executeStep())
		{
			int index = 0;
			if (data.flag & UDATA_FLAG_USERNAME)
			{
				string userName = statement.getColumn(index++);
				data.userName = userName;
			}
			if (data.flag & UDATA_FLAG_PASSWORD)
			{
				string password = statement.getColumn(index++);
				data.password = password;
			}
			if (data.flag & UDATA_FLAG_REGISTERTIME)
			{
				data.registerTime = statement.getColumn(index++);
			}
			if (data.flag & UDATA_FLAG_REGISTERIP)
			{
				data.registerIP = statement.getColumn(index++).getString();
			}
			if (data.flag & UDATA_FLAG_FIRSTLOGONTIME)
			{
				data.firstLogonTime = statement.getColumn(index++);
			}
			if (data.flag & UDATA_FLAG_LASTLOGONTIME)
			{
				data.lastLogonTime = statement.getColumn(index++);
			}
			if (data.flag & UDATA_FLAG_LASTIP)
			{
				string lastIP = statement.getColumn(index++);
				data.lastIP = lastIP;
			}
			if (data.flag & UDATA_FLAG_LASTHWID)
			{
				SQLite::Column lastHWID = statement.getColumn(index++);
				data.lastHWID.assign((unsigned char*)lastHWID.getBlob(), (unsigned char*)lastHWID.getBlob() + lastHWID.getBytes());
			}
		}
		else
		{
			return 0;
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetUserData: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates base user data
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateUserData(int userID, CUserData data)
{
	try
	{
		// format query
		string query = GetUserDataString(data.flag, true);
		query[query.size() - 1] = ' ';
		query += OBFUSCATE("WHERE userID = ?");

		SQLite::Statement statement(m_Database, query);
		int index = 1;

		if (data.flag & UDATA_FLAG_USERNAME)
		{
			statement.bind(index++, data.userName);
		}
		if (data.flag & UDATA_FLAG_PASSWORD)
		{
			statement.bind(index++, data.password);
		}
		if (data.flag & UDATA_FLAG_REGISTERTIME)
		{
			statement.bind(index++, data.registerTime);
		}
		if (data.flag & UDATA_FLAG_REGISTERIP)
		{
			statement.bind(index++, data.registerIP);
		}
		if (data.flag & UDATA_FLAG_FIRSTLOGONTIME)
		{
			statement.bind(index++, data.firstLogonTime);
		}
		if (data.flag & UDATA_FLAG_LASTLOGONTIME)
		{
			statement.bind(index++, data.lastLogonTime);
		}
		if (data.flag & UDATA_FLAG_LASTIP)
		{
			statement.bind(index++, data.lastIP);
		}
		if (data.flag & UDATA_FLAG_LASTHWID)
		{
			void* hwid = data.lastHWID.data();
			statement.bind(index++, hwid, (int)data.lastHWID.size());
		}
		statement.bind(index++, userID);
		statement.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateUserData: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// creates user character
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::CreateCharacter(int userID, const string& gameName)
{
	try
	{
		DefaultUser defUser = g_pServerConfig->defUser;

		SQLite::Transaction transcation(m_Database);
		SQLite::Statement insertCharacter(m_Database, OBFUSCATE("INSERT INTO UserCharacter VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
		insertCharacter.bind(1, userID);
		insertCharacter.bind(2, gameName);
		insertCharacter.bind(3, (int64_t)defUser.exp);
		insertCharacter.bind(4, defUser.level);
		insertCharacter.bind(5, (int64_t)defUser.points);
		insertCharacter.bind(6, 0); // cash
		insertCharacter.bind(7, defUser.battles);
		insertCharacter.bind(8, defUser.win);
		insertCharacter.bind(9, defUser.kills);
		insertCharacter.bind(10, defUser.deaths);
		insertCharacter.bind(11, 0); // nation
		insertCharacter.bind(12, 0); // city
		insertCharacter.bind(13, 0); // town
		insertCharacter.bind(14, 0); // league
		insertCharacter.bind(15, defUser.passwordBoxes);
		insertCharacter.bind(16, defUser.mileagePoints);
		insertCharacter.bind(17, defUser.honorPoints);
		insertCharacter.bind(18, defUser.prefixID);
		insertCharacter.bind(19, OBFUSCATE(""));
		insertCharacter.bind(20, OBFUSCATE("0,0,0,0,0"));
		insertCharacter.bind(21, 0); // clan
		insertCharacter.bind(22, 0); // tournament hud
		insertCharacter.bind(23, 0); // nameplateID
		insertCharacter.bind(24, 0); // chatColorID
		insertCharacter.exec();

		SQLite::Statement insertCharacterExtended(m_Database, OBFUSCATE("INSERT INTO UserCharacterExtended VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
		insertCharacterExtended.bind(1, userID);
		insertCharacterExtended.bind(2, defUser.gameMaster);
		insertCharacterExtended.bind(3, 100); // killsToGetGachaponItem
		insertCharacterExtended.bind(4, 1); // nextInventorySlot
		insertCharacterExtended.bind(5, OBFUSCATE("")); // config
		insertCharacterExtended.bind(6, 0); // curLoadout
		insertCharacterExtended.bind(7, 0); // characterID
		insertCharacterExtended.bind(8, 2); // banSettings
		insertCharacterExtended.bind(9, ""); // _2ndPassword
		insertCharacterExtended.bind(10, 0); // securityQuestion
		insertCharacterExtended.bind(11, ""); // securityAnswer
		insertCharacterExtended.bind(12, 0); // zbRespawnEffect
		insertCharacterExtended.bind(13, 0); // killerMarkEffect
		insertCharacterExtended.bind(14, 0); // curGroup
		insertCharacterExtended.exec();

		if ((int)defUser.loadouts.size())
		{
			std::string query;
			query += OBFUSCATE("INSERT INTO UserLoadout VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

			for (int i = 1; i < GROUP_COUNT * (int)defUser.loadouts.size(); i++)
			{
				query += OBFUSCATE(", (?, ?, ?, ?, ?, ?, ?, ?)");
			}

			SQLite::Statement statement(m_Database, query);

			int bindIndex = 1;

			for (int groupID = 0; groupID < GROUP_COUNT; groupID++)
			{
				for (int i = 0; i < (int)defUser.loadouts.size(); i++)
				{
					statement.bind(bindIndex++, userID);
					statement.bind(bindIndex++, groupID);
					statement.bind(bindIndex++, i);
					statement.bind(bindIndex++, defUser.loadouts[i].name);

					for (auto item : defUser.loadouts[i].items)
					{
						statement.bind(bindIndex++, item);
					}
				}
			}

			statement.exec();
		}

		if ((int)defUser.buyMenu.size())
		{
			std::string query;
			query += OBFUSCATE("INSERT INTO UserBuyMenu VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

			for (int i = 1; i < (int)defUser.buyMenu.size(); i++)
			{
				query += OBFUSCATE(", (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
			}

			SQLite::Statement statement(m_Database, query);

			int bindIndex = 1;

			for (int i = 0; i < (int)defUser.buyMenu.size(); i++)
			{
				statement.bind(bindIndex++, userID);
				statement.bind(bindIndex++, i);
				for (auto item : defUser.buyMenu[i].items)
				{
					statement.bind(bindIndex++, item);
				}
			}

			statement.exec();
		}

		{
			std::string query;
			query += OBFUSCATE("INSERT INTO UserFastBuy VALUES (?, ?, ?, ?)");

			for (int i = 1; i < 5; i++)
			{
				query += OBFUSCATE(", (?, ?, ?, ?)");
			}

			SQLite::Statement statement(m_Database, query);

			int bindIndex = 1;

			for (int i = 0; i < FASTBUY_COUNT; i++)
			{
				statement.bind(bindIndex++, userID);
				statement.bind(bindIndex++, i);
				statement.bind(bindIndex++, va(OBFUSCATE("Favorites %d"), i + 1));
				statement.bind(bindIndex++, OBFUSCATE("0,0,0,0,0,0,0,0,0,0,0"));
			}

			statement.exec();
		}

		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserRank VALUES (?, ?, ?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, 71); // No Record tier Original
			statement.bind(3, 71); // No Record tier Zombie
			statement.bind(4, 71); // No Record tier Zombie PVE
			statement.bind(5, 71); // No Record tier Death Match
			statement.exec();
		}

		// init daily rewards
		/*UserDailyRewards dailyReward = {};
		dailyReward.canGetReward = true;
		g_ItemManager.UpdateDailyRewardsRandomItems(dailyReward);
		UpdateDailyRewards(userID, dailyReward);*/

		transcation.commit();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::CreateCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// deletes user character and everything related to userID(except user data)
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::DeleteCharacter(int userID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserCharacter WHERE userID = ?"));
		query.bind(1, userID);
		query.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::DeleteCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

string GetCharacterString(CUserCharacter& character, bool update)
{
	ostringstream query;
	query << (update ? OBFUSCATE("UPDATE UserCharacter SET") : OBFUSCATE("SELECT"));
	if (character.lowFlag & UFLAG_LOW_NAMEPLATE)
		query << (update ? OBFUSCATE(" nameplateID = ?,") : OBFUSCATE(" nameplateID,"));
	if (character.lowFlag & UFLAG_LOW_GAMENAME)
		query << (update ? OBFUSCATE(" gameName = ?,") : OBFUSCATE(" gameName,"));
	if (character.lowFlag & UFLAG_LOW_LEVEL)
		query << (update ? OBFUSCATE(" level = ?,") : OBFUSCATE(" level,"));
	if (character.lowFlag & UFLAG_LOW_EXP)
		query << (update ? OBFUSCATE(" exp = ?,") : OBFUSCATE(" exp,"));
	if (character.lowFlag & UFLAG_LOW_CASH)
		query << (update ? OBFUSCATE(" cash = ?,") : OBFUSCATE(" cash,"));
	if (character.lowFlag & UFLAG_LOW_POINTS)
		query << (update ? OBFUSCATE(" points = ?,") : OBFUSCATE(" points,"));
	if (character.lowFlag & UFLAG_LOW_STAT)
	{
		if (update)
		{
			if (character.statFlag & 0x1)
				query << OBFUSCATE(" battles = ?,");
			if (character.statFlag & 0x2)
				query << OBFUSCATE(" win = ?,");
			if (character.statFlag & 0x4)
				query << OBFUSCATE(" kills = ?,");
			if (character.statFlag & 0x8)
				query << OBFUSCATE(" deaths = ?,");
		}
		else
		{
			query << OBFUSCATE(" battles, win, kills, deaths,");
		}
	}
	if (character.lowFlag & UFLAG_LOW_LOCATION)
		query << (update ? OBFUSCATE(" nation = ?, city = ?, town = ?,") : OBFUSCATE(" nation, city, town,"));
	if (character.lowFlag & UFLAG_LOW_RANK)
		query << (update ? OBFUSCATE(" leagueID = ?,") : OBFUSCATE(" leagueID,"));
	if (character.lowFlag & UFLAG_LOW_PASSWORDBOXES)
		query << (update ? OBFUSCATE(" passwordBoxes = ?,") : OBFUSCATE(" passwordBoxes,"));
	if (character.lowFlag & UFLAG_LOW_ACHIEVEMENT)
	{
		if (update)
		{
			if (character.achievementFlag & 0x1)
				query << OBFUSCATE(" honorPoints = ?,");
			if (character.achievementFlag & 0x2)
				query << OBFUSCATE(" prefixID = ?,");
		}
		else
		{
			query << OBFUSCATE(" honorPoints, prefixID,");
		}
	}
	if (character.lowFlag & UFLAG_LOW_ACHIEVEMENTLIST)
		query << (update ? OBFUSCATE(" achievementList = ?,") : OBFUSCATE(" achievementList,"));
	if (character.lowFlag & UFLAG_LOW_TITLES)
		query << (update ? OBFUSCATE(" titles = ?,") : OBFUSCATE(" titles,"));
	if (character.lowFlag & UFLAG_LOW_CLAN)
		query << (update ? OBFUSCATE(" clanID = ?,") : OBFUSCATE(" clanID, (SELECT markID FROM Clan WHERE clanID = UserCharacter.clanID LIMIT 1), (SELECT name FROM Clan WHERE clanID = UserCharacter.clanID LIMIT 1),"));
	if (character.lowFlag & UFLAG_LOW_TOURNAMENT)
		query << (update ? OBFUSCATE(" tournament = ?,") : OBFUSCATE(" tournament,"));
	if (character.lowFlag & UFLAG_LOW_UNK26)
		query << (update ? OBFUSCATE(" mileagePoints = ?,") : OBFUSCATE(" mileagePoints,"));
	if (character.highFlag & UFLAG_HIGH_CHATCOLOR)
		query << (update ? OBFUSCATE(" chatColorID = ?,") : OBFUSCATE(" chatColorID,"));

	return query.str();
}

// gets user character
// returns -1 == character doesn't exist, 0 == database error, 1 on success
int CUserDatabaseSQLite::GetCharacter(int userID, CUserCharacter& character)
{
	try
	{
		// format query
		string query = GetCharacterString(character, false);
		query[query.size() - 1] = ' ';
		query += OBFUSCATE("FROM UserCharacter WHERE userID = ? LIMIT 1"); // TODO: use stringstream

		SQLite::Statement statement(m_Database, query);
		statement.bind(1, userID);

		if (statement.executeStep())
		{
			int index = 0;
			if (character.lowFlag & UFLAG_LOW_NAMEPLATE)
			{
				character.nameplateID = statement.getColumn(index++);
			}
			if (character.lowFlag & UFLAG_LOW_GAMENAME)
			{
				string gameName = statement.getColumn(index++);
				character.gameName = gameName;
			}
			if (character.lowFlag & UFLAG_LOW_LEVEL)
			{
				character.level = statement.getColumn(index++);
			}
			if (character.lowFlag & UFLAG_LOW_EXP)
			{
				character.exp = statement.getColumn(index++); // no uint64 support ;(
			}
			if (character.lowFlag & UFLAG_LOW_CASH)
			{
				character.cash = statement.getColumn(index++);
			}
			if (character.lowFlag & UFLAG_LOW_POINTS)
			{
				character.points = statement.getColumn(index++);
			}
			if (character.lowFlag & UFLAG_LOW_STAT)
			{
				character.battles = statement.getColumn(index++);
				character.win = statement.getColumn(index++);
				character.kills = statement.getColumn(index++);
				character.deaths = statement.getColumn(index++);
			}
			if (character.lowFlag & UFLAG_LOW_LOCATION)
			{
				character.nation = statement.getColumn(index++);
				character.city = statement.getColumn(index++);
				character.town = statement.getColumn(index++);
			}
			if (character.lowFlag & UFLAG_LOW_RANK)
			{
				character.leagueID = statement.getColumn(index++);

				// TODO: rewrite
				{
					SQLite::Statement query(m_Database, OBFUSCATE("SELECT tierOri, tierZM, tierZPVE, tierDM FROM UserRank WHERE userID = ? LIMIT 1"));
					query.bind(1, userID);
					if (query.executeStep())
					{
						for (int i = 0; i < 4; i++)
						{
							character.tier[i] = query.getColumn(i);
						}
					}
				}
			}
			if (character.lowFlag & UFLAG_LOW_PASSWORDBOXES)
			{
				character.passwordBoxes = statement.getColumn(index++);
			}
			if (character.lowFlag & UFLAG_LOW_ACHIEVEMENT)
			{
				character.honorPoints = statement.getColumn(index++);
				character.prefixId = statement.getColumn(index++);
			}
			if (character.lowFlag & UFLAG_LOW_ACHIEVEMENTLIST)
			{
				character.achievementList = deserialize_array_int(statement.getColumn(index++));
			}
			if (character.lowFlag & UFLAG_LOW_TITLES)
			{
				string serialized = statement.getColumn(index++);
				character.titles = deserialize_array_int(serialized);
			}
			if (character.lowFlag & UFLAG_LOW_CLAN)
			{
				character.clanID = statement.getColumn(index++);
				character.clanMarkID = statement.getColumn(index++);
				character.clanName = (const char*)statement.getColumn(index++);
			}
			if (character.lowFlag & UFLAG_LOW_TOURNAMENT)
			{
				character.tournament = statement.getColumn(index++);
			}
			if (character.lowFlag & UFLAG_LOW_UNK26)
			{
				character.mileagePoints = statement.getColumn(index++);
			}
			if (character.highFlag & UFLAG_HIGH_CHATCOLOR)
			{
				character.chatColorID = statement.getColumn(index++);
			}
		}
		else
		{
			return -1;
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user character
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateCharacter(int userID, CUserCharacter& character)
{
	try
	{
		// format query
		string query = GetCharacterString(character, true);
		query[query.size() - 1] = ' ';
		query += OBFUSCATE("WHERE userID = ?"); // TODO: use stringstream

		SQLite::Statement statement(m_Database, query);
		int index = 1;

		if (character.lowFlag & UFLAG_LOW_NAMEPLATE)
		{
			statement.bind(index++, character.nameplateID);
		}
		if (character.lowFlag & UFLAG_LOW_GAMENAME)
		{
			statement.bind(index++, character.gameName);
		}
		if (character.lowFlag & UFLAG_LOW_LEVEL)
		{
			statement.bind(index++, character.level);
		}
		if (character.lowFlag & UFLAG_LOW_EXP)
		{
			statement.bind(index++, (int64_t)character.exp);
		}
		if (character.lowFlag & UFLAG_LOW_CASH)
		{
			statement.bind(index++, character.cash);
		}
		if (character.lowFlag & UFLAG_LOW_POINTS)
		{
			statement.bind(index++, character.points);
		}
		if (character.lowFlag & UFLAG_LOW_STAT)
		{
			if (character.statFlag & 0x1)
			{
				statement.bind(index++, character.battles);
			}
			if (character.statFlag & 0x2)
			{
				statement.bind(index++, character.win);
			}
			if (character.statFlag & 0x4)
			{
				statement.bind(index++, character.kills);
			}
			if (character.statFlag & 0x8)
			{
				statement.bind(index++, character.deaths);
			}
		}
		if (character.lowFlag & UFLAG_LOW_LOCATION)
		{
			statement.bind(index++, character.nation);
			statement.bind(index++, character.city);
			statement.bind(index++, character.town);
		}
		if (character.lowFlag & UFLAG_LOW_RANK)
		{
			statement.bind(index++, character.leagueID);
		}
		if (character.lowFlag & UFLAG_LOW_PASSWORDBOXES)
		{
			statement.bind(index++, character.passwordBoxes);
		}
		if (character.lowFlag & UFLAG_LOW_ACHIEVEMENT)
		{
			if (character.achievementFlag & 0x1)
			{
				statement.bind(index++, character.honorPoints);
			}
			if (character.achievementFlag & 0x2)
			{
				statement.bind(index++, character.prefixId);
			}
		}
		if (character.lowFlag & UFLAG_LOW_ACHIEVEMENTLIST)
		{
			string serialized = serialize_array_int(character.achievementList);
			statement.bind(index++, serialized);
		}
		if (character.lowFlag & UFLAG_LOW_TITLES)
		{
			string serialized = serialize_array_int(character.titles);
			statement.bind(index++, serialized);
		}
		if (character.lowFlag & UFLAG_LOW_CLAN)
		{
			statement.bind(index++, character.clanID);
		}
		if (character.lowFlag & UFLAG_LOW_TOURNAMENT)
		{
			statement.bind(index++, character.tournament);
		}
		if (character.lowFlag & UFLAG_LOW_UNK26)
		{
			statement.bind(index++, character.mileagePoints);
		}
		if (character.highFlag & UFLAG_HIGH_CHATCOLOR)
		{
			statement.bind(index++, character.chatColorID);
		}

		statement.bind(index++, userID);

		statement.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

string GetCharacterExtendedString(int flag, bool update)
{
	ostringstream query;
	query << (update ? OBFUSCATE("UPDATE UserCharacterExtended SET") : OBFUSCATE("SELECT"));
	if (flag & EXT_UFLAG_GAMEMASTER)
		query << (update ? OBFUSCATE(" gameMaster = ?,") : OBFUSCATE(" gameMaster,"));
	if (flag & EXT_UFLAG_KILLSTOGETGACHAPONITEM)
		query << (update ? OBFUSCATE(" killsToGetGachaponItem = ?,") : OBFUSCATE(" killsToGetGachaponItem,"));
	if (flag & EXT_UFLAG_NEXTINVENTORYSLOT)
		query << (update ? OBFUSCATE(" nextInventorySlot = ?,") : OBFUSCATE(" nextInventorySlot,"));
	if (flag & EXT_UFLAG_CONFIG)
		query << (update ? OBFUSCATE(" config = ?,") : OBFUSCATE(" config,"));
	if (flag & EXT_UFLAG_CURLOADOUT)
		query << (update ? OBFUSCATE(" curLoadout = ?,") : OBFUSCATE(" curLoadout,"));
	if (flag & EXT_UFLAG_CHARACTERID)
		query << (update ? OBFUSCATE(" characterID = ?,") : OBFUSCATE(" characterID,"));
	if (flag & EXT_UFLAG_BANSETTINGS)
		query << (update ? OBFUSCATE(" banSettings = ?,") : OBFUSCATE(" banSettings,"));
	if (flag & EXT_UFLAG_2NDPASSWORD)
		query << (update ? OBFUSCATE(" _2ndPassword = ?,") : OBFUSCATE(" _2ndPassword,"));
	if (flag & EXT_UFLAG_SECURITYQNA)
		query << (update ? OBFUSCATE(" securityQuestion = ?, securityAnswer = ?,") : OBFUSCATE(" securityQuestion, securityAnswer,"));
	if (flag & EXT_UFLAG_ZBRESPAWNEFFECT)
		query << (update ? OBFUSCATE(" zbRespawnEffect = ?,") : OBFUSCATE(" zbRespawnEffect,"));
	if (flag & EXT_UFLAG_KILLERMARKEFFECT)
		query << (update ? OBFUSCATE(" killerMarkEffect = ?,") : OBFUSCATE(" killerMarkEffect,"));
	if (flag & EXT_UFLAG_CURGROUP)
		query << (update ? OBFUSCATE(" curGroup = ?,") : OBFUSCATE(" curGroup,"));

	return query.str();
}

// gets user character extended
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetCharacterExtended(int userID, CUserCharacterExtended& character)
{
	try
	{
		// format query
		string query = GetCharacterExtendedString(character.flag, false);
		query[query.size() - 1] = ' ';
		query += OBFUSCATE("FROM UserCharacterExtended WHERE userID = ? LIMIT 1"); // TODO: use stringstream

		SQLite::Statement statement(m_Database, query);
		statement.bind(1, userID);

		if (statement.executeStep())
		{
			int index = 0;
			if (character.flag & EXT_UFLAG_GAMEMASTER)
			{
				character.gameMaster = (int)statement.getColumn(index++);
			}
			if (character.flag & EXT_UFLAG_KILLSTOGETGACHAPONITEM)
			{
				character.killsToGetGachaponItem = statement.getColumn(index++);
			}
			if (character.flag & EXT_UFLAG_NEXTINVENTORYSLOT)
			{
				character.nextInventorySlot = statement.getColumn(index++);
			}
			if (character.flag & EXT_UFLAG_CONFIG)
			{
				SQLite::Column config = statement.getColumn(index++);
				character.config.assign((unsigned char*)config.getBlob(), (unsigned char*)config.getBlob() + config.getBytes());
			}
			if (character.flag & EXT_UFLAG_CURLOADOUT)
			{
				character.curLoadout = statement.getColumn(index++);
			}
			if (character.flag & EXT_UFLAG_CHARACTERID)
			{
				character.characterID = statement.getColumn(index++);
			}
			if (character.flag & EXT_UFLAG_BANSETTINGS)
			{
				character.banSettings = statement.getColumn(index++);
			}
			if (character.flag & EXT_UFLAG_2NDPASSWORD)
			{
				SQLite::Column _2ndPassword = statement.getColumn(index++);
				character._2ndPassword.assign((unsigned char*)_2ndPassword.getBlob(), (unsigned char*)_2ndPassword.getBlob() + _2ndPassword.getBytes());
			}
			if (character.flag & EXT_UFLAG_SECURITYQNA)
			{
				character.securityQuestion = statement.getColumn(index++);
				SQLite::Column securityAnswer = statement.getColumn(index++);
				character.securityAnswer.assign((unsigned char*)securityAnswer.getBlob(), (unsigned char*)securityAnswer.getBlob() + securityAnswer.getBytes());
			}
			if (character.flag & EXT_UFLAG_ZBRESPAWNEFFECT)
			{
				character.zbRespawnEffect = statement.getColumn(index++);
			}
			if (character.flag & EXT_UFLAG_KILLERMARKEFFECT)
			{
				character.killerMarkEffect = statement.getColumn(index++);
			}
			if (character.flag & EXT_UFLAG_CURGROUP)
			{
				character.curGroup = statement.getColumn(index++);
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetCharacterExtended: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user character extended
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateCharacterExtended(int userID, CUserCharacterExtended& character)
{
	try
	{
		// format query
		string query = GetCharacterExtendedString(character.flag, true);
		query[query.size() - 1] = ' ';
		query += OBFUSCATE("WHERE userID = ?"); // TODO: use stringstream

		SQLite::Statement statement(m_Database, query);
		int index = 1;

		if (character.flag & EXT_UFLAG_GAMEMASTER)
		{
			statement.bind(index++, character.gameMaster);
		}
		if (character.flag & EXT_UFLAG_KILLSTOGETGACHAPONITEM)
		{
			statement.bind(index++, character.killsToGetGachaponItem);
		}
		if (character.flag & EXT_UFLAG_NEXTINVENTORYSLOT)
		{
			statement.bind(index++, character.nextInventorySlot);
		}
		if (character.flag & EXT_UFLAG_CONFIG)
		{
			void* config = character.config.data();
			statement.bind(index++, config, (int)character.config.size());
		}
		if (character.flag & EXT_UFLAG_CURLOADOUT)
		{
			statement.bind(index++, character.curLoadout);
		}
		if (character.flag & EXT_UFLAG_CHARACTERID)
		{
			statement.bind(index++, character.characterID);
		}
		if (character.flag & EXT_UFLAG_BANSETTINGS)
		{
			statement.bind(index++, character.banSettings);
		}
		if (character.flag & EXT_UFLAG_2NDPASSWORD)
		{
			void* _2ndPassword = character._2ndPassword.data();
			statement.bind(index++, _2ndPassword, (int)character._2ndPassword.size());
		}
		if (character.flag & EXT_UFLAG_SECURITYQNA)
		{
			statement.bind(index++, character.securityQuestion);
			void* securityAnswer = character.securityAnswer.data();
			statement.bind(index++, securityAnswer, (int)character.securityAnswer.size());
		}
		if (character.flag & EXT_UFLAG_ZBRESPAWNEFFECT)
		{
			statement.bind(index++, character.zbRespawnEffect);
		}
		if (character.flag & EXT_UFLAG_KILLERMARKEFFECT)
		{
			statement.bind(index++, character.killerMarkEffect);
		}
		if (character.flag & EXT_UFLAG_CURGROUP)
		{
			statement.bind(index++, character.curGroup);
		}

		statement.bind(index++, userID);

		statement.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateCharacterExtended: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user ban info
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetUserBan(int userID, UserBan& ban)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT type, reason, term FROM UserBan WHERE userID = ? LIMIT 1"));
		statement.bind(1, userID);

		if (statement.executeStep())
		{
			ban.banType = statement.getColumn(0);
			string reason = statement.getColumn(1);
			ban.reason = reason;
			ban.term = statement.getColumn(2);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetUserBan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user ban info
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateUserBan(int userID, UserBan ban)
{
	try
	{
		if (ban.banType == 0)
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("DELETE FROM UserBan WHERE userID = ?"));
			statement.bind(1, userID);
			statement.exec();

			return 1;
		}

		SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE UserBan SET type = ?, reason = ?, term = ? WHERE userID = ?"));
		statement.bind(1, ban.banType);
		statement.bind(2, ban.reason);
		statement.bind(3, ban.term);
		statement.bind(4, userID);
		if (!statement.exec())
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserBan VALUES (?, ?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, ban.banType);
			statement.bind(3, ban.reason);
			statement.bind(4, ban.term);
			if (!statement.exec())
			{
				//Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateUserBan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
				return 0;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateUserBan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user loadout
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetLoadouts(int userID, vector<vector<CUserLoadout>>& loadouts)
{
	try
	{
		for (int groupID = 0; groupID < GROUP_COUNT; groupID++)
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("SELECT slot0, slot1, slot2, slot3, name FROM UserLoadout WHERE userID = ? AND groupID = ? LIMIT ?"));
			statement.bind(1, userID);
			statement.bind(2, groupID);
			statement.bind(3, LOADOUT_COUNT);

			vector<CUserLoadout> lds;

			while (statement.executeStep())
			{
				vector<int> ld;
				for (int i = 0; i < LOADOUT_SLOT_COUNT; i++)
				{
					ld.push_back(statement.getColumn(i));
				}

				lds.push_back(CUserLoadout(statement.getColumn(LOADOUT_SLOT_COUNT), ld));
			}

			loadouts.push_back(lds);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetLoadout: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user loadout
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateLoadout(int userID, int groupID, int loadoutID, int slot, int itemID)
{
	try
	{
		string query;
		query = OBFUSCATE("UPDATE UserLoadout SET ");
		query += OBFUSCATE("slot") + to_string(slot) + OBFUSCATE(" = ? ");
		query += OBFUSCATE("WHERE userID = ? AND groupID = ? AND loadoutID = ?");

		SQLite::Statement statement(m_Database, query);
		statement.bind(1, itemID);
		statement.bind(2, userID);
		statement.bind(3, groupID);
		statement.bind(4, loadoutID);
		if (!statement.exec())
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserLoadout VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, groupID);
			statement.bind(3, loadoutID);

			char name[8];
			sprintf(name, OBFUSCATE("Set #%d"), loadoutID + 1);
			statement.bind(4, name);

			statement.bind(5, slot == 0 ? itemID : 24);
			statement.bind(6, slot == 1 ? itemID : 6);
			statement.bind(7, slot == 2 ? itemID : 161);
			statement.bind(8, slot == 3 ? itemID : 31);
			statement.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateLoadout: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user loadout name
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateLoadoutName(int userID, int groupID, int loadoutID, string name)
{
	try
	{
		string query;
		query = OBFUSCATE("UPDATE UserLoadout SET name = ? WHERE userID = ? AND groupID = ? AND loadoutID = ?");

		SQLite::Statement statement(m_Database, query);
		statement.bind(1, name);
		statement.bind(2, userID);
		statement.bind(3, groupID);
		statement.bind(4, loadoutID);
		if (!statement.exec())
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserLoadout VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, groupID);
			statement.bind(3, loadoutID);
			statement.bind(4, name);
			statement.bind(5, 24);
			statement.bind(6, 6);
			statement.bind(7, 161);
			statement.bind(8, 31);
			statement.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateLoadoutName: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user fast buy
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetFastBuy(int userID, vector<CUserFastBuy>& fastBuy)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT name, items FROM UserFastBuy WHERE userID = ? LIMIT ?"));
		statement.bind(1, userID);
		statement.bind(2, FASTBUY_COUNT);

		while (statement.executeStep())
		{
			fastBuy.push_back(CUserFastBuy(statement.getColumn(0), deserialize_array_int(statement.getColumn(1))));
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetFastBuy: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user fast buy
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateFastBuy(int userID, int slot, const string& name, const vector<int>& items)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE UserFastBuy SET name = ?, items = ? WHERE userID = ? AND fastBuyID = ?"));
		statement.bind(1, name);
		statement.bind(2, serialize_array_int(items));
		statement.bind(3, userID);
		statement.bind(4, slot);
		if (!statement.exec())
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserFastBuy VALUES (?, ?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, slot);
			statement.bind(3, name);
			statement.bind(4, serialize_array_int(items));
			statement.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateFastBuy: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user buy menu
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetBuyMenu(int userID, vector<CUserBuyMenu>& buyMenu)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT slot1, slot2, slot3, slot4, slot5, slot6, slot7, slot8, slot9 FROM UserBuyMenu WHERE userID = ? LIMIT ?"));
		statement.bind(1, userID);
		statement.bind(2, BUYMENU_COUNT);

		while (statement.executeStep())
		{
			vector<int> bm;
			for (int i = 0; i < BUYMENU_SLOT_COUNT; i++)
			{
				bm.push_back(statement.getColumn(i));
			}

			buyMenu.push_back(CUserBuyMenu(bm));
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetBuyMenu: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user buy menu
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateBuyMenu(int userID, int subMenuID, int subMenuSlot, int itemID)
{
	try
	{
		string query;
		query = OBFUSCATE("UPDATE UserBuyMenu SET ");
		query += OBFUSCATE("slot") + to_string(subMenuSlot + 1) + OBFUSCATE(" = ? ");
		query += OBFUSCATE("WHERE userID = ? AND buyMenuID = ?");

		SQLite::Statement statement(m_Database, query);
		statement.bind(1, itemID);
		statement.bind(2, userID);
		statement.bind(3, subMenuID);
		if (!statement.exec())
		{
			// insert?
			Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBuyMenu: UserBuyMenu is empty(userID: %d)???\n"), userID);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBuyMenu: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetBookmark(int userID, vector<vector<int>>& bookmark)
{
	try
	{
		for (int groupID = 0; groupID < GROUP_COUNT; groupID++)
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT itemID FROM UserBookmark WHERE userID = ? AND groupID = ? LIMIT ?"));
			query.bind(1, userID);
			query.bind(2, groupID);
			query.bind(3, BOOKMARK_COUNT);

			vector<int> bk;

			while (query.executeStep())
			{
				bk.push_back(query.getColumn(0));
			}

			bk.resize(BOOKMARK_COUNT);
			bookmark.push_back(bk);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetBookmark: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateBookmark(int userID, int groupID, int bookmarkID, int itemID)
{
	try
	{
		SQLite::Statement query(m_Database, "UPDATE UserBookmark SET itemID = ? WHERE userID = ? AND groupID = ? AND bookmarkID = ?");
		query.bind(1, itemID);
		query.bind(2, userID);
		query.bind(3, groupID);
		query.bind(4, bookmarkID);
		if (!query.exec())
		{
			SQLite::Statement query(m_Database, "INSERT INTO UserBookmark VALUES (?, ?, ?, ?)");
			query.bind(1, userID);
			query.bind(2, groupID);
			query.bind(3, bookmarkID);
			query.bind(4, itemID);
			query.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBookmark: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user costume loadout
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetCostumeLoadout(int userID, CUserCostumeLoadout& loadout)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT head, back, arm, pelvis, face, tattoo, pet FROM UserCostumeLoadout WHERE userID = ? LIMIT 1"));
		query.bind(1, userID);
		if (query.executeStep())
		{
			loadout.m_nHeadCostumeID = query.getColumn(0);
			loadout.m_nBackCostumeID = query.getColumn(1);
			loadout.m_nArmCostumeID = query.getColumn(2);
			loadout.m_nPelvisCostumeID = query.getColumn(3);
			loadout.m_nFaceCostumeID = query.getColumn(4);
			loadout.m_nTattooID = query.getColumn(5);
			loadout.m_nPetCostumeID = query.getColumn(6);
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT slot, itemID FROM UserZBCostumeLoadout WHERE userID = ? LIMIT ?"));
			query.bind(1, userID);
			query.bind(2, ZB_COSTUME_SLOT_COUNT_MAX);
			while (query.executeStep())
			{
				int slot = query.getColumn(0);
				int itemID = query.getColumn(1);

				loadout.m_ZombieSkinCostumeID[slot] = itemID;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetCostumeLoadout: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user costume loadout
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateCostumeLoadout(int userID, CUserCostumeLoadout& loadout, int zbSlot)
{
	try
	{
		// update zombie loadout
		if (zbSlot == -1)
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserCostumeLoadout SET head = ?, back = ?, arm = ?, pelvis = ?, face = ?, tattoo = ?, pet = ? WHERE userID = ?"));
			query.bind(1, loadout.m_nHeadCostumeID);
			query.bind(2, loadout.m_nBackCostumeID);
			query.bind(3, loadout.m_nArmCostumeID);
			query.bind(4, loadout.m_nPelvisCostumeID);
			query.bind(5, loadout.m_nFaceCostumeID);
			query.bind(6, loadout.m_nTattooID);
			query.bind(7, loadout.m_nPetCostumeID);
			query.bind(8, userID);
			if (!query.exec())
			{
				SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserCostumeLoadout VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
				query.bind(1, userID);
				query.bind(2, loadout.m_nHeadCostumeID);
				query.bind(3, loadout.m_nBackCostumeID);
				query.bind(4, loadout.m_nArmCostumeID);
				query.bind(5, loadout.m_nPelvisCostumeID);
				query.bind(6, loadout.m_nFaceCostumeID);
				query.bind(7, loadout.m_nTattooID);
				query.bind(8, loadout.m_nPetCostumeID);

				query.exec();
			}
		}
		else
		{
			if (loadout.m_ZombieSkinCostumeID.count(zbSlot) && loadout.m_ZombieSkinCostumeID[zbSlot] == 0)
			{
				loadout.m_ZombieSkinCostumeID.erase(zbSlot);

				SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserZBCostumeLoadout WHERE userID = ? AND slot = ?"));
				query.bind(1, userID);
				query.bind(2, zbSlot);
				query.exec();
			}
			else
			{
				SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserZBCostumeLoadout SET itemID = ? WHERE slot = ? AND userID = ?"));
				query.bind(1, loadout.m_ZombieSkinCostumeID[zbSlot]);
				query.bind(2, zbSlot);
				query.bind(3, userID);
				if (!query.exec())
				{
					SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserZBCostumeLoadout VALUES (?, ?, ?)"));
					query.bind(1, userID);
					query.bind(2, zbSlot);
					query.bind(3, loadout.m_ZombieSkinCostumeID[zbSlot]);
					query.exec();
				}
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateCostumeLoadout: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}


// gets user reward notices
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetRewardNotices(int userID, vector<int>& notices)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT rewardID FROM UserRewardNotice WHERE userID = ?"));
		statement.bind(1, userID);

		while (statement.executeStep())
		{
			notices.push_back(statement.getColumn(0));
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetRewardNotices: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user reward notices
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateRewardNotices(int userID, int rewardID)
{
	try
	{
		if (!rewardID)
		{
			// delete all records
			SQLite::Statement statement(m_Database, OBFUSCATE("DELETE FROM UserRewardNotice WHERE userID = ?"));
			statement.bind(1, userID);
			statement.exec();

			return 1;
		}

		SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserRewardNotice VALUES (?, ?)"));
		statement.bind(1, userID);
		statement.bind(2, rewardID);
		statement.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateRewardNotices: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetExpiryNotices(int userID, vector<int>& notices)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT itemID FROM UserExpiryNotice WHERE userID = ?"));
		statement.bind(1, userID);

		while (statement.executeStep())
		{
			notices.push_back(statement.getColumn(0));
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetExpiryNotices: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateExpiryNotices(int userID, int itemID)
{
	try
	{
		if (!itemID)
		{
			// delete all records
			SQLite::Statement statement(m_Database, OBFUSCATE("DELETE FROM UserExpiryNotice WHERE userID = ?"));
			statement.bind(1, userID);
			statement.exec();

			return 1;
		}

		SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserExpiryNotice VALUES (?, ?)"));
		statement.bind(1, userID);
		statement.bind(2, itemID);
		statement.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateRewardNotices: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetDailyRewards(int userID, UserDailyRewards& dailyRewards)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT day, canGetReward FROM UserDailyReward WHERE userID = ? LIMIT 1"));
		statement.bind(1, userID);

		if (statement.executeStep())
		{
			dailyRewards.day = statement.getColumn(0);
			dailyRewards.canGetReward = (char)statement.getColumn(1);
		}

		SQLite::Statement statement_getItems(m_Database, OBFUSCATE("SELECT itemID, count, duration, eventFlag FROM UserDailyRewardItems WHERE userID = ?"));
		statement_getItems.bind(1, userID);

		while (statement_getItems.executeStep())
		{
			RewardItem item;
			item.itemID = statement_getItems.getColumn(0);
			item.count = statement_getItems.getColumn(1);
			item.duration = statement_getItems.getColumn(2);
			item.eventFlag = statement_getItems.getColumn(3);

			dailyRewards.randomItems.push_back(item);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetDailyRewards: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateDailyRewards(int userID, UserDailyRewards& dailyRewards)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE UserDailyReward SET day = ?, canGetReward = ? WHERE userID = ?"));
		statement.bind(1, dailyRewards.day);
		statement.bind(2, dailyRewards.canGetReward);
		statement.bind(3, userID);
		if (!statement.exec())
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserDailyReward VALUES (?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, dailyRewards.day);
			statement.bind(3, dailyRewards.canGetReward);
			statement.exec();
		}

		SQLite::Statement statement_updateItems(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM UserDailyRewardItems WHERE userID = ? LIMIT 1)"));
		statement_updateItems.bind(1, userID);
		
		if (statement_updateItems.executeStep())
		{
			if ((int)statement_updateItems.getColumn(0))
			{
				SQLite::Statement statement_deleteItems(m_Database, OBFUSCATE("DELETE FROM UserDailyRewardItems WHERE userID = ?"));
				statement_deleteItems.bind(1, userID);
				statement_deleteItems.exec();
			}
			else
			{
				SQLite::Statement statement_addItems(m_Database, OBFUSCATE("INSERT INTO UserDailyRewardItems VALUES (?, ?, ?, ?, ?)"));
				for (RewardItem& item : dailyRewards.randomItems)
				{
					statement_addItems.bind(1, userID);
					statement_addItems.bind(2, item.itemID);
					statement_addItems.bind(3, item.count);
					statement_addItems.bind(4, item.duration);
					statement_addItems.bind(5, item.eventFlag);
					statement_addItems.exec();
					statement_addItems.reset();
				}
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateDailyRewards: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetQuestsProgress(int userID, vector<UserQuestProgress>& questsProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT questID, status, favourite, started FROM UserQuestProgress WHERE userID = ?"));
		statement.bind(1, userID);
		while (statement.executeStep())
		{
			UserQuestProgress progress;
			progress.questID = statement.getColumn(0);
			progress.status = statement.getColumn(1);
			progress.favourite = (char)statement.getColumn(2);
			progress.started = (char)statement.getColumn(3);

			questsProgress.push_back(progress);
		}

		{
			SQLite::Statement statement(m_Database, OBFUSCATE("SELECT questID, taskID, unitsDone, taskVar FROM UserQuestTaskProgress WHERE userID = ?"));
			statement.bind(1, userID);
			while (statement.executeStep())
			{
				UserQuestTaskProgress progress;
				int questID = statement.getColumn(0);
				progress.taskID = statement.getColumn(1);
				progress.unitsDone = statement.getColumn(2);
				progress.taskVar = statement.getColumn(3);

				vector<UserQuestProgress>::iterator userProgressIt = find_if(questsProgress.begin(), questsProgress.end(),
					[questID](UserQuestProgress& userProgress) { return userProgress.questID == questID; });

				if (userProgressIt != questsProgress.end())
				{
					UserQuestProgress& userProgress = *userProgressIt;
					userProgress.tasks.push_back(progress);
				}
				else
				{
					// TODO: fix
					UserQuestProgress userProgress = {};
					userProgress.questID = questID;
					userProgress.tasks.push_back(progress);
					questsProgress.push_back(userProgress);
				}
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetQuestProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetQuestProgress(int userID, int questID, UserQuestProgress& questProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT status, favourite, started FROM UserQuestProgress WHERE userID = ? AND questID = ? LIMIT 1"));
		statement.bind(1, userID);
		statement.bind(2, questID);
		if (statement.executeStep())
		{
			questProgress.status = statement.getColumn(0);
			questProgress.favourite = (char)statement.getColumn(1);
			questProgress.started = (char)statement.getColumn(2);
		}
		
		questProgress.questID = questID;
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetQuestProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateQuestProgress(int userID, UserQuestProgress& questProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE UserQuestProgress SET status = ?, favourite = ?, started = ? WHERE userID = ? AND questID = ?"));
		statement.bind(1, questProgress.status);
		statement.bind(2, questProgress.favourite);
		statement.bind(3, questProgress.started);
		statement.bind(4, userID);
		statement.bind(5, questProgress.questID);
		if (!statement.exec())
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserQuestProgress VALUES (?, ?, ?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, questProgress.questID);
			statement.bind(3, questProgress.status);
			statement.bind(4, questProgress.favourite);
			statement.bind(5, questProgress.started);
			statement.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateQuestProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetQuestTaskProgress(int userID, int questID, int taskID, UserQuestTaskProgress& taskProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT unitsDone, taskVar, finished FROM UserQuestTaskProgress WHERE userID = ? AND questID = ? AND taskID = ? LIMIT 1"));
		statement.bind(1, userID);
		statement.bind(2, questID);
		statement.bind(3, taskID);
		if (statement.executeStep())
		{
			taskProgress.unitsDone = statement.getColumn(0);
			taskProgress.taskVar = statement.getColumn(1);
			taskProgress.finished = (char)statement.getColumn(2);
		}
		
		taskProgress.taskID = taskID;
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetQuestTaskProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateQuestTaskProgress(int userID, int questID, UserQuestTaskProgress& taskProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE UserQuestTaskProgress SET unitsDone = ?, taskVar = ?, finished = ? WHERE userID = ? AND questID = ? AND taskID = ?"));
		statement.bind(1, taskProgress.unitsDone);
		statement.bind(2, taskProgress.taskVar);
		statement.bind(3, taskProgress.finished);
		statement.bind(4, userID);
		statement.bind(5, questID);
		statement.bind(6, taskProgress.taskID);
		if (!statement.exec())
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserQuestTaskProgress VALUES (?, ?, ?, ?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, questID);
			statement.bind(3, taskProgress.taskID);
			statement.bind(4, taskProgress.unitsDone);
			statement.bind(5, taskProgress.taskVar);
			statement.bind(6, taskProgress.finished);
			statement.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateQuestTaskProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

bool CUserDatabaseSQLite::IsQuestTaskFinished(int userID, int questID, int taskID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM UserQuestTaskProgress WHERE userID = ? AND questID = ? AND taskID = ? AND finished = 0 LIMIT 1)"));
		query.bind(1, userID);
		query.bind(2, questID);
		query.bind(3, taskID);

		if (query.executeStep())
		{
			if ((int)query.getColumn(0))
			{
				return false;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsAllQuestTasksFinished: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return false;
	}

	return true;
}

string GetQuestStatString(int flag, bool update)
{
	ostringstream query;
	query << (update ? OBFUSCATE("UPDATE UserQuestStat SET") : OBFUSCATE("SELECT"));
	if (flag & 0x2)
		query << (update ? OBFUSCATE(" continiousSpecialQuest,") : OBFUSCATE(" continiousSpecialQuest = ?,"));
	if (flag & 0x8)
		query << (update ? OBFUSCATE(" dailyMissionsCompletedToday,") : OBFUSCATE(" dailyMissionsCompletedToday = ?,"));
	if (flag & 0x20)
		query << (update ? OBFUSCATE(" dailyMissionsCleared,") : OBFUSCATE(" dailyMissionsCleared = ?,"));

	return query.str();
}

int CUserDatabaseSQLite::GetQuestStat(int userID, int flag, UserQuestStat& stat)
{
	try
	{
		// format query
		string query = GetQuestStatString(flag, false);
		query[query.size() - 1] = ' ';
		query += OBFUSCATE("FROM UserQuestStat WHERE userID = ? LIMIT 1"); // TODO: use stringstream

		SQLite::Statement statement(m_Database, query);
		statement.bind(1, userID);
		if (statement.executeStep())
		{
			int index = 0;
			if (flag & 0x2)
			{
				stat.continiousSpecialQuest = statement.getColumn(index++);
			}
			if (flag & 0x8)
			{
				stat.dailyMissionsCompletedToday = statement.getColumn(index++);
			}
			if (flag & 0x20)
			{
				stat.dailyMissionsCleared = statement.getColumn(index++);
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetQuestStat: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateQuestStat(int userID, int flag, UserQuestStat& stat)
{
	try
	{
		// format query
		string query = GetQuestStatString(flag, true);
		query[query.size() - 1] = ' ';
		query += OBFUSCATE("WHERE userID = ?"); // TODO: use stringstream

		SQLite::Statement statement(m_Database, query);
		int index = 1;

		if (flag & 0x2)
		{
			statement.bind(index++, stat.continiousSpecialQuest);
		}
		if (flag & 0x8)
		{
			statement.bind(index++, stat.dailyMissionsCompletedToday);
		}
		if (flag & 0x20)
		{
			statement.bind(index++, stat.dailyMissionsCleared);
		}

		statement.bind(index++, userID);

		statement.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateQuestStat: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetBingoProgress(int userID, UserBingo& bingo)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT status, canPlay FROM UserMiniGameBingo WHERE userID = ? LIMIT 1"));
		statement.bind(1, userID);
		if (statement.executeStep())
		{
			bingo.status = statement.getColumn(0);
			bingo.canPlay = (char)statement.getColumn(1);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetBingoProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateBingoProgress(int userID, UserBingo& bingo)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE UserMiniGameBingo SET status = ?, canPlay = ? WHERE userID = ?"));
		statement.bind(1, bingo.status);
		statement.bind(2, bingo.canPlay);
		statement.bind(3, userID);
		if (!statement.exec())
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserMiniGameBingo VALUES (?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, bingo.status);
			statement.bind(3, bingo.canPlay);
			statement.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBingoProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetBingoSlot(int userID, vector<UserBingoSlot>& slots)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT number, opened FROM UserMiniGameBingoSlot WHERE userID = ?"));
		statement.bind(1, userID);
		while (statement.executeStep())
		{
			UserBingoSlot slot;
			slot.number = statement.getColumn(0);
			slot.opened = (char)statement.getColumn(1);

			slots.push_back(slot);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetBingoSlot: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateBingoSlot(int userID, vector<UserBingoSlot>& slots, bool remove)
{
	try
	{
		if (remove)
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("DELETE FROM UserMiniGameBingoSlot WHERE userID = ?"));
			statement.bind(1, userID);
			statement.exec();
		}

		for (UserBingoSlot& slot : slots)
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE UserMiniGameBingoSlot SET opened = ? WHERE userID = ? AND number = ?"));
			statement.bind(1, slot.opened);
			statement.bind(2, userID);
			statement.bind(3, slot.number);
			if (!statement.exec())
			{
				// TODO: use transaction or multiple insert
				for (UserBingoSlot& slot : slots)
				{
					SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserMiniGameBingoSlot VALUES (?, ?, ?)"));
					statement.bind(1, userID);
					statement.bind(2, slot.number);
					statement.bind(3, slot.opened);
					statement.exec();
				}
				break;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBingoSlot: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetBingoPrizeSlot(int userID, vector<UserBingoPrizeSlot>& prizes)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT idx, opened, itemID, count, duration, relatesTo FROM UserMiniGameBingoPrizeSlot WHERE userID = ?"));
		statement.bind(1, userID);
		while (statement.executeStep())
		{
			UserBingoPrizeSlot slot;
			slot.index = statement.getColumn(0);
			slot.opened = (char)statement.getColumn(1);
			slot.item.itemID = statement.getColumn(2);
			slot.item.count = statement.getColumn(3);
			slot.item.duration = statement.getColumn(4);
			slot.bingoIndexes = deserialize_array_int(statement.getColumn(5));

			prizes.push_back(slot);
		}
		statement.reset();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetBingoPrizeSlot: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateBingoPrizeSlot(int userID, vector<UserBingoPrizeSlot>& prizes, bool remove)
{
	try
	{
		if (remove)
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("DELETE FROM UserMiniGameBingoPrizeSlot WHERE userID = ?"));
			statement.bind(1, userID);
			statement.exec();
		}

		for (UserBingoPrizeSlot& prize : prizes)
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE UserMiniGameBingoPrizeSlot SET opened = ? WHERE userID = ? AND idx = ?"));
			statement.bind(1, prize.opened);
			statement.bind(2, userID);
			statement.bind(3, prize.index);
			if (!statement.exec())
			{
				// TODO: use transaction or multiple insert
				for (UserBingoPrizeSlot& prize : prizes)
				{
					SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserMiniGameBingoPrizeSlot VALUES (?, ?, ?, ?, ?, ?, ?)"));
					statement.bind(1, userID);
					statement.bind(2, prize.index);
					statement.bind(3, prize.opened);
					statement.bind(4, prize.item.itemID);
					statement.bind(5, prize.item.count);
					statement.bind(6, prize.item.duration);
					statement.bind(7, serialize_array_int(prize.bingoIndexes));
					statement.exec();
				}
				break;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBingoPrizeSlot: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user rank
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetUserRank(int userID, CUserCharacter& character)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT tierOri, tierZM, tierZPVE, tierDM FROM UserRank WHERE userID = ? LIMIT 1"));
		statement.bind(1, userID);

		if (statement.executeStep())
		{
			for (int i = 0; i < 4; i++)
				character.tier[i] = statement.getColumn(i);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetUserRank: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user rank
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateUserRank(int userID, CUserCharacter& character)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE UserRank SET tierOri = ?, tierZM = ?, tierZPVE = ?, tierDM = ? WHERE userID = ?"));
		int index = 1;
		for (int i = 0; i < 4; i++)
			statement.bind(index++, character.tier[i]);

		statement.bind(index++, userID);

		statement.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateUserRank: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets ban list
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetBanList(int userID, vector<string>& banList)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT gameName, (SELECT NOT EXISTS(SELECT 1 FROM UserCharacter WHERE gameName = UserBanList.gameName LIMIT 1)) FROM UserBanList WHERE userID = ?"));
		query.bind(1, userID);
		while (query.executeStep())
		{
			banList.push_back((const char*)query.getColumn(0));
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates ban list(add/remove)
// returns 0 == database error, -1 on user not exists, -2 on user exists(add), -3 on limit exceeded(add), 1 on success
int CUserDatabaseSQLite::UpdateBanList(int userID, string gameName, bool remove)
{
	try
	{
		if (remove)
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserBanList WHERE userID = ? AND gameName = ?"));
			query.bind(1, userID);
			query.bind(2, gameName);
			if (!query.exec())
			{
				return -1;
			}
		}
		else
		{
			{
				SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM UserCharacter WHERE gameName = ? LIMIT 1)"));
				query.bind(1, gameName);
				if (query.executeStep())
				{
					if (!(int)query.getColumn(0))
					{
						return -1;
					}
				}
			}
			{
				SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM UserBanList WHERE userID = ? AND gameName = ? LIMIT 1)"));
				query.bind(1, userID);
				query.bind(2, gameName);
				if (query.executeStep())
				{
					if ((int)query.getColumn(0))
					{
						return -2;
					}
				}
			}
			{
				SQLite::Statement query(m_Database, OBFUSCATE("SELECT COUNT(1) FROM UserBanList WHERE userID = ?"));
				query.bind(1, userID);
				if (query.executeStep())
				{
					if ((int)query.getColumn(0) >= g_pServerConfig->banListMaxSize)
					{
						return -3;
					}
				}
			}
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserBanList VALUES (?, ?)"));
			query.bind(1, userID);
			query.bind(2, gameName);
			if (!query.exec())
			{
				return 0;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates ban list(add/remove)
// returns 0 == database error or user not in ban list, 1 on user in ban list
bool CUserDatabaseSQLite::IsInBanList(int userID, int destUserID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT NOT EXISTS(SELECT 1 FROM UserBanList WHERE userID = ? AND gameName = (SELECT gameName FROM UserCharacter WHERE userID = ? LIMIT 1) LIMIT 1)"));
		query.bind(1, userID);
		query.bind(2, destUserID);
		if (query.executeStep())
		{
			if ((int)query.getColumn(0))
			{
				return false;
			}
		}

	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsInBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return false;
	}

	return true;
}

bool CUserDatabaseSQLite::IsSurveyAnswered(int userID, int surveyID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM UserSurveyAnswer WHERE userID = ? AND surveyID = ? LIMIT 1)"));
		query.bind(1, userID);
		query.bind(2, surveyID);
		if (query.executeStep())
		{
			return (char)query.getColumn(0);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsSurveyAnswered: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return true;
	}

	return true;
}

int CUserDatabaseSQLite::SurveyAnswer(int userID, UserSurveyAnswer& answer)
{
	try
	{
		for (auto& questionAnswer : answer.questionsAnswers)
		{
			if (questionAnswer.checkBox)
			{
				for (auto& answerStr : questionAnswer.answers)
				{
					SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserSurveyAnswer VALUES (?, ?, ?, ?)"));
					query.bind(1, answer.surveyID);
					query.bind(2, questionAnswer.questionID);
					query.bind(3, userID);
					query.bind(4, answerStr);
					if (!query.exec())
					{
						return 0;
					}
				}
			}
			else
			{
				SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserSurveyAnswer VALUES (?, ?, ?, ?)"));
				query.bind(1, answer.surveyID);
				query.bind(2, questionAnswer.questionID);
				query.bind(3, userID);
				query.bind(4, questionAnswer.answers[0]);
				if (!query.exec())
				{
					return 0;
				}
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::SurveyAnswer: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetWeaponReleaseRows(int userID, vector<UserWeaponReleaseRow>& rows)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT slot, character, opened FROM UserMiniGameWeaponReleaseItemProgress WHERE userID = ?"));
		query.bind(1, userID);
		while (query.executeStep())
		{
			UserWeaponReleaseRow row;
			row.id = query.getColumn(0);
			row.progress = query.getColumn(1);
			row.opened = (char)query.getColumn(2);

			rows.push_back(row);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetWeaponReleaseProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetWeaponReleaseRow(int userID, UserWeaponReleaseRow& row)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT character, opened FROM UserMiniGameWeaponReleaseItemProgress WHERE userID = ? AND slot = ? LIMIT 1"));
		query.bind(1, userID);
		query.bind(2, row.id);
		if (query.executeStep())
		{
			row.progress = query.getColumn(0);
			row.opened = (char)query.getColumn(1);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetWeaponReleaseRow: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateWeaponReleaseRow(int userID, UserWeaponReleaseRow& row)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserMiniGameWeaponReleaseItemProgress SET character = ?, opened = ? WHERE userID = ? AND slot = ?"));
		query.bind(1, row.progress);
		query.bind(2, row.opened);
		query.bind(3, userID);
		query.bind(4, row.id);
		if (!query.exec())
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserMiniGameWeaponReleaseItemProgress VALUES (?, ?, ?, ?)"));
			query.bind(1, userID);
			query.bind(2, row.id);
			query.bind(3, row.progress);
			query.bind(4, row.opened);
			query.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateWeaponReleaseRow: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetWeaponReleaseCharacters(int userID, vector<UserWeaponReleaseCharacter>& characters, int& totalCount)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT character, count FROM UserMiniGameWeaponReleaseCharacters WHERE userID = ?"));
		query.bind(1, userID);
		while (query.executeStep())
		{
			UserWeaponReleaseCharacter character;
			character.character = query.getColumn(0);
			character.count = query.getColumn(1);

			totalCount += character.count;

			characters.push_back(character);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetWeaponReleaseCharacters: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetWeaponReleaseCharacter(int userID, UserWeaponReleaseCharacter& character)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT count FROM UserMiniGameWeaponReleaseCharacters WHERE userID = ? AND character = ? LIMIT 1"));
		query.bind(1, userID);
		query.bind(2, character.character);
		if (query.executeStep())
		{
			character.count = query.getColumn(0);
		}
		else
		{
			return -1;
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetWeaponReleaseCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateWeaponReleaseCharacter(int userID, UserWeaponReleaseCharacter& character)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserMiniGameWeaponReleaseCharacters SET count = count + ? WHERE userID = ? AND character = ?"));
		query.bind(1, character.count);
		query.bind(2, userID);
		query.bind(3, character.character);
		if (!query.exec())
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserMiniGameWeaponReleaseCharacters VALUES (?, ?, ?)"));
			query.bind(1, userID);
			query.bind(2, character.character);
			query.bind(3, character.count);
			query.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateWeaponReleaseCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::SetWeaponReleaseCharacter(int userID, int weaponSlot, int slot, int character, bool opened)
{
	try
	{
		SQLite::Transaction transaction(m_Database);
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserMiniGameWeaponReleaseItemProgress SET character = character | (1 << ?), opened = ? WHERE userID = ? AND slot = ?"));
			query.bind(1, slot);
			query.bind(2, opened);
			query.bind(3, userID);
			query.bind(4, weaponSlot);
			if (!query.exec())
			{
				SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserMiniGameWeaponReleaseItemProgress VALUES(?, ?, (1 << ?), ?)"));
				query.bind(1, userID);
				query.bind(2, weaponSlot);
				query.bind(3, slot);
				query.bind(4, opened);
				if (!query.exec())
				{
					return -1;
				}
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserMiniGameWeaponReleaseCharacters SET count = count - 1 WHERE userID = ? AND character = ?"));
			query.bind(1, userID);
			query.bind(2, character);
			if (!query.exec())
			{
				return -1;
			}
		}
		transaction.commit();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateWeaponReleaseCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}


int CUserDatabaseSQLite::GetAddons(int userID, vector<int>& addons)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT itemID FROM UserAddon WHERE userID = ? LIMIT ?"));
		query.bind(1, userID);
		query.bind(2, ADDON_COUNT);
		while (query.executeStep())
		{
			addons.push_back(query.getColumn(0));
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetAddons: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::SetAddons(int userID, vector<int>& addons)
{
	try
	{
		SQLite::Transaction transaction(m_Database);

		SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserAddon WHERE userID = ?"));
		query.bind(1, userID);
		query.exec();

		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserAddon VALUES (?, ?)"));

			for (auto itemID : addons)
			{
				query.bind(1, userID);
				query.bind(2, itemID);
				if (!query.exec())
				{
					return -1;
				}
				query.reset();
				query.clearBindings();
			}
		}

		transaction.commit();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::SetAddons: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetUsersAssociatedWithIP(const string& ip, vector<CUserData>& userData)
{
	try
	{
		SQLite::Statement query(m_Database, "SELECT DISTINCT User.userID, User.userName FROM User "
			"INNER JOIN UserSessionHistory "
			"ON User.userID = UserSessionHistory.userID "
			"AND UserSessionHistory.ip = ? "
			"GROUP BY User.userID "
			"HAVING COUNT(DISTINCT User.userID) = 1");
		query.bind(1, ip);
		while (query.executeStep())
		{
			CUserData data;
			data.userID = query.getColumn(0);
			data.userName = query.getColumn(1).getString();
			userData.push_back(data);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetUsersAssociatedWithIP: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetUsersAssociatedWithHWID(const vector<unsigned char>& hwid, vector<CUserData>& userData)
{
	try
	{
		SQLite::Statement query(m_Database, "SELECT DISTINCT User.userID, User.userName FROM User "
			"INNER JOIN UserSessionHistory "
			"ON User.userID = UserSessionHistory.userID "
			"AND UserSessionHistory.hwid = ? "
			"GROUP BY User.userID "
			"HAVING COUNT(DISTINCT User.userID) = 1");
		query.bind(1, hwid.data(), (int)hwid.size());
		while (query.executeStep())
		{
			CUserData data;
			data.userID = query.getColumn(0);
			data.userName = query.getColumn(1).getString();
			userData.push_back(data);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetUsersAssociatedWithHWID: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::CreateClan(ClanCreateConfig& clanCfg)
{
	int clanID = 0;
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM Clan WHERE name = ? LIMIT 1)"));
			query.bind(1, clanCfg.name);
			if (query.executeStep())
			{
				if ((int)query.getColumn(0))
					return -1;
			}
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM ClanMember WHERE userID = ? LIMIT 1)"));
			query.bind(1, clanCfg.masterUserID);
			if (query.executeStep())
			{
				if ((int)query.getColumn(0))
					return -2;
			}
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM ClanMemberRequest WHERE userID = ? LIMIT 1)"));
			query.bind(1, clanCfg.masterUserID);
			if (query.executeStep())
			{
				if ((int)query.getColumn(0))
					return -2;
			}
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT points FROM UserCharacter WHERE userID = ? LIMIT 1"));
			query.bind(1, clanCfg.masterUserID);
			if (query.executeStep())
			{
				int points = query.getColumn(0);
				if (points < 30000) // 30k points
				{
					return -3;
				}
			}
		}

		SQLite::Transaction transaction(m_Database);
		{
			IUser* user = g_UserManager.GetUserById(clanCfg.masterUserID);
			if (!user->UpdatePoints(-30000))
			{
				return 0;
			}
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanIDNext FROM UserDist"));
			query.executeStep();
			clanID = query.getColumn(0);
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO Clan VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
			query.bind(1, clanID);
			query.bind(2, clanCfg.masterUserID);
			query.bind(3, clanCfg.name);
			query.bind(4, clanCfg.description);
			query.bind(5, clanCfg.noticeMsg);
			query.bind(6, clanCfg.gameModeID);
			query.bind(7, clanCfg.mapID);
			query.bind(8, clanCfg.time);
			query.bind(9, 1); // member count
			query.bind(10, clanCfg.expBoost);
			query.bind(11, clanCfg.pointBoost);
			query.bind(12, clanCfg.region);
			query.bind(13, clanCfg.joinMethod);
			query.bind(14, clanCfg.points); // score
			query.bind(15, 0); // markID
			query.bind(16, 0); // markColor
			query.bind(17, clanCfg.markChangeCount); // markChangeCount
			query.bind(18, 450); // max member count

			query.exec();
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserDist SET clanIDNext = clanIDNext + 1"));
			query.exec();
		}

		CUserCharacter character = {};
		character.lowFlag = UFLAG_LOW_GAMENAME;
		if (GetCharacter(clanCfg.masterUserID, character) <= 0)
		{
			return 0;
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO ClanMember VALUES (?, ?, ?)"));
			query.bind(1, clanID);
			query.bind(2, clanCfg.masterUserID);
			query.bind(3, 0);
			if (!query.exec())
			{
				return 0;
			}
		}

		{
			for (int i = 0; i < 5; i++)
			{
				SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO ClanStoragePage VALUES (?, ?, ?)"));
				query.bind(1, clanID);
				query.bind(2, i);
				query.bind(3, 3);
				if (!query.exec())
				{
					return 0;
				}

				for (int k = 0; k < 20; k++)
				{
					SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO ClanStorageItem VALUES (?, ?, ?, ?, ?, ?, ?)"));
					query.bind(1, clanID);
					query.bind(2, i);
					query.bind(3, k);
					query.bind(4, 0);
					query.bind(5, 0);
					query.bind(6, 0);
					query.bind(7, 0);
					if (!query.exec())
					{
						return 0;
					}
				}
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO ClanChronicle VALUES (?, strftime('%Y%m%d', datetime(?, 'unixepoch')), ?, '')"));
			query.bind(1, clanID);
			query.bind(2, g_pServerInstance->GetCurrentTime() * 60); // convert minutes to seconds
			query.bind(3, 0); // clan create
			if (!query.exec())
			{
				return 0;
			}
		}

		transaction.commit();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::CreateClan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return clanID;
}

int CUserDatabaseSQLite::JoinClan(int userID, int clanID, string& clanName)
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM ClanMember WHERE userID = ? AND clanID = ? LIMIT 1)"));
			query.bind(1, userID);
			query.bind(2, clanID);
			if (query.executeStep())
			{
				if ((int)query.getColumn(0))
				{
					if (0)
					{
						/*if (LeaveClan(userID) <= 0)
						{
						return 0;
						}*/
					}
					else
					{
						return -1; // user already in this clan
					}
				}
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT maxMemberCount, memberCount FROM Clan WHERE clanID = ? LIMIT 1"));
			query.bind(1, clanID);
			if (query.executeStep())
			{
				int maxMemberCount = query.getColumn(0);
				int memberCount = query.getColumn(1);
				if (memberCount >= maxMemberCount)
				{
					return -6;
				}
			}
			else
			{
				return 0;
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT joinMethod FROM Clan WHERE clanID = ? LIMIT 1"));
			query.bind(1, clanID);
			if (query.executeStep())
			{
				int joinMethod = query.getColumn(0);
				switch (joinMethod)
				{
				case 0:
					return -3; // Not Recruiting
				case 1:
					//if (!inviterUserID)
					//{
					return -4; // Family Member Invitation Required
							   //}
				case 2:
				{
					{
						SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMemberRequest WHERE userID = ? LIMIT 1"));
						query.bind(1, userID);
						if (query.executeStep())
						{
							int reqClanID = query.getColumn(0);
							if (reqClanID == clanID)
							{
								return -2; // already sent join request
							}
							else
							{
								SQLite::Statement query(m_Database, OBFUSCATE("SELECT name FROM Clan WHERE clanID = ? LIMIT 1"));
								query.bind(1, reqClanID);
								if (query.executeStep())
								{
									clanName = (const char*)query.getColumn(0);
								}

								return -7;
							}
						}
					}

					// add to member request list
					int inviterUserID = 0;
					{
						SQLite::Statement query(m_Database, OBFUSCATE("SELECT userID FROM ClanInvite WHERE destUserID = ? AND clanID = ? LIMIT 1"));
						query.bind(1, userID);
						query.bind(2, clanID);
						if (query.executeStep())
						{
							inviterUserID = query.getColumn(0);

							SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM ClanInvite WHERE destUserID = ? AND clanID = ?"));
							query.bind(1, userID);
							query.bind(2, clanID);
							query.exec();
						}
					}
					{
						SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO ClanMemberRequest VALUES (?, ?, ?, strftime('%Y%m%d', datetime(?, 'unixepoch')))"));
						query.bind(1, clanID);
						query.bind(2, userID);
						query.bind(3, inviterUserID);
						query.bind(4, g_pServerInstance->GetCurrentTime() * 60); // convert minutes to seconds
						if (!query.exec())
						{
							return 0;
						}
					}

					return -5; // Approval Required
				}
				}
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO ClanMember VALUES (?, ?, ?)"));
			query.bind(1, clanID);
			query.bind(2, userID);
			query.bind(3, 3); // Associate Member by default
			if (!query.exec())
			{
				return 0;
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE Clan SET memberCount = memberCount + 1 WHERE clanID = ?"));
			query.bind(1, clanID);
			if (!query.exec())
			{
				return 0;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::JoinClan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::CancelJoin(int userID, int clanID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM ClanMemberRequest WHERE userID = ? AND clanID = ?"));
		query.bind(1, userID);
		query.bind(2, clanID);
		if (!query.exec())
		{
			return -1; // not in request list
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::CancelJoin: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::LeaveClan(int userID)
{
	try
	{
		int clanID = 0;
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade != 0 LIMIT 1"));
			query.bind(1, userID);
			if (!query.executeStep())
			{
				return -1; // user not in clan or has no permission to leave
			}
			else
			{
				clanID = query.getColumn(0);
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM ClanMember WHERE clanID = ? AND userID = ?"));
			query.bind(1, clanID);
			query.bind(2, userID);
			if (!query.exec())
			{
				return 0;
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE Clan SET memberCount = memberCount - 1 WHERE clanID = ?"));
			query.bind(1, clanID);
			if (!query.exec())
			{
				return 0;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::LeaveClan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::DissolveClan(int userID)
{
	try
	{
		int clanID = 0;
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1 LIMIT 1"));
			query.bind(1, userID);
			if (!query.executeStep())
			{
				return -1; // user not in clan or not admin
			}
			else
			{
				clanID = query.getColumn(0);
			}
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM Clan WHERE clanID = ?"));
			query.bind(1, clanID);
			if (!query.exec())
			{
				return 0;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::DissolveClan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanList(vector<ClanList_s>& clans, string clanName, int flag, int gameModeID, int playTime, int pageID, int &pageMax)
{
	try
	{
		static SQLite::Statement query(m_Database, OBFUSCATE("SELECT Clan.clanID, gameName, name, notice, gameModeID, time, region, memberCount, joinMethod, score, markID FROM Clan, UserCharacter WHERE userID = Clan.masterUserID AND CASE WHEN ? == 0 THEN name LIKE ('%' || ? || '%') WHEN ? == 1 THEN gameModeID = ? WHEN ? == 2 THEN time = ? WHEN ? == 3 THEN gameModeID = ? AND time = ? END ORDER BY score DESC LIMIT 15 OFFSET ? * 15"));
		if (query.getErrorCode())
			query.reset();

		query.bind(1, flag);
		query.bind(2, clanName);
		query.bind(3, flag);
		query.bind(4, gameModeID);
		query.bind(5, flag);
		query.bind(6, playTime);
		query.bind(7, flag);
		query.bind(8, gameModeID);
		query.bind(9, playTime);
		query.bind(10, pageID);
		while (query.executeStep())
		{
			ClanList_s clanList = {};
			clanList.id = query.getColumn(0);
			clanList.clanMaster = (const char*)query.getColumn(1);
			clanList.name = (const char*)query.getColumn(2);
			clanList.noticeMsg = (const char*)query.getColumn(3);
			clanList.gameModeID = query.getColumn(4);
			clanList.time = query.getColumn(5);
			clanList.region = query.getColumn(6);
			clanList.memberCount = query.getColumn(7);
			clanList.joinMethod = query.getColumn(8);
			clanList.score = query.getColumn(9);
			clanList.markID = query.getColumn(10);

			clans.push_back(clanList);
		}

		query.reset();

		{
			static SQLite::Statement query(m_Database, OBFUSCATE("SELECT COUNT(1) / 15 + 1 FROM Clan"));
			if (query.getErrorCode())
				query.reset();

			if (query.executeStep())
			{
				pageMax = query.getColumn(0);
			}

			query.reset();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetClanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanInfo(int clanID, Clan_s& clan)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT Clan.clanID, gameName, name, notice, gameModeID, mapID, time, memberCount, expBonus, pointBonus, markID, maxMemberCount FROM Clan, UserCharacter WHERE userID = Clan.masterUserID AND Clan.clanID = ? LIMIT 1"));
		query.bind(1, clanID);
		if (query.executeStep())
		{
			clan.id = query.getColumn(0);
			clan.clanMaster = (const char*)query.getColumn(1);
			clan.name = (const char*)query.getColumn(2);
			clan.noticeMsg = (const char*)query.getColumn(3);
			clan.gameModeID = query.getColumn(4);
			clan.mapID = query.getColumn(5);
			clan.time = query.getColumn(6);
			clan.memberCount = query.getColumn(7);
			clan.expBoost = query.getColumn(8);
			clan.pointBoost = query.getColumn(9);
			clan.markID = query.getColumn(10);
			clan.maxMemberCount = query.getColumn(11);
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT itemID, itemCount, itemDuration FROM ClanStorageItem WHERE clanID = ? AND itemID != 0 LIMIT 10"));
			query.bind(1, clanID);
			while (query.executeStep())
			{
				RewardItem item;
				item.itemID = query.getColumn(0);
				item.count = query.getColumn(1);
				item.duration = query.getColumn(2);

				clan.lastStorageItems.push_back(item);
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT date, type, string FROM ClanChronicle WHERE clanID = ?"));
			query.bind(1, clanID);
			while (query.executeStep())
			{
				ClanChronicle chr;
				chr.date = query.getColumn(0);
				chr.type = query.getColumn(1);
				chr.str = (const char*)query.getColumn(2);

				clan.chronicle.push_back(chr);
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetClanInfo: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::AddClanStorageItem(int userID, int pageID, CUserInventoryItem& item)
{
	try
	{
		SQLite::Transaction transaction(m_Database);

		int clanID = 0;
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= (SELECT accessGrade FROM ClanStoragePage WHERE clanID = ClanMember.clanID AND pageID = ? LIMIT 1) LIMIT 1"));
			query.bind(1, userID);
			query.bind(2, pageID);
			if (!query.executeStep())
			{
				// user not in clan or access grade is lower than user's member grade
				return -1;
			}
			else
			{
				clanID = query.getColumn(0);
			}
		}

		int slot = 0;
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT slot FROM ClanStorageItem WHERE clanID = ? AND pageID = ? AND itemID = 0 LIMIT 1"));
			query.bind(1, clanID);
			query.bind(2, pageID);
			if (!query.executeStep())
			{
				return -3;
			}
			else
			{
				slot = query.getColumn(0);
			}
		}

		if (GetInventoryItemBySlot(userID, item.GetSlot(), item) <= 0)
		{
			return -2;
		}

		// TODO: add more checks for item
		if (item.m_nItemID <= 0 || item.m_nIsClanItem)
		{
			return -2;
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE ClanStorageItem SET itemID = ?, itemCount = ?, itemDuration = ?, itemEnhValue = ? WHERE clanID = ? AND pageID = ? AND slot = ?"));
			query.bind(1, item.m_nItemID);
			query.bind(2, item.m_nCount);
			query.bind(3, item.m_nExpiryDate * CSO_24_HOURS_IN_MINUTES + g_pServerInstance->GetCurrentTime());
			query.bind(4, item.m_nEnhanceValue);
			query.bind(5, clanID);
			query.bind(6, pageID);
			query.bind(7, slot);
			if (!query.exec())
			{
				return 0;
			}
		}

		item.Reset();
		if (UpdateInventoryItem(userID, item, UITEM_FLAG_ALL) <= 0)
		{
			return -2;
		}

		transaction.commit();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::AddClanStorageItem: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::DeleteClanStorageItem(int userID, int pageID, int slot)
{
	try
	{
		int clanID = 0;
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= (SELECT accessGrade FROM ClanStoragePage WHERE clanID = ClanMember.clanID AND pageID = ? LIMIT 1) LIMIT 1"));
			query.bind(1, userID);
			query.bind(2, pageID);
			if (!query.executeStep())
			{
				// user not in clan or access grade is lower than user's member grade
				return -1;
			}
			else
			{
				clanID = query.getColumn(0);
			}
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE ClanStorageItem SET itemID = 0, itemCount = 0, itemDuration = 0 WHERE clanID = ? AND pageID = ? AND slot = ?"));
			query.bind(1, clanID);
			query.bind(2, pageID);
			query.bind(3, slot);
			if (!query.exec())
			{
				return -1;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::DeleteClanStorageItem: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanStorageItem(int userID, int pageID, int slot, CUserInventoryItem& item)
{
	// TODO: check for item limit
	try
	{
		//int clanID = 0;
		{
			// SELECT itemID, itemCount, itemDuration FROM ClanStorageItem INNER JOIN Clan ON ClanStorageItem.clanID = Clan.clanID INNER JOIN ClanStoragePage ON Clan.clanID = ClanStoragePage.clanID AND ClanStoragePage.pageID = ClanStorageItem.pageID INNER JOIN ClanMember ON ClanMember.userID = ? WHERE ClanMember.memberGrade <= ClanStoragePage.accessGrade AND ClanStorageItem.pageID = ? AND ClanStorageItem.slot = ?
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT itemID, itemCount FROM ClanStorageItem INNER JOIN ClanStoragePage ON ClanStorageItem.clanID = ClanStoragePage.clanID AND ClanStoragePage.pageID = ClanStorageItem.pageID INNER JOIN ClanMember ON ClanMember.userID = ? WHERE ClanMember.memberGrade <= ClanStoragePage.accessGrade AND ClanStorageItem.clanID = ClanMember.clanID AND ClanStorageItem.pageID = ? AND ClanStorageItem.slot = ? AND ClanStorageItem.itemID != 0 LIMIT 1"));
			query.bind(1, userID);
			query.bind(2, pageID);
			query.bind(3, slot);
			if (!query.executeStep())
			{
				// user not in clan or access grade is lower than user's member grade or there is no item slot
				return -1;
			}
			else
			{
				item.m_nItemID = query.getColumn(0);
				item.m_nCount = query.getColumn(1);
				item.m_nExpiryDate = 1;
				item.ConvertDurationToExpiryDate();
				item.m_nIsClanItem = true;
				item.m_nInUse = 1;
				item.m_nStatus = 1;
			}
		}

		if (IsInventoryFull(userID))
		{
			return -2;
		}

		// we got an item, try to add it to user inventory
		int result = AddInventoryItem(userID, item);
		switch (result)
		{
		case -1:
			return -3;
		case -2:
			return -5;
		case 0:
			return 0;
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetClanStorageItem: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanStorageLastItems(int userID, std::vector<RewardItem>& items)
{
	Logger().Warn("CUserDatabaseSQLite::GetClanStorageLastItems: not implemented\n");
	return 0;
}

int CUserDatabaseSQLite::GetClanStoragePage(int userID, ClanStoragePage& clanStoragePage)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT slot, itemID, itemCount, itemDuration, itemEnhValue FROM ClanStorageItem WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? LIMIT 1) AND pageID = ? AND itemID != 0"));
		statement.bind(1, userID);
		statement.bind(2, clanStoragePage.pageID);
		while (statement.executeStep())
		{
			RewardItem item;
			item.selectID = statement.getColumn(0);
			item.itemID = statement.getColumn(1);
			item.count = statement.getColumn(2);
			item.duration = statement.getColumn(3);
			item.enhValue = statement.getColumn(4);

			clanStoragePage.items.push_back(item);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetClanStoragePage: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanStorageHistory(int userID, ClanStorageHistory& clanStorageHistory)
{
	return false;
}

int CUserDatabaseSQLite::GetClanStorageAccessGrade(int userID, vector<int>& accessGrade)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT accessGrade FROM ClanStoragePage WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? LIMIT 1)"));
		query.bind(1, userID);
		while (query.executeStep())
		{
			accessGrade.push_back(query.getColumn(0));
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetClanStorageAccessGrade: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateClanStorageAccessGrade(int userID, int pageID, int accessGrade)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE ClanStoragePage SET accessGrade = ? WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? LIMIT 1) AND pageID = ? AND (SELECT memberGrade FROM ClanMember WHERE userID = ? LIMIT 1) <= 1"));
		statement.bind(1, accessGrade);
		statement.bind(2, userID);
		statement.bind(3, pageID);
		statement.bind(4, userID);
		if (!statement.exec())
		{
			return 0;
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateClanStorageAccessGrade: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanUserList(int id, bool byUser, vector<ClanUser>& users)
{
	try
	{
		// get clan user list by clanID or by userID
		string strQuery;
		if (!byUser)
		{
			strQuery = OBFUSCATE("SELECT ClanMember.userID, gameName, userName, memberGrade FROM ClanMember INNER JOIN UserCharacter ON UserCharacter.userID = ClanMember.userID AND ClanMember.clanID = ? INNER JOIN User ON UserCharacter.userID = User.userID");
		}
		else
		{
			strQuery = OBFUSCATE("SELECT ClanMember.userID, gameName, userName, memberGrade FROM ClanMember INNER JOIN UserCharacter ON UserCharacter.userID = ClanMember.userID AND ClanMember.clanID = (SELECT clanID FROM UserCharacter WHERE userID = ? LIMIT 1) INNER JOIN User ON UserCharacter.userID = User.userID");
		}

		SQLite::Statement query(m_Database, strQuery);
		query.bind(1, id);
		while (query.executeStep())
		{
			ClanUser clanUser = {};
			clanUser.userID = query.getColumn(0);
			clanUser.character.gameName = (const char*)query.getColumn(1);
			clanUser.userName = (const char*)query.getColumn(2);
			clanUser.user = g_UserManager.GetUserById(clanUser.userID);
			clanUser.memberGrade = query.getColumn(3);

			users.push_back(clanUser);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetClanUserList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanMemberList(int userID, vector<ClanUser>& users)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT ClanMember.userID, memberGrade, gameName, userName, level, kills, deaths FROM ClanMember INNER JOIN UserCharacter ON ClanMember.userID = UserCharacter.userID INNER JOIN User ON UserCharacter.userID = User.userID WHERE (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1 LIMIT 1) = ClanMember.clanID"));
		query.bind(1, userID);
		while (query.executeStep())
		{
			ClanUser clanUser = {};
			clanUser.userID = query.getColumn(0);
			clanUser.memberGrade = query.getColumn(1);
			clanUser.character.gameName = (const char*)query.getColumn(2);
			clanUser.userName = (const char*)query.getColumn(3);
			clanUser.character.level = query.getColumn(4);
			clanUser.character.kills = query.getColumn(5);
			clanUser.character.deaths = query.getColumn(6);
			clanUser.user = g_UserManager.GetUserById(clanUser.userID);

			users.push_back(clanUser);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetClanMemberList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanMemberJoinUserList(int userID, vector<ClanUserJoinRequest>& users)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT ClanMemberRequest.userID, userName, gameName, level, kills, deaths, (SELECT gameName FROM UserCharacter WHERE userID = inviterUserID LIMIT 1), date FROM ClanMemberRequest INNER JOIN UserCharacter ON ClanMemberRequest.userID = UserCharacter.userID INNER JOIN ClanMember ON ClanMember.userID = ? AND ClanMember.memberGrade <= 1 AND ClanMember.clanID = ClanMemberRequest.clanID INNER JOIN User ON User.userID = ClanMemberRequest.userID"));
		statement.bind(1, userID);
		while (statement.executeStep())
		{
			ClanUserJoinRequest clanUser;
			clanUser.userID = statement.getColumn(0);
			clanUser.userName = (const char*)statement.getColumn(1);
			clanUser.character.gameName = (const char*)statement.getColumn(2);
			clanUser.character.level = statement.getColumn(3);
			clanUser.character.kills = statement.getColumn(4);
			clanUser.character.deaths = statement.getColumn(5);
			clanUser.inviterGameName = (const char*)statement.getColumn(6);
			clanUser.date = statement.getColumn(7);

			users.push_back(clanUser);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetClanMemberJoinUserList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

string GetClanString(int flag, bool update)
{
	ostringstream query;
	query << (update ? OBFUSCATE("UPDATE Clan SET") : OBFUSCATE("SELECT"));
	if (flag & CFLAG_NAME)
		query << (update ? OBFUSCATE(" name = ?,") : OBFUSCATE(" name,"));
	if (flag & CFLAG_MASTERUID)
		query << (update ? OBFUSCATE(" masterUserID = ?,") : OBFUSCATE(" masterUserID,"));
	if (flag & CFLAG_TIME)
		query << (update ? OBFUSCATE(" time = ?,") : OBFUSCATE(" time,"));
	if (flag & CFLAG_GAMEMODEID)
		query << (update ? OBFUSCATE(" gameModeID = ?,") : OBFUSCATE(" gameModeID,"));
	if (flag & CFLAG_MAPID)
		query << (update ? OBFUSCATE(" mapID = ?,") : OBFUSCATE(" mapID,"));
	if (flag & CFLAG_REGION)
		query << (update ? OBFUSCATE(" region = ?,") : OBFUSCATE(" region,"));
	if (flag & CFLAG_JOINMETHOD)
		query << (update ? OBFUSCATE(" joinMethod = ?,") : OBFUSCATE(" joinMethod,"));
	if (flag & CFLAG_EXPBOOST)
		query << (update ? OBFUSCATE(" expBonus = ?,") : OBFUSCATE(" expBonus,"));
	if (flag & CFLAG_POINTBOOST)
		query << (update ? OBFUSCATE(" pointBonus = ?,") : OBFUSCATE(" pointBonus,"));
	if (flag & CFLAG_NOTICEMSG)
		query << (update ? OBFUSCATE(" notice = ?,") : OBFUSCATE(" notice,"));
	if (flag & CFLAG_SCORE)
		query << (update ? OBFUSCATE(" score = ?,") : OBFUSCATE(" score,"));
	if (flag & CFLAG_MARKID)
		query << (update ? OBFUSCATE(" markID = ?,") : OBFUSCATE(" markID,"));
	if (flag & CFLAG_MARKCOLOR)
		query << (update ? OBFUSCATE(" markColor = ?,") : OBFUSCATE(" markColor,"));
	if (!update && flag & CFLAG_ID)
		query << OBFUSCATE(" clanID,");
	if (!update && flag & CFLAG_CLANMASTER)
		query << OBFUSCATE(" (SELECT gameName FROM UserCharacter WHERE userID = Clan.masterUserID),");
	if (flag & CFLAG_MARKCHANGECOUNT)
		query << (update ? OBFUSCATE(" markChangeCount = ?,") : OBFUSCATE(" markChangeCount,"));
	if (flag & CFLAG_MAXMEMBERCOUNT)
		query << (update ? OBFUSCATE(" maxMemberCount = ?,") : OBFUSCATE(" maxMemberCount,"));

	return query.str();
}

int CUserDatabaseSQLite::GetClan(int userID, int flag, Clan_s& clan)
{
	try
	{
		// format query
		string query = GetClanString(flag, false);
		query[query.size() - 1] = ' ';
		query += OBFUSCATE("FROM Clan WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? LIMIT 1) LIMIT 1"); // TODO: use stringstream

		SQLite::Statement statement(m_Database, query);
		statement.bind(1, userID);

		if (statement.executeStep())
		{
			int index = 0;
			if (flag & CFLAG_NAME)
			{
				clan.name = (const char*)statement.getColumn(index++);
			}
			if (flag & CFLAG_MASTERUID)
			{
				clan.masterUserID = statement.getColumn(index++);
			}
			if (flag & CFLAG_TIME)
			{
				clan.time = statement.getColumn(index++);
			}
			if (flag & CFLAG_GAMEMODEID)
			{
				clan.gameModeID = statement.getColumn(index++);
			}
			if (flag & CFLAG_MAPID)
			{
				clan.mapID = statement.getColumn(index++);
			}
			if (flag & CFLAG_REGION)
			{
				clan.region = statement.getColumn(index++);
			}
			if (flag & CFLAG_JOINMETHOD)
			{
				clan.joinMethod = statement.getColumn(index++);
			}
			if (flag & CFLAG_EXPBOOST)
			{
				clan.expBoost = statement.getColumn(index++);
			}
			if (flag & CFLAG_POINTBOOST)
			{
				clan.pointBoost = statement.getColumn(index++);
			}
			if (flag & CFLAG_NOTICEMSG)
			{
				clan.noticeMsg = (const char*)statement.getColumn(index++);
			}
			if (flag & CFLAG_SCORE)
			{
				clan.score = statement.getColumn(index++);
			}
			if (flag & CFLAG_MARKID)
			{
				clan.markID = statement.getColumn(index++);
			}
			if (flag & CFLAG_MARKCOLOR)
			{
				clan.markColor = statement.getColumn(index++);
			}
			if (flag & CFLAG_ID)
			{
				clan.id = statement.getColumn(index++);
			}
			if (flag & CFLAG_CLANMASTER)
			{
				clan.clanMaster = (const char*)statement.getColumn(index++);
			}
			if (flag & CFLAG_MARKCHANGECOUNT)
			{
				clan.markChangeCount = statement.getColumn(index++);
			}
			if (flag & CFLAG_MAXMEMBERCOUNT)
			{
				clan.maxMemberCount = statement.getColumn(index++);
			}
			if (flag & CFLAG_CHRONICLE)
			{
				// TODO: rewrite
				{
					SQLite::Statement query(m_Database, OBFUSCATE("SELECT date, type, string FROM ClanChronicle WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? LIMIT 1)"));
					query.bind(1, userID);
					while (query.executeStep())
					{
						ClanChronicle chr;
						chr.date = query.getColumn(0);
						chr.type = query.getColumn(1);
						chr.str = (const char*)query.getColumn(2);

						clan.chronicle.push_back(chr);
					}
				}
			}
		}
		else
		{
			return -1;
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetClan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanMember(int userID, ClanUser& clanUser)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT ClanMember.userID, memberGrade, gameName, userName, level, kills, deaths FROM ClanMember INNER JOIN UserCharacter ON UserCharacter.userID = ClanMember.userID AND ClanMember.clanID = (SELECT clanID FROM UserCharacter WHERE userID = ? LIMIT 1) INNER JOIN User ON User.userID = UserCharacter.userID LIMIT 1"));
		query.bind(1, userID);
		if (query.executeStep())
		{
			ClanUser clanUser = {};
			clanUser.userID = query.getColumn(0);
			clanUser.memberGrade = query.getColumn(1);
			clanUser.character.gameName = (const char*)query.getColumn(2);
			clanUser.userName = (const char*)query.getColumn(3);
			clanUser.character.level = query.getColumn(4);
			clanUser.character.kills = query.getColumn(5);
			clanUser.character.deaths = query.getColumn(6);
			clanUser.user = g_UserManager.GetUserById(clanUser.userID);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetClanMember: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateClan(int userID, int flag, Clan_s clan)
{
	try
	{
		// format query
		string query = GetClanString(flag, true);
		query[query.size() - 1] = ' ';
		query += OBFUSCATE("WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1 LIMIT 1)"); // TODO: use stringstream

		SQLite::Statement statement(m_Database, query);
		int index = 1;

		if (flag & CFLAG_NAME)
		{
			statement.bind(index++, clan.name);
		}
		if (flag & CFLAG_MASTERUID)
		{
			statement.bind(index++, clan.masterUserID);
		}
		if (flag & CFLAG_TIME)
		{
			statement.bind(index++, clan.time);
		}
		if (flag & CFLAG_GAMEMODEID)
		{
			statement.bind(index++, clan.gameModeID);
		}
		if (flag & CFLAG_MAPID)
		{
			statement.bind(index++, clan.mapID);
		}
		if (flag & CFLAG_REGION)
		{
			statement.bind(index++, clan.region);
		}
		if (flag & CFLAG_JOINMETHOD)
		{
			statement.bind(index++, clan.joinMethod);
		}
		if (flag & CFLAG_EXPBOOST)
		{
			statement.bind(index++, clan.expBoost);
		}
		if (flag & CFLAG_POINTBOOST)
		{
			statement.bind(index++, clan.pointBoost);
		}
		if (flag & CFLAG_NOTICEMSG)
		{
			statement.bind(index++, clan.noticeMsg);
		}
		if (flag & CFLAG_SCORE)
		{
			statement.bind(index++, clan.score);
		}
		if (flag & CFLAG_MARKID)
		{
			statement.bind(index++, clan.markID);
		}
		if (flag & CFLAG_MARKCOLOR)
		{
			statement.bind(index++, clan.markColor);
		}
		if (flag & CFLAG_MARKCHANGECOUNT)
		{
			statement.bind(index++, clan.markChangeCount);
		}
		if (flag & CFLAG_MAXMEMBERCOUNT)
		{
			statement.bind(index++, clan.maxMemberCount);
		}

		statement.bind(index++, userID);

		if (!statement.exec())
		{
			return -1; // not clan master
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateClan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateClanMemberGrade(int userID, const string& targetUserName, int newGrade, ClanUser& targetMember)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("UPDATE ClanMember SET memberGrade = ? WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade = 0 LIMIT 1) AND userID = (SELECT userID FROM User WHERE userName = ? LIMIT 1)"));
		query.bind(1, newGrade);
		query.bind(2, userID);
		query.bind(3, targetUserName);
		if (!query.exec())
		{
			return -1; // not clan master
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT ClanMember.userID, memberGrade, gameName, userName, level, kills, deaths FROM ClanMember INNER JOIN UserCharacter ON UserCharacter.userID = ClanMember.userID INNER JOIN User ON User.userID = UserCharacter.userID AND User.userName = ? LIMIT 1"));
			query.bind(1, targetUserName);
			if (query.executeStep())
			{
				targetMember.userID = query.getColumn(0);
				targetMember.memberGrade = query.getColumn(1);
				targetMember.character.gameName = (const char*)query.getColumn(2);
				targetMember.userName = (const char*)query.getColumn(3);
				targetMember.character.level = query.getColumn(4);
				targetMember.character.kills = query.getColumn(5);
				targetMember.character.deaths = query.getColumn(6);
				targetMember.user = g_UserManager.GetUserByUsername(targetUserName);
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateClanMemberGrade: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::ClanReject(int userID, const string& userName)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM ClanMemberRequest WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1 LIMIT 1) AND userID = (SELECT userID FROM User WHERE userName = ? LIMIT 1)"));
		query.bind(1, userID);
		query.bind(2, userName);
		if (!query.exec())
		{
			return -1; // not clan master
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::ClanReject: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::ClanRejectAll(int userID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM ClanMemberRequest WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade = 0 LIMIT 1)"));
		query.bind(1, userID);
		if (!query.exec())
		{
			return -1; // not clan master
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::ClanRejectAll: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::ClanApprove(int userID, const string& userName)
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM ClanMemberRequest WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1 LIMIT 1) AND userID = (SELECT userID FROM User WHERE userName = ? LIMIT 1)"));
			query.bind(1, userID);
			query.bind(2, userName);
			if (!query.exec())
			{
				return -1; // not clan master
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO ClanMember VALUES ((SELECT clanID FROM ClanMember WHERE userID = ? LIMIT 1), (SELECT userID FROM User WHERE userName = ? LIMIT 1), ?)"));
			query.bind(1, userID);
			query.bind(2, userName);
			query.bind(3, 3);
			if (!query.exec())
			{
				return -1;
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE Clan SET memberCount = memberCount + 1 WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? LIMIT 1)"));
			query.bind(1, userID);
			if (!query.exec())
			{
				return 0;
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserCharacter SET clanID = (SELECT clanID FROM ClanMember WHERE userID = ? LIMIT 1) WHERE userID = (SELECT userID FROM User WHERE userName = ? LIMIT 1)"));
			query.bind(1, userID);
			query.bind(2, userName);
			if (!query.exec())
			{
				return 0;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::ClanApprove: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::IsClanWithMarkExists(int markID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT NOT EXISTS(SELECT 1 FROM Clan WHERE markID = ? LIMIT 1)"));
		query.bind(1, markID);
		if (query.executeStep())
		{
			if ((int)query.getColumn(0))
				return -1;
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsClanWithMarkExists: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::ClanInvite(int userID, const string& gameName, IUser*& destUser, int& clanID)
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1 LIMIT 1"));
			query.bind(1, userID);
			if (!query.executeStep())
			{
				return -1; // only admin and family master can invite other users
			}
			else
			{
				clanID = query.getColumn(0);
			}
		}
		int destUserID = 0;
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT userID FROM UserCharacter WHERE gameName = ? LIMIT 1"));
			query.bind(1, gameName);
			if (!query.executeStep())
			{
				return -3; // user does not exist
			}
			else
			{
				destUserID = query.getColumn(0);
			}
		}

		destUser = g_UserManager.GetUserById(destUserID);
		if (!destUser)
		{
			return -2; // user is offline(TODO: are there more conditions?)
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID, memberGrade FROM ClanMember WHERE userID = ? LIMIT 1"));
			query.bind(1, destUserID);
			if (query.executeStep())
			{
				int destClanID = query.getColumn(0);
				int destMemberGrade = query.getColumn(1);

				if (destMemberGrade == 0)
				{
					return -4; // user is clan master
				}
				else if (destClanID == clanID)
				{
					return -5; // user already in the same clan
				}
			}
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO ClanInvite VALUES(?, ?, ?)"));
			query.bind(1, clanID);
			query.bind(2, userID);
			query.bind(3, destUserID);
			query.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::ClanInvite: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::ClanKick(int userID, const string& userName)
{
	try
	{
		int clanID = 0;
		{
			// TODO: rewrite query (family master can't kick admins)
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1 LIMIT 1"));
			query.bind(1, userID);
			if (!query.executeStep())
			{
				return -1; // only admin and family master can kick other users
			}
			else
			{
				clanID = query.getColumn(0);
			}
		}
		int targetUserID = 0;
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT userID FROM ClanMember WHERE userID = (SELECT userID FROM User WHERE userName = ? LIMIT 1) AND memberGrade > (SELECT memberGrade FROM ClanMember WHERE userID = ? LIMIT 1) LIMIT 1"));
			query.bind(1, userName);
			query.bind(2, userID);
			if (!query.executeStep())
			{
				return -1;
			}
			else
			{
				targetUserID = query.getColumn(0);
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM ClanMember WHERE clanID = ? AND userID = ?"));
			query.bind(1, clanID);
			query.bind(2, targetUserID);
			if (!query.exec())
			{
				return 0;
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE Clan SET memberCount = memberCount - 1 WHERE clanID = ?"));
			query.bind(1, clanID);
			if (!query.exec())
			{
				return 0;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::ClanKick: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::ClanMasterDelegate(int userID, const string& userName)
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE ClanMember SET memberGrade = 0 WHERE userID = (SELECT userID FROM User WHERE userName = ? LIMIT 1) AND memberGrade = 1 AND (SELECT memberGrade FROM ClanMember WHERE userID = ? LIMIT 1) = 0"));
			query.bind(1, userName);
			query.bind(2, userID);
			if (!query.exec())
			{
				return -1;
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE ClanMember SET memberGrade = 1 WHERE userID = ?"));
			query.bind(1, userID);
			if (!query.exec())
			{
				return 0;
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO ClanChronicle VALUES ((SELECT clanID FROM ClanMember WHERE userID = ? LIMIT 1), strftime('%Y%m%d', datetime(?, 'unixepoch')), ?, ?)"));
			query.bind(1, userID);
			query.bind(2, g_pServerInstance->GetCurrentTime() * 60); // convert minutes to seconds
			query.bind(3, 1); // master change
			query.bind(4, userName);
			if (!query.exec())
			{
				return 0;
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::ClanMasterDelegate: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::IsClanExists(const string& clanName)
{
	int clanID = 0;
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM Clan WHERE name = ? LIMIT 1"));
		query.bind(1, clanName);
		if (query.executeStep())
		{
			clanID = query.getColumn(0);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsClanExists: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return clanID;
}


int CUserDatabaseSQLite::GetQuestEventProgress(int userID, int questID, UserQuestProgress& questProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT status, favourite, started FROM UserQuestEventProgress WHERE userID = ? AND questID = ? LIMIT 1"));
		statement.bind(1, userID);
		statement.bind(2, questID);
		if (statement.executeStep())
		{
			questProgress.status = statement.getColumn(0);
			questProgress.favourite = (char)statement.getColumn(1);
			questProgress.started = (char)statement.getColumn(2);
		}

		questProgress.questID = questID;
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetQuestEventProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateQuestEventProgress(int userID, const UserQuestProgress& questProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE UserQuestEventProgress SET status = ?, favourite = ?, started = ? WHERE userID = ? AND questID = ?"));
		statement.bind(1, questProgress.status);
		statement.bind(2, questProgress.favourite);
		statement.bind(3, questProgress.started);
		statement.bind(4, userID);
		statement.bind(5, questProgress.questID);
		if (!statement.exec())
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserQuestEventProgress VALUES (?, ?, ?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, questProgress.questID);
			statement.bind(3, questProgress.status);
			statement.bind(4, questProgress.favourite);
			statement.bind(5, questProgress.started);
			statement.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateQuestEventProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetQuestEventTaskProgress(int userID, int questID, int taskID, UserQuestTaskProgress& taskProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT unitsDone, taskVar, finished FROM UserQuestEventTaskProgress WHERE userID = ? AND questID = ? AND taskID = ? LIMIT 1"));
		statement.bind(1, userID);
		statement.bind(2, questID);
		statement.bind(3, taskID);
		if (statement.executeStep())
		{
			taskProgress.unitsDone = statement.getColumn(0);
			taskProgress.taskVar = statement.getColumn(1);
			taskProgress.finished = (char)statement.getColumn(2);
		}

		taskProgress.taskID = taskID;
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetQuestEventTaskProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateQuestEventTaskProgress(int userID, int questID, const UserQuestTaskProgress& taskProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE UserQuestEventTaskProgress SET unitsDone = ?, taskVar = ?, finished = ? WHERE userID = ? AND questID = ? AND taskID = ?"));
		statement.bind(1, taskProgress.unitsDone);
		statement.bind(2, taskProgress.taskVar);
		statement.bind(3, taskProgress.finished);
		statement.bind(4, userID);
		statement.bind(5, questID);
		statement.bind(6, taskProgress.taskID);
		if (!statement.exec())
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserQuestEventTaskProgress VALUES (?, ?, ?, ?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, questID);
			statement.bind(3, taskProgress.taskID);
			statement.bind(4, taskProgress.unitsDone);
			statement.bind(5, taskProgress.taskVar);
			statement.bind(6, taskProgress.finished);
			statement.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateQuestEventTaskProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

bool CUserDatabaseSQLite::IsQuestEventTaskFinished(int userID, int questID, int taskID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT NOT EXISTS(SELECT 1 FROM UserQuestEventTaskProgress WHERE userID = ? AND questID = ? AND taskID = ? AND finished = 1 LIMIT 1)"));
		query.bind(1, userID);
		query.bind(2, questID);
		query.bind(3, taskID);
		if (query.executeStep())
		{
			if ((int)query.getColumn(0))
				return false;
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsQuestEventTaskFinished: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return false;
	}

	return true;
}

// checks if user exists
// returns 0 == database error, 1 == user exists
int CUserDatabaseSQLite::IsUserExists(int userID)
{
	int retVal = 0;
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM User WHERE userID = ? LIMIT 1)"));
		statement.bind(1, userID);
		if (statement.executeStep())
		{
			return statement.getColumn(0);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsUserExists: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return retVal;
}

// checks if user exists
// returns 0 == database error or user does not exists, > 0 == userID
int CUserDatabaseSQLite::IsUserExists(const string& userName, bool searchByUserName)
{
	int userID = 0;
	try
	{
		string query = searchByUserName ? OBFUSCATE("SELECT userID FROM User WHERE userName = ? LIMIT 1") : OBFUSCATE("SELECT userID FROM UserCharacter WHERE gameName = ? LIMIT 1");

		SQLite::Statement statement(m_Database, query);
		statement.bind(1, userName);
		if (statement.executeStep())
		{
			userID = statement.getColumn(0);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsUserExists: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return userID;
}

// adds user suspect action to suspect actions list
// actionID(0 - DLLMOD, 1 - old client build, 2 - more than 2 accounts with the same HWID)
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::SuspectAddAction(const vector<unsigned char>& hwid, int actionID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO SuspectAction VALUES (?, ?, ?)"));
		query.bind(1, hwid.data(), (int)hwid.size());
		query.bind(2, actionID);
		query.bind(3, g_pServerInstance->GetCurrentTime());
		query.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::SuspectAddAction: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// checks if user is in suspect list
// returns 0 == database error or user not in suspect list, 1 on user is not suspect
int CUserDatabaseSQLite::IsUserSuspect(int userID)
{
	try
	{
		// TODO: check all hwid logged in on this account
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM SuspectAction WHERE hwid = (SELECT lastHWID FROM User WHERE userID = ? LIMIT 1) LIMIT 1)"));
		query.bind(1, userID);
		if (query.executeStep())
		{
			return query.getColumn(0);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsUserSuspect: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 0;
}

// processes user database every minute
void CUserDatabaseSQLite::OnMinuteTick(time_t curTime)
{
	try
	{
		// process inventory
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT userID, slot, itemID, status, inUse FROM UserInventory WHERE expiryDate != 0 AND inUse = 1 AND expiryDate < ?"));
			query.bind(1, curTime);

			while (query.executeStep())
			{
				CUserInventoryItem item(query.getColumn(1), query.getColumn(2), 0, query.getColumn(3), query.getColumn(4), 0, 0, 0, 0, 0, 0, 0, {}, 0, 0, 0);
				int userID = query.getColumn(0);

				IUser* user = g_UserManager.GetUserById(userID);
				if (user)
				{
					g_PacketManager.SendUMsgExpiryNotice(user->GetExtendedSocket(), vector<int>{ item.m_nItemID });

					g_ItemManager.RemoveItem(userID, user, item);
				}
				else
				{
					// add to notice list
					UpdateExpiryNotices(userID, item.m_nItemID);

					g_ItemManager.RemoveItem(userID, user, item);
				}
			}

			query.reset();
		}

		// delete ban rows with expired term
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserBan WHERE term <= ?"));
			query.bind(1, curTime);
			query.exec();
		}

		// update session time for user session
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserSession SET sessionTime = sessionTime + 1"));
			query.exec();
		}

		// delete storage items with expired date
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE ClanStorageItem SET itemID = 0, itemDuration = 0, itemCount = 0 WHERE itemID != 0 AND itemDuration <= ?"));
			query.bind(1, curTime);
			query.exec();
		}

		// check if day tick need to be done
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT nextDayResetTime FROM TimeConfig WHERE ? >= nextDayResetTime"));
			query.bind(1, curTime);
			if (query.executeStep())
			{				
				time_t tempCurTime = curTime;
				tempCurTime *= 60;
				tempCurTime += CSO_24_HOURS_IN_SECONDS;
				tm* localTime = localtime(&tempCurTime);
				localTime->tm_hour = 6;
				localTime->tm_sec = 0;
				localTime->tm_min = 0;

				time_t dayTick = mktime(localTime);
				dayTick /= 60;

				SQLite::Statement query(m_Database, OBFUSCATE("UPDATE TimeConfig SET nextDayResetTime = ?"));
				query.bind(1, dayTick);
				if (!query.exec())
				{
					SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO TimeConfig VALUES (?, 0)"));
					query.bind(1, dayTick);
					query.exec();
				}

				OnDayTick();
			}
		}

		// check if week tick need to be done
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT nextWeekResetTime FROM TimeConfig WHERE ? >= nextWeekResetTime"));
			query.bind(1, curTime);
			if (query.executeStep())
			{
				time_t tempCurTime = curTime;
				tempCurTime *= 60;
				tempCurTime += CSO_24_HOURS_IN_SECONDS * 7;
				tm* localTime = localtime(&tempCurTime);
				localTime->tm_hour = 6;
				localTime->tm_sec = 0;
				localTime->tm_min = 0;

				time_t weekTick = mktime(localTime);
				weekTick /= 60;

				SQLite::Statement query(m_Database, OBFUSCATE("UPDATE TimeConfig SET nextWeekResetTime = ?"));
				query.bind(1, weekTick);
				if (!query.exec())
				{
					query.bind(1, weekTick);
					query.exec();
				}

				OnWeekTick();
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::OnMinuteTick: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
	}
}

// processes user database every day
// returns 0 == database error, 1 on success
void CUserDatabaseSQLite::OnDayTick()
{
	try
	{
#ifndef PUBLIC_RELEASE
		// do backup
		tm* time = g_pServerInstance->GetCurrentLocalTime();
		m_Database.backup(va(OBFUSCATE("UserDatabase_%d_%d_%d_%d.db3"), time->tm_year + 1900, time->tm_mon, time->tm_mday, time->tm_hour), SQLite::Database::BackupType::Save);
#endif

		// quests
		// reset daily/special quest progress
		int dailyQuests = 0;
		{
			//SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserQuestProgress SET status = 0 WHERE questID > 0 AND questID < 2000 AND (? - (SELECT lastLogonTime FROM User)) <= 1440"));
			//query.bind(1, g_pServerInstance->GetCurrentTime());
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserQuestProgress SET status = 0 WHERE questID > 0 AND questID < 2000"));
			dailyQuests = query.exec();
		}
		// reset task daily/special quest progress
		{
			//SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestTaskProgress WHERE questID > 0 AND questID < 2000 AND (? - (SELECT lastLogonTime FROM User)) <= 1440"));
			//query.bind(1, g_pServerInstance->GetCurrentTime());
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestTaskProgress WHERE questID > 0 AND questID < 2000"));
			query.exec();
		}

		// event quests
		// reset daily quest progress
		int dailyEventQuests = 0;
		{
			// shitty condition
			//SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestEventProgress WHERE questID > 2000 AND questID < 4000 AND (? - (SELECT lastLogonTime FROM User)) <= 1440"));
			//query.bind(1, g_pServerInstance->GetCurrentTime());
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestEventProgress WHERE questID > 2000 AND questID < 4000"));
			dailyEventQuests = query.exec();
		}
		// reset task daily quest progress
		{
			//SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestEventTaskProgress WHERE questID > 2000 AND questID < 4000 AND (? - (SELECT lastLogonTime FROM User)) <= 1440"));
			//query.bind(1, g_pServerInstance->GetCurrentTime());
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestEventTaskProgress WHERE questID > 2000 AND questID < 4000"));
			query.exec();
		}

		Logger().Info(OBFUSCATE("CUserDatabaseSQLite::OnDayTick: daily quest: %d, daily event quest: %d\n"), dailyQuests, dailyEventQuests);

		// get affected users and send quest update
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT userID FROM UserSession WHERE (? - (SELECT lastLogonTime FROM User)) < 1440"));
			query.bind(1, g_pServerInstance->GetCurrentTime());
			while (query.executeStep())
			{
				int userID = query.getColumn(0);
				IUser* user = g_UserManager.GetUserById(userID);
				if (user)
				{
					Logger().Info(OBFUSCATE("CUserDatabaseSQLite::OnDayTick: TODO: fix user client-side quest update\n"));

					vector<UserQuestProgress> progress;
					GetQuestsProgress(userID, progress);
					g_PacketManager.SendQuests(user->GetExtendedSocket(), userID, g_QuestManager.GetQuests(), progress, 0xFFFF, 0, 0, 0);
				}
			}
		}

		Logger().Warn(OBFUSCATE("CUserDatabaseSQLite::OnWeekTick: TODO: impl userdailyreward\n"));
		/*
		// reset daily rewards random items
		m_Database.exec(OBFUSCATE("DELETE FROM UserDailyRewardItems"));
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserDailyReward SET day = 0 WHERE canGetReward = 1 OR day >= 7"));
			query.exec();
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserDailyReward SET canGetReward = 1"));
			query.exec();
		}
		// update daily reward items
		for (auto user : g_UserManager.users)
		{
			UserDailyRewards dailyReward = {};
			GetDailyRewards(user->GetID(), dailyReward);
			g_ItemManager.UpdateDailyRewardsRandomItems(dailyReward);
			UpdateDailyRewards(user->GetID(), dailyReward);

			g_PacketManager.SendItemDailyRewardsUpdate(user->GetExtendedSocket(), g_pServerConfig->dailyRewardsItems, dailyReward);
		}*/
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::OnDayTick: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
	}
}

// processes user database every week
// returns 0 == database error, 1 on success
void CUserDatabaseSQLite::OnWeekTick()
{
	try
	{
		// reset week quest progress
		{
			// questID 1-500 = daily quests
			// questID 500-1000 = weekly quests
			// questID 1000-2000 = special quests
			// questID 2000-? = honor quests

			// TODO: affect only users who logged in last week at least once
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserQuestProgress SET status = 0 WHERE questID > 500 AND questID < 1000"));
			query.exec();
		}
		// reset task week quest progress
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestTaskProgress WHERE questID > 500 AND questID < 1000"));
			query.exec();
		}

		// event quests
		// questID 0-2000 = non resetable quests
		// questID 2000-4000 = daily quests
		// questID 4000-6000 = weekly quests
		// reset week quest progress
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestEventProgress WHERE questID > 4000 AND questID < 6000"));
			query.exec();
		}
		// reset task week quest progress
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestEventTaskProgress WHERE questID > 4000 AND questID < 6000"));
			query.exec();
		}

		// get affected users and send quest update
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT userID FROM UserSession WHERE (? - (SELECT lastLogonTime FROM User)) <= 10080"));
			query.bind(1, g_pServerInstance->GetCurrentTime());
			while (query.executeStep())
			{
				int userID = query.getColumn(0);
				IUser* user = g_UserManager.GetUserById(userID);
				if (user)
				{
					Logger().Warn(OBFUSCATE("CUserDatabaseSQLite::OnWeekTick: TODO: fix user client-side quest update\n"));

					vector<UserQuestProgress> progress;
					GetQuestsProgress(userID, progress);
					g_PacketManager.SendQuests(user->GetExtendedSocket(), userID, g_QuestManager.GetQuests(), progress, 0xFFFF, 0xFFFF, 0, 0);

					/*vector<Quest_s> quests;
					vector<UserQuestProgress> questsProgress;
					SQLite::Statement query(m_Database, OBFUSCATE("SELECT questID FROM UserQuestProgress WHERE questID > 500 AND questID < 1000 AND userID = ?"));
					query.bind(1, userID);
					while (query.executeStep())
					{
					UserQuestProgress progress;
					progress.questID = query.getColumn(0);
					progress.status = 0;
					questsProgress.push_back(progress);

					Quest_s quest;
					quest.id = progress.questID;
					quests.push_back(quest);
					}
					g_PacketManager.SendQuests(user->GetExtendedSocket(), userID, quests, questsProgress, 0x20, 0, 0, 0);*/
				}
			}
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::OnWeekTick: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
	}
}

map<int, UserBan> CUserDatabaseSQLite::GetUserBanList()
{
	map<int, UserBan> banList;
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserBan"));
		while (statement.executeStep())
		{
			UserBan ban;
			ban.banType = statement.getColumn(1);
			string reason = statement.getColumn(2);
			ban.reason = reason;
			ban.term = statement.getColumn(3);

			banList.insert(pair<int, UserBan>(statement.getColumn(0), ban));
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetUserBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return banList;
	}

	return banList;
}

vector<int> CUserDatabaseSQLite::GetUsers(int lastLoginTime)
{
	vector<int> users;
	try
	{
		string query = OBFUSCATE("SELECT userID FROM User");

		SQLite::Statement statement(m_Database, query);
		while (statement.executeStep())
		{
			users.push_back(statement.getColumn(0));
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetUsers: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return users;
	}

	return users;
}

int CUserDatabaseSQLite::UpdateIPBanList(const string& ip, bool remove)
{
	try
	{
		if (remove)
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM IPBanList WHERE ip = ?"));
			query.bind(1, ip);
			query.exec();
		}
		else
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT or IGNORE INTO IPBanList VALUES (?)"));
			query.bind(1, ip);
			query.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateIPBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

vector<string> CUserDatabaseSQLite::GetIPBanList()
{
	vector<string> ip;
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT ip FROM IPBanList"));
		while (statement.executeStep())
		{
			ip.push_back(statement.getColumn(0));
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetIPBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return ip;
	}

	return ip;
}

bool CUserDatabaseSQLite::IsIPBanned(const string& ip)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT NOT EXISTS(SELECT 1 FROM IPBanList WHERE ip = ? LIMIT 1)"));
		query.bind(1, ip);
		if (query.executeStep())
		{
			if ((int)query.getColumn(0))
				return false;
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsIPBanned: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return false;
	}

	return true;
}

int CUserDatabaseSQLite::UpdateHWIDBanList(const vector<unsigned char>& hwid, bool remove)
{
	try
	{
		if (remove)
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM HWIDBanList WHERE hwid = ?"));
			query.bind(1, hwid.data(), (int)hwid.size());
			query.exec();
		}
		else
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT or IGNORE INTO HWIDBanList VALUES (?)"));
			query.bind(1, hwid.data(), (int)hwid.size());
			query.exec();
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateHWIDBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

vector<vector<unsigned char>> CUserDatabaseSQLite::GetHWIDBanList()
{
	vector<vector<unsigned char>> hwidList;
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT hwid FROM HWIDBanList"));
		while (statement.executeStep())
		{
			vector<unsigned char> hwid;
			SQLite::Column lastHWID = statement.getColumn(0);
			hwid.assign((unsigned char*)lastHWID.getBlob(), (unsigned char*)lastHWID.getBlob() + lastHWID.getBytes());
			hwidList.push_back(hwid);
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetHWIDBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return hwidList;
	}

	return hwidList;
}

bool CUserDatabaseSQLite::IsHWIDBanned(vector<unsigned char>& hwid)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT NOT EXISTS(SELECT 1 FROM HWIDBanList WHERE hwid = ? LIMIT 1)"));
		statement.bind(1, hwid.data(), (int)hwid.size());
		if (statement.executeStep())
		{
			if ((int)statement.getColumn(0))
				return false;
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsHWIDBanned: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return false;
	}

	return true;
}

bool CUserDatabaseSQLite::IsClassModExist(int userID, int slot)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM UserClassMod WHERE userID = ? AND slot = ? LIMIT 1)"));
		query.bind(1, userID);
		query.bind(2, slot);
		if (query.executeStep())
		{
			if ((int)query.getColumn(0))
				return true;
		}
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::IsClassModExist: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return false;
	}

	return false;
}

int CUserDatabaseSQLite::AddUserClassModLoadOut(int userID, int itemslot, const ClassModInfo_t& info)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserClassMod VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
		query.bind(1, userID);
		query.bind(2, itemslot);

		auto lambda = [](const ClassModInfo_t::ClassModSlot_t& slot) -> string
			{
				string value;
				for (int i = 0; i < 4; ++i)
				{
					if (slot.itemId[i] != -1)
						value = value.append(to_string(slot.itemId[i])).append(",");
					else
						value = value.append("-1,");
				}
				if (slot.itemId[4] != -1)
					value = value.append(to_string(slot.itemId[4]));
				else
					value = value.append("-1");
				return value;
			};

		string status, sessionbonus(lambda(info.sessionbonus)), displayinfo(lambda(info.infodisplay)), modbuff(lambda(info.modbuff)), activeskill(lambda(info.activeskill)), passiveskill(lambda(info.passiveskill)), addon(lambda(info.addon)), pairingweapon(lambda(info.pairingweapon));
		status = status.append(to_string(info.status.health)).append(",").append(to_string(info.status.attack)).append(",").append(to_string(info.status.speed)).append(",").append(to_string(info.status.armor)).append(",").append(to_string(info.status.ammo));

		query.bind(3, status);
		query.bind(4, sessionbonus);
		query.bind(5, displayinfo);
		query.bind(6, modbuff);
		query.bind(7, activeskill);
		query.bind(8, passiveskill);
		query.bind(9, addon);
		query.bind(10, pairingweapon);
		query.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::AddUserClassModLoadOut: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}
	return 1;
}

int CUserDatabaseSQLite::GetUserClassModLoadOut(int userID, int slot, ClassModInfo_t& info)
{
	try
	{
		auto lambda = [](string& column) -> std::vector<int>
			{
				std::vector<int> data;
				std::stringstream ss(column);
				std::string item;

				while (std::getline(ss, item, ','))
				{
					data.push_back(stoi(item));
				}
				return data;
			};

		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserClassMod WHERE userID = ? AND slot = ? LIMIT 1"));
		query.bind(1, userID);
		query.bind(2, slot);

		if (query.executeStep())
		{
			info.slotId = (int)query.getColumn(1) + (int)g_pServerConfig->defUser.defaultItems.size();

			std::vector<unsigned char> data;
			std::stringstream ss(query.getColumn(2).getString());
			std::string item;
			while (std::getline(ss, item, ','))
			{
				data.push_back(stoi(item));
			}
			info.status.health = data.at(0);
			info.status.attack = data.at(1);
			info.status.speed = data.at(2);
			info.status.armor = data.at(3);
			info.status.ammo = data.at(4);

			info.sessionbonus = *(ClassModInfo_t::ClassModSlot_t*)lambda(query.getColumn(3).getString()).data();
			info.infodisplay = *(ClassModInfo_t::ClassModSlot_t*)lambda(query.getColumn(4).getString()).data();
			info.modbuff = *(ClassModInfo_t::ClassModSlot_t*)lambda(query.getColumn(5).getString()).data();
			info.activeskill = *(ClassModInfo_t::ClassModSlot_t*)lambda(query.getColumn(6).getString()).data();
			info.passiveskill = *(ClassModInfo_t::ClassModSlot_t*)lambda(query.getColumn(7).getString()).data();
			info.addon = *(ClassModInfo_t::ClassModSlot_t*)lambda(query.getColumn(8).getString()).data();
			info.pairingweapon = *(ClassModInfo_t::ClassModSlot_t*)lambda(query.getColumn(9).getString()).data();
		}

	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::GetUserClassModLoadOut: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}
	return 1;
}

int CUserDatabaseSQLite::UpdateUserClassModLoadOut(int userID, int itemslot, const ClassModInfo_t& info)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserClassMod SET status = ?, sessionbonus = ?, displayinfo = ?, modbuff = ?, activeskill = ?, passiveskill = ?, addon = ?, pairingweapon = ? WHERE userID = ? AND slot = ?"));

		auto lambda = [](const ClassModInfo_t::ClassModSlot_t& slot) -> string
			{
				string value;
				for (int i = 0; i < 4; ++i)
				{
					if (slot.itemId[i] != -1)
						value = value.append(to_string(slot.itemId[i])).append(",");
					else
						value = value.append("-1,");
				}
				if (slot.itemId[4] != -1)
					value = value.append(to_string(slot.itemId[4]));
				else
					value = value.append("-1");
				return value;
			};

		string status, sessionbonus(lambda(info.sessionbonus)), displayinfo(lambda(info.infodisplay)), modbuff(lambda(info.modbuff)), activeskill(lambda(info.activeskill)), passiveskill(lambda(info.passiveskill)), addon(lambda(info.addon)), pairingweapon(lambda(info.pairingweapon));
		status = status.append(to_string(info.status.health)).append(",").append(to_string(info.status.attack)).append(",").append(to_string(info.status.speed)).append(",").append(to_string(info.status.armor)).append(",").append(to_string(info.status.ammo));

		query.bind(1, status);
		query.bind(2, sessionbonus);
		query.bind(3, displayinfo);
		query.bind(4, modbuff);
		query.bind(5, activeskill);
		query.bind(6, passiveskill);
		query.bind(7, addon);
		query.bind(8, pairingweapon);
		query.bind(9, userID);
		query.bind(10, itemslot);
		query.exec();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::UpdateUserClassModLoadOut: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

void CUserDatabaseSQLite::CreateTransaction()
{
	if (!m_pTransaction)
	{
		m_pTransaction = new SQLite::Transaction(m_Database);
	}
}

bool CUserDatabaseSQLite::CommitTransaction()
{
	try
	{
		m_pTransaction->commit();
	}
	catch (exception& e)
	{
		Logger().Error(OBFUSCATE("CUserDatabaseSQLite::CommitTransaction: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());

		delete m_pTransaction;
		m_pTransaction = NULL;

		return false;
	}

	delete m_pTransaction;
	m_pTransaction = NULL;

	return true;
}