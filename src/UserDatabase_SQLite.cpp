#include "UserDatabase_SQLite.h"
#include "ServerConfig.h"
#include "UserFastBuy.h"
#ifdef WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

using namespace std;

#define LAST_DB_VERSION 2
#define MAX_USER_REGISTRATIONS_PER_IP 3

//#define OBFUSCATE(data) (string)AY_OBFUSCATE_KEY(data, 'F')
#define OBFUSCATE(data) (char*)AY_OBFUSCATE_KEY(data, 'F')

CUserDatabaseSQLite::CUserDatabaseSQLite()
try : m_Database(OBFUSCATE("UserDatabase.db3"), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
	m_bInited = false;
}
catch (exception& e)
{
	g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite(): SQLite exception: %s\n"), e.what());
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
		}
		else
		{
			if (!ExecuteScript(OBFUSCATE("Data/SQL/main.sql")))
			{
				g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::CheckForTables: failed to execute main SQL script. Server may not work properly\n"));
				return false;
			}
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::CheckForTables: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return false;
	}

	return true;
}	

// upgrades user database to last version
bool CUserDatabaseSQLite::UpgradeDatabase(int currentDatabaseVer)
{
	for (int i = currentDatabaseVer; i < LAST_DB_VERSION; i++)
	{
		if (!ExecuteScript(va(OBFUSCATE("Data/SQL/Update_%d.sql"), i + 1)))
		{
			g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpgradeDatabase: file Update_%d.sql doesn't exist or sql execute returned error, current db ver: %d, last db ver: %d. Script not applied.\n"), i + 1, currentDatabaseVer, LAST_DB_VERSION);
			return false;
		}
	}

	m_Database.exec(va(OBFUSCATE("PRAGMA user_version = %d"), LAST_DB_VERSION));

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
			//printf(token);
			m_Database.exec(token);
			token = strtok(NULL, OBFUSCATE(";"));
		}

		transaction.commit();
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::ExecuteScript(%s): database internal error: %s, %d\n"), scriptPath.c_str(), e.what(), m_Database.getErrorCode());

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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::ExecuteOnce: failed to allocate buffer\n"));
		return false;
	}

	buffer[lSize] = '\0';

	size_t result = fread(buffer, 1, lSize, file);
	if (result != lSize)
	{
		fclose(file);
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::ExecuteOnce: failed to read file\n"));
		return false;
	}

	char* token = strtok(buffer, "\n");
	while (token)
	{
		g_pConsole->Log("CUserDatabaseSQLite::ExecuteOnce: executing %s\n", token);
		ExecuteScript(token);
		token = strtok(NULL, "\n");
	}

	fclose(file);

#ifdef WIN32
	_mkdir("Data/SQL/ExecutedHistory");

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
#else
#error
#endif
	return true;
}

// creates a new session for user
// returns > 0 == userID, 0 == database error, -1 == no such user or not in restore list, -2 == user already logged in, -4 == user banned
int CUserDatabaseSQLite::Login(string userName, string password, CExtendedSocket* socket, UserBan& ban, UserRestoreData* restoreData)
{
	try
	{
		auto start = ExecCalcStart();

		int userID = 0;
		if (restoreData)
		{
			// get userID if transferring server
			SQLite::Statement queryGetUser(m_Database, OBFUSCATE("SELECT userID FROM User WHERE userName = ?"));
			queryGetUser.bind(1, userName);

			if (!queryGetUser.executeStep())
			{
				ExecCalcEnd(start, OBFUSCATE("CUserDatabaseSQLite::Login"));

				return LOGIN_NO_SUCH_USER;
			}

			userID = queryGetUser.getColumn(0);
		}
		else
		{
			SQLite::Statement queryGetUser(m_Database, OBFUSCATE("SELECT userID FROM User WHERE userName = ? AND password = ?"));
			queryGetUser.bind(1, userName);
			queryGetUser.bind(2, password);

			if (!queryGetUser.executeStep())
			{
				ExecCalcEnd(start, OBFUSCATE("CUserDatabaseSQLite::Login"));

				return LOGIN_NO_SUCH_USER;
			}

			userID = queryGetUser.getColumn(0);
		}

		GetUserBan(userID, ban);
		if (ban.banType)
		{
			ExecCalcEnd(start, OBFUSCATE("CUserDatabaseSQLite::Login"));

			return LOGIN_USER_BANNED;
		}

		// check if user present in UserSession table
		SQLite::Statement queryGetUserSession(m_Database, OBFUSCATE("SELECT userID FROM UserSession WHERE userID = ?"));
		queryGetUserSession.bind(1, userID);
		if (queryGetUserSession.executeStep())
		{
			CUser* user = g_pUserManager->GetUserById(userID);
			if (user)
			{
				g_pUserManager->DisconnectUser(user);
			}
		}

		if (restoreData)
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT channelServerID, channelID FROM UserRestore WHERE userID = ?"));
			query.bind(1, userID);

			if (!query.executeStep())
			{
				ExecCalcEnd(start, OBFUSCATE("CUserDatabaseSQLite::Login"));

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
		void* hwid = socket->GetHWID().data();
		queryInsertUserSession.bind(4, hwid, socket->GetHWID().size());
		queryInsertUserSession.bind(5, UserStatus::STATUS_MENU);
		queryInsertUserSession.bind(6, 0);
		queryInsertUserSession.exec();

		ExecCalcEnd(start, OBFUSCATE("CUserDatabaseSQLite::Login"));

		return userID;
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::Login: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::AddToRestoreList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// creates a new user
// returns 0 == database error, 1 on success, -1 == user with the same username already exists, -4 == ip limit
int CUserDatabaseSQLite::Register(string userName, string password, string ip)
{
	try
	{
		auto start = ExecCalcStart();

		SQLite::Statement queryuser(m_Database, OBFUSCATE("SELECT userID FROM User WHERE userName = ?"));
		queryuser.bind(1, userName);
		queryuser.executeStep();

		if (queryuser.hasRow())
		{
			queryuser.reset();

			ExecCalcEnd(start, OBFUSCATE("CUserDatabaseSQLite::Register"));

			return -1;
		}

		// check registrations per IP limit
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT COUNT(*) FROM User WHERE registerIP = ?"));
			query.bind(1, ip);
			if (query.executeStep() && (int)query.getColumn(0) >= MAX_USER_REGISTRATIONS_PER_IP)
			{
				ExecCalcEnd(start, OBFUSCATE("CUserDatabaseSQLite::Register"));

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

		ExecCalcEnd(start, OBFUSCATE("CUserDatabaseSQLite::Register"));
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::Register: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO UserSessionHistory SELECT UserSession.userID, UserSession.ip, UserSession.hwid, (SELECT lastLogonTime FROM User WHERE userID = UserSession.userID), UserSession.sessionTime FROM UserSession WHERE UserSession.userID = ?"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::DropSession: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::DropSessions: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

void CUserDatabaseSQLite::PrintUserList()
{
	try
	{
		static SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM User"));
		cout << OBFUSCATE("Database user list:\n");
		while (query.executeStep())
		{
			cout << OBFUSCATE("UserID: ") << query.getColumn(0) << OBFUSCATE(", Username: ") << query.getColumn(1) << OBFUSCATE("\n");
		}
		query.reset();
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::PrintUserList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
	}
}

void CUserDatabaseSQLite::LoadBackup(string backupDate)
{
	try
	{
		m_Database.backup(va(OBFUSCATE("UserDatabase_%s.db3"), backupDate.c_str()), SQLite::Database::BackupType::Load);

		CheckForTables();

		g_pConsole->Log(OBFUSCATE("User database backup loaded successfully\n"));
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::LoadBackup: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
	}
}

void CUserDatabaseSQLite::PrintBackupList()
{
#ifdef WIN32
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;

	g_pConsole->Log(OBFUSCATE("SQLite user database backup list:\n"));
	if ((hFind = FindFirstFile(OBFUSCATE("UserDatabase_*.db3"), &FindFileData)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			g_pConsole->Log(OBFUSCATE("%s\n"), FindFileData.cFileName);
		} while (FindNextFile(hFind, &FindFileData));

		FindClose(hFind);
	}
#else
	printf("CUserDatabaseSQLite::PrintBackupList: not implemented\n");
#endif
}

void CUserDatabaseSQLite::ResetQuestEvent(int eventID)
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestEventProgress WHERE eventID = ?"));
			query.bind(1, eventID);
			query.exec();
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestEventTaskProgress WHERE eventID = ?"));
			query.bind(1, eventID);
			query.exec();
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::ResetQuestEvent: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
	}
}

void CUserDatabaseSQLite::WriteUserStatistic(string fdate, string sdate)
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::WriteUserStatistic: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
	}
}

// adds new user inventory item
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::AddInventoryItem(int userID, CUserInventoryItem& item)
{
	try
	{
		auto start = ExecCalcStart();

		if (item.m_nIsClanItem)
		{
			{
				SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND itemID = ? AND expiryDate = 0"));
				query.bind(1, userID);
				query.bind(2, item.m_nItemID);
				if (query.executeStep())
				{
					return -1; // user already has permanent item
				}
			}

			{
				SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND itemID = ? AND isClanItem = 1"));
				query.bind(1, userID);
				query.bind(2, item.m_nItemID);
				if (query.executeStep())
				{
					return -2; // user already has this clan item
				}
			}
		}

		int slot = -1;

		// find free slot
		SQLite::Statement queryGetFreeInvSlot(m_Database, OBFUSCATE("SELECT slot FROM UserInventory WHERE userID = ? AND itemID = 0"));
		queryGetFreeInvSlot.bind(1, userID);

		if (queryGetFreeInvSlot.executeStep())
		{
			slot = queryGetFreeInvSlot.getColumn(0);
		}
		else
		{
			SQLite::Statement queryGetNextInvSlot(m_Database, OBFUSCATE("SELECT nextInventorySlot FROM UserCharacterExtended WHERE userID = ?"));
			queryGetNextInvSlot.bind(1, userID);
			queryGetNextInvSlot.executeStep();
			slot = queryGetNextInvSlot.getColumn(0);

			SQLite::Statement queryUpdateNextInvSlot(m_Database, OBFUSCATE("UPDATE UserCharacterExtended SET nextInventorySlot = nextInventorySlot + 1 WHERE userID = ?"));
			queryUpdateNextInvSlot.bind(1, userID);
			queryUpdateNextInvSlot.exec();
		}

		SQLite::Statement queryGetInvItem(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND slot = ?"));
		queryGetInvItem.bind(1, userID);
		queryGetInvItem.bind(2, slot);

		item.m_nSlot = slot;

		if (!queryGetInvItem.executeStep())
		{
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
			queryInsertNewInvItem.bind(14, item.m_nPaintIDList);
			queryInsertNewInvItem.bind(15, item.m_nPartSlot1);
			queryInsertNewInvItem.bind(16, item.m_nPartSlot2);
			queryInsertNewInvItem.bind(17, item.m_nLockStatus);

			queryInsertNewInvItem.exec();
		}
		else
		{
			SQLite::Statement queryUpdateItem(m_Database, OBFUSCATE("UPDATE UserInventory SET itemID = ? , count = ? , status = ? , inUse = ? , obtainDate = ? , expiryDate = ? , isClanItem = ?, enhancementLevel = ?, enhancementExp = ?, enhanceValue = ?, paintID = ?, paintIDList = ?, partSlot1 = ?, partSlot2 = ?, lockStatus = ? WHERE userID = ? AND slot = ?"));
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
			queryUpdateItem.bind(12, item.m_nPaintIDList);
			queryUpdateItem.bind(13, item.m_nPartSlot1);
			queryUpdateItem.bind(14, item.m_nPartSlot2);
			queryUpdateItem.bind(15, item.m_nLockStatus);
			queryUpdateItem.bind(16, userID);
			queryUpdateItem.bind(17, slot);

			queryUpdateItem.exec();
		}

		ExecCalcEnd(start, OBFUSCATE("CUserDatabaseSQLite::AddInventoryItem"));
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::AddInventoryItem: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::AddInventoryItems(int userID, std::vector<CUserInventoryItem>& items)
{
	try
	{
		auto start = ExecCalcStart();

		SQLite::Transaction transaction(m_Database);

		for (auto& item : items)
		{
			if (item.m_nIsClanItem)
			{
				{
					SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND itemID = ? AND expiryDate = 0"));
					query.bind(1, userID);
					query.bind(2, item.m_nItemID);
					if (query.executeStep())
					{
						return -1; // user already has permanent item
					}
				}

				{
					SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND itemID = ? AND isClanItem = 1"));
					query.bind(1, userID);
					query.bind(2, item.m_nItemID);
					if (query.executeStep())
					{
						return -2; // user already has this clan item
					}
				}
			}

			int slot = -1;

			// find free slot
			SQLite::Statement queryGetFreeInvSlot(m_Database, OBFUSCATE("SELECT slot FROM UserInventory WHERE userID = ? AND itemID = 0"));
			queryGetFreeInvSlot.bind(1, userID);

			if (queryGetFreeInvSlot.executeStep())
			{
				slot = queryGetFreeInvSlot.getColumn(0);
			}
			else
			{
				SQLite::Statement queryGetNextInvSlot(m_Database, OBFUSCATE("SELECT nextInventorySlot FROM UserCharacterExtended WHERE userID = ?"));
				queryGetNextInvSlot.bind(1, userID);
				queryGetNextInvSlot.executeStep();
				slot = queryGetNextInvSlot.getColumn(0);

				SQLite::Statement queryUpdateNextInvSlot(m_Database, OBFUSCATE("UPDATE UserCharacterExtended SET nextInventorySlot = nextInventorySlot + 1 WHERE userID = ?"));
				queryUpdateNextInvSlot.bind(1, userID);
				queryUpdateNextInvSlot.exec();
			}

			SQLite::Statement queryGetInvItem(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND slot = ?"));
			queryGetInvItem.bind(1, userID);
			queryGetInvItem.bind(2, slot);

			item.m_nSlot = slot;

			if (!queryGetInvItem.executeStep())
			{
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
				queryInsertNewInvItem.bind(14, item.m_nPaintIDList);
				queryInsertNewInvItem.bind(15, item.m_nPartSlot1);
				queryInsertNewInvItem.bind(16, item.m_nPartSlot2);
				queryInsertNewInvItem.bind(17, item.m_nLockStatus);

				queryInsertNewInvItem.exec();
			}
			else
			{
				SQLite::Statement queryUpdateItem(m_Database, OBFUSCATE("UPDATE UserInventory SET itemID = ? , count = ? , status = ? , inUse = ? , obtainDate = ? , expiryDate = ? , isClanItem = ?, enhancementLevel = ?, enhancementExp = ?, enhanceValue = ?, paintID = ?, paintIDList = ?, partSlot1 = ?, partSlot2 = ?, lockStatus = ? WHERE userID = ? AND slot = ?"));
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
				queryUpdateItem.bind(12, item.m_nPaintIDList);
				queryUpdateItem.bind(13, item.m_nPartSlot1);
				queryUpdateItem.bind(14, item.m_nPartSlot2);
				queryUpdateItem.bind(15, item.m_nLockStatus);
				queryUpdateItem.bind(16, userID);
				queryUpdateItem.bind(17, slot);

				queryUpdateItem.exec();
			}
		}
	
		transaction.commit();

		ExecCalcEnd(start, OBFUSCATE("CUserDatabaseSQLite::AddInventoryItem"));
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::AddInventoryItem: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user inventory item data
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateInventoryItem(int userID, CUserInventoryItem item)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserInventory SET itemID = ? , count = ? , status = ? , inUse = ? , obtainDate = ? , expiryDate = ? , isClanItem = ?, enhancementLevel = ?, enhancementExp = ?, enhanceValue = ?, paintID = ?, paintIDList = ?, partSlot1 = ?, partSlot2 = ?, lockStatus = ? WHERE userID = ? AND slot = ?"));
		query.bind(1, item.m_nItemID);
		query.bind(2, item.m_nCount);
		query.bind(3, item.m_nStatus);
		query.bind(4, item.m_nInUse);
		query.bind(5, item.m_nObtainDate);
		query.bind(6, item.m_nExpiryDate);
		query.bind(7, item.m_nIsClanItem);
		query.bind(8, item.m_nEnhancementLevel);
		query.bind(9, item.m_nEnhancementExp);
		query.bind(10, item.m_nEnhanceValue);
		query.bind(11, item.m_nPaintID);
		query.bind(12, item.m_nPaintIDList);
		query.bind(13, item.m_nPartSlot1);
		query.bind(14, item.m_nPartSlot2);
		query.bind(15, item.m_nLockStatus);
		query.bind(16, userID);
		query.bind(17, item.m_nSlot);

		query.exec();
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateInventoryItem: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetInventoryItems: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user inventory items by itemID
// returns 0 == database error or no such item, 1 on success
int CUserDatabaseSQLite::GetInventoryItemsByID(int userID, int itemID, vector<CUserInventoryItem>& items)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND itemID = ?"));
		query.bind(1, userID);
		query.bind(2, itemID);

		if (!query.executeStep())
		{
			return 0;
		}

		CUserInventoryItem item = query.getColumns<CUserInventoryItem, 17>();
		items.push_back(item);
		while (query.executeStep())
		{
			CUserInventoryItem item = query.getColumns<CUserInventoryItem, 17>();
			items.push_back(item);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetInventoryItemByID: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user inventory item by slot
// returns 0 == database error or no such slot, 1 on success
int CUserDatabaseSQLite::GetInventoryItemBySlot(int userID, int slot, CUserInventoryItem& item)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE userID = ? AND slot = ?"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetInventoryItemBySlot: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets first active user inventory item by itemID
// returns 0 == database error or no such slot, 1 on success
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetFirstActiveItemByItemID: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// checks if user inventory is full
// returns 0 == inventory is not full, 1 == database error or inventory is full
int CUserDatabaseSQLite::IsInventoryFull(int userID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT COUNT(*) FROM UserInventory WHERE userID = ? AND itemID != 0"));
		query.bind(1, userID);
		query.executeStep();

		int itemsCount = query.getColumn(0);
		if (itemsCount >= g_pServerConfig->inventorySlotMax)
		{
			return 1;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::IsInventoryFull: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 1;
	}

	return 0;
}

// processes user inventory(finds expired items and sends user notification)
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::ProcessInventory(time_t curTime)
{
	try
	{
		static SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserInventory WHERE expiryDate != 0 AND inUse = 1 AND expiryDate < ?"));
		query.bind(1, curTime);	

		map<int, int> expiredItems;
		while (query.executeStep())
		{
			CUserInventoryItem item(query.getColumn(1), query.getColumn(2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", 0, 0, 0);
			int userID = query.getColumn(0);

			CUser* user = g_pUserManager->GetUserById(userID);
			if (user)
			{
				g_pPacketManager->SendUMsgExpiryNotice(user->GetExtendedSocket(), vector<int>{ item.m_nItemID });

				g_pItemManager->RemoveItem(userID, user, item);
			}
			else
			{
				// add to notice list
				g_pUserDatabase->UpdateExpiryNotices(userID, item.m_nItemID);

				// RemoveItem() should be used
				item.Reset();

				UpdateInventoryItem(userID, item);
			}
		}

		query.reset();
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::ProcessInventory: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

string GetUserDataString(int flag, bool update)
{
	ostringstream query;
	query << (update ? OBFUSCATE("UPDATE User SET ") : OBFUSCATE("SELECT "));
	if (flag & UDATA_FLAG_USERNAME)
		query << (update ? OBFUSCATE("userName = ?, ") : OBFUSCATE("userName, "));
	if (flag & UDATA_FLAG_PASSWORD)
		query << (update ? OBFUSCATE("password = ?, ") : OBFUSCATE("password, "));
	if (flag & UDATA_FLAG_REGISTERTIME)
		query << (update ? OBFUSCATE("registerTime = ?, ") : OBFUSCATE("registerTime, "));
	if (flag & UDATA_FLAG_REGISTERIP)
		query << (update ? OBFUSCATE("registerIP = ?, ") : OBFUSCATE("registerIP, "));
	if (flag & UDATA_FLAG_FIRSTLOGONTIME)
		query << (update ? OBFUSCATE("firstLogonTime = ?, ") : OBFUSCATE("firstLogonTime, "));
	if (flag & UDATA_FLAG_LASTLOGONTIME)
		query << (update ? OBFUSCATE("lastLogonTime = ?, ") : OBFUSCATE("lastLogonTime, "));
	if (flag & UDATA_FLAG_LASTIP)
		query << (update ? OBFUSCATE("lastIP = ?, ") : OBFUSCATE("lastIP, "));
	if (flag & UDATA_FLAG_LASTHWID)
		query << (update ? OBFUSCATE("lastHWID = ?, ") : OBFUSCATE("lastHWID, "));

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
		query[query.size() - 2] = ' ';
		query += OBFUSCATE("FROM User WHERE userID = ?"); // TODO: use stringstream

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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetUserData: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		query[query.size() - 2] = ' ';
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
			statement.bind(index++, hwid, data.lastHWID.size());
		}
		statement.bind(index++, userID);
		statement.exec();
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateUserData: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// creates user character
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::CreateCharacter(int userID, string gameName)
{
	try
	{
		auto start = ExecCalcStart();

		DefaultUser defUser = g_pServerConfig->defUser;

		SQLite::Transaction transcation(m_Database);
		SQLite::Statement insertCharacter(m_Database, OBFUSCATE("INSERT INTO UserCharacter VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
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
		insertCharacter.exec();

		SQLite::Statement insertCharacterExtended(m_Database, OBFUSCATE("INSERT INTO UserCharacterExtended VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
		insertCharacterExtended.bind(1, userID);
		insertCharacterExtended.bind(2, defUser.gameMaster);
		insertCharacterExtended.bind(3, 100); // gachapon kills
		insertCharacterExtended.bind(4, 1);
		insertCharacterExtended.bind(5, OBFUSCATE(""));
		insertCharacterExtended.bind(6, 0);
		insertCharacterExtended.bind(7, 0);
		insertCharacterExtended.bind(8, 2); // ban settings
		insertCharacterExtended.bind(9, ""); // 2nd password
		insertCharacterExtended.bind(10, 0); // security question
		insertCharacterExtended.bind(11, ""); // security answer
		insertCharacterExtended.exec();

		for (int i = 0; i < (int)defUser.loadouts.m_Loadouts.size(); i++)
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserLoadout VALUES (?, ?, ?, ?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, i);
			statement.bind(3, defUser.loadouts.m_Loadouts[i][0]);
			statement.bind(4, defUser.loadouts.m_Loadouts[i][1]);
			statement.bind(5, defUser.loadouts.m_Loadouts[i][2]);
			statement.bind(6, defUser.loadouts.m_Loadouts[i][3]);
			statement.exec();
		}

		for (int i = 0; i < (int)defUser.buyMenu.size(); i++)
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserBuyMenu VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
			int bindIndex = 1;
			statement.bind(bindIndex++, userID);
			statement.bind(bindIndex++, i);
			for (int k = 0; k < (int)defUser.buyMenu[i].items.size(); k++)
				statement.bind(bindIndex++, defUser.buyMenu[i].items[k]);
			statement.exec();
		}

		for (int i = 0; i < 5; i++)
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserFastBuy VALUES (?, ?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, i);
			statement.bind(3, va(OBFUSCATE("Favorites %d"), i + 1));
			statement.bind(4, OBFUSCATE("0,0,0,0,0,0,0,0,0,0,0"));
			statement.exec();
			statement.reset();
		}

		SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserRank VALUES (?, ?, ?, ?, ?)"));
		statement.bind(1, userID);
		statement.bind(2, 71); // No Record tier Original
		statement.bind(3, 71); // No Record tier Zombie
		statement.bind(4, 71); // No Record tier Zombie PVE
		statement.bind(5, 71); // No Record tier Death Match
		statement.exec();

		// init daily rewards
		/*UserDailyRewards dailyReward = {};
		dailyReward.canGetReward = true;
		g_pItemManager->UpdateDailyRewardsRandomItems(dailyReward);
		UpdateDailyRewards(userID, dailyReward);*/

		transcation.commit();

		ExecCalcEnd(start, OBFUSCATE("CUserDatabaseSQLite::CreateCharacter"));
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::CreateCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::DeleteCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user character
// returns -1 == character doesn't exist, 0 == database error, 1 on success
int CUserDatabaseSQLite::GetCharacter(int userID, CUserCharacter& character)
{
	try
	{
		// format query
		ostringstream query;
		query << OBFUSCATE("SELECT ");
		if (character.flag & UFLAG_GAMENAME)
		{
			query << OBFUSCATE("gameName, ");
		}
		if (character.flag & UFLAG_LEVEL)
		{
			query << OBFUSCATE("level, ");
		}
		if (character.flag & UFLAG_EXP)
		{
			query << OBFUSCATE("exp, ");
		}
		if (character.flag & UFLAG_CASH)
		{
			query << OBFUSCATE("cash, ");
		}
		if (character.flag & UFLAG_POINTS)
		{
			query << OBFUSCATE("points, ");
		}
		if (character.flag & UFLAG_STAT)
		{
			query << OBFUSCATE("battles, win, kills, deaths, ");
		}
		if (character.flag & UFLAG_LOCATION)
		{
			query << OBFUSCATE("nation, city, town, ");
		}
		if (character.flag & UFLAG_RANK)
		{
			query << OBFUSCATE("leagueID, ");
			// TODO: rewrite
			{
				SQLite::Statement query(m_Database, OBFUSCATE("SELECT tierOri, tierZM, tierZPVE, tierDM FROM UserRank WHERE userID = ?"));
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
		if (character.flag & UFLAG_PASSWORDBOXES)
		{
			query << OBFUSCATE("passwordBoxes, ");
		}
		if (character.flag & UFLAG_ACHIEVEMENT)
		{
			query << OBFUSCATE("honorPoints, prefixID, ");
		}
		if (character.flag & UFLAG_ACHIEVEMENTLIST)
		{
			query << OBFUSCATE("achievementList, ");
		}
		if (character.flag & UFLAG_TITLES)
		{
			query << OBFUSCATE("titles, ");
		}
		if (character.flag & UFLAG_CLAN)
		{
			query << OBFUSCATE("clanID, (SELECT markID FROM Clan WHERE clanID = UserCharacter.clanID), (SELECT name FROM Clan WHERE clanID = UserCharacter.clanID), ");
		}
		if (character.flag & UFLAG_TOURNAMENT)
		{
			query << OBFUSCATE("tournament, ");
		}
		if (character.flag & UFLAG_UNK19)
		{
			query << OBFUSCATE("mileagePoints, ");
		}
		query << OBFUSCATE("FROM UserCharacter WHERE userID = ?");

		string queryStr = query.str();
		queryStr[queryStr.size() - strlen(OBFUSCATE("FROM UserCharacter WHERE userID = ?")) - 2] = ' ';

		SQLite::Statement statement(m_Database, queryStr);
		statement.bind(1, userID);

		if (statement.executeStep())
		{
			int index = 0;
			if (character.flag & UFLAG_GAMENAME)
			{
				string gameName = statement.getColumn(index++);
				character.gameName = gameName;
			}
			if (character.flag & UFLAG_LEVEL)
			{
				character.level = statement.getColumn(index++);
			}
			if (character.flag & UFLAG_EXP)
			{
				character.exp = statement.getColumn(index++); // no uint64 support ;(
			}
			if (character.flag & UFLAG_CASH)
			{
				character.cash = statement.getColumn(index++);
			}
			if (character.flag & UFLAG_POINTS)
			{
				character.points = statement.getColumn(index++);
			}
			if (character.flag & UFLAG_STAT)
			{
				character.battles = statement.getColumn(index++);
				character.win = statement.getColumn(index++);
				character.kills = statement.getColumn(index++);
				character.deaths = statement.getColumn(index++);
			}
			if (character.flag & UFLAG_LOCATION)
			{
				character.nation = statement.getColumn(index++);
				character.city = statement.getColumn(index++);
				character.town = statement.getColumn(index++);
			}
			if (character.flag & UFLAG_RANK)
			{
				character.leagueID = statement.getColumn(index++);
			}
			if (character.flag & UFLAG_PASSWORDBOXES)
			{
				character.passwordBoxes = statement.getColumn(index++);
			}
			if (character.flag & UFLAG_ACHIEVEMENT)
			{
				character.honorPoints = statement.getColumn(index++);
				character.prefixId = statement.getColumn(index++);
			}
			if (character.flag & UFLAG_ACHIEVEMENTLIST)
			{
				character.achievementList = deserialize_array_int(statement.getColumn(index++));
			}
			if (character.flag & UFLAG_TITLES)
			{
				string serialized = statement.getColumn(index++);
				character.titles = deserialize_array_int(serialized);
			}
			if (character.flag & UFLAG_CLAN)
			{
				character.clanID = statement.getColumn(index++);
				character.clanMarkID = statement.getColumn(index++);
				character.clanName = (const char*)statement.getColumn(index++);
			}
			if (character.flag & UFLAG_TOURNAMENT)
			{
				character.tournament = statement.getColumn(index++);
			}
			if (character.flag & UFLAG_UNK19)
			{
				character.mileagePoints = statement.getColumn(index++);
			}
		}
		else
		{
			return -1;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		ostringstream query;
		query << OBFUSCATE("UPDATE UserCharacter SET ");
		if (character.flag & UFLAG_GAMENAME)
		{
			query << OBFUSCATE("gameName = ?, ");
		}
		if (character.flag & UFLAG_LEVEL)
		{
			query << OBFUSCATE("level = ?, ");
		}
		if (character.flag & UFLAG_EXP)
		{
			query << OBFUSCATE("exp = ?, ");
		}
		if (character.flag & UFLAG_CASH)
		{
			query << OBFUSCATE("cash = ?, ");
		}
		if (character.flag & UFLAG_POINTS)
		{
			query << OBFUSCATE("points = ?, ");
		}
		if (character.flag & UFLAG_STAT)
		{
			if (character.statFlag & 0x1)
			{
				query << OBFUSCATE("battles = ?, ");
			}
			if (character.statFlag & 0x2)
			{
				query << OBFUSCATE("win = ?, ");
			}
			if (character.statFlag & 0x4)
			{
				query << OBFUSCATE("kills = ?, ");
			}
			if (character.statFlag & 0x8)
			{
				query << OBFUSCATE("deaths = ?, ");
			}
		}
		if (character.flag & UFLAG_LOCATION)
		{
			query << OBFUSCATE("nation = ?, city = ?, town = ?, ");
		}
		if (character.flag & UFLAG_RANK)
		{
			query << OBFUSCATE("leagueID = ?, ");
		}
		if (character.flag & UFLAG_PASSWORDBOXES)
		{
			query << OBFUSCATE("passwordBoxes = ?, ");
		}
		if (character.flag & UFLAG_ACHIEVEMENT)
		{
			if (character.achievementFlag & 0x1)
			{
				query << OBFUSCATE("honorPoints = ?, ");
			}
			if (character.achievementFlag & 0x2)
			{
				query << OBFUSCATE("prefixID = ?, ");
			}
		}
		if (character.flag & UFLAG_ACHIEVEMENTLIST)
		{
			query << OBFUSCATE("achievementList = ?, ");
		}
		if (character.flag & UFLAG_TITLES)
		{
			query << OBFUSCATE("titles = ?, ");
		}
		if (character.flag & UFLAG_CLAN)
		{
			query << OBFUSCATE("clanID = ?, ");
		}
		if (character.flag & UFLAG_TOURNAMENT)
		{
			query << OBFUSCATE("tournament = ?, ");
		}
		if (character.flag & UFLAG_UNK19)
		{
			query << OBFUSCATE("mileagePoints = ?, ");
		}
		query << OBFUSCATE("WHERE userID = ?");

		string queryStr = query.str();
		queryStr[queryStr.size() - strlen(OBFUSCATE("WHERE userID = ?")) - 2] = ' ';

		SQLite::Statement statement(m_Database, queryStr);
		int index = 1;
		if (character.flag & UFLAG_GAMENAME)
		{
			statement.bind(index++, character.gameName);
		}
		if (character.flag & UFLAG_LEVEL)
		{
			statement.bind(index++, character.level);
		}
		if (character.flag & UFLAG_EXP)
		{
			statement.bind(index++, (int64_t)character.exp);
		}
		if (character.flag & UFLAG_CASH)
		{
			statement.bind(index++, character.cash);
		}
		if (character.flag & UFLAG_POINTS)
		{
			statement.bind(index++, character.points);
		}
		if (character.flag & UFLAG_STAT)
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
		if (character.flag & UFLAG_LOCATION)
		{
			statement.bind(index++, character.nation);
			statement.bind(index++, character.city);
			statement.bind(index++, character.town);
		}
		if (character.flag & UFLAG_RANK)
		{
			statement.bind(index++, character.leagueID);
		}
		if (character.flag & UFLAG_PASSWORDBOXES)
		{
			statement.bind(index++, character.passwordBoxes);
		}
		if (character.flag & UFLAG_ACHIEVEMENT)
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
		if (character.flag & UFLAG_ACHIEVEMENTLIST)
		{
			string serialized = serialize_array_int(character.achievementList);
			statement.bind(index++, serialized);
		}
		if (character.flag & UFLAG_TITLES)
		{
			string serialized = serialize_array_int(character.titles);
			statement.bind(index++, serialized);
		}
		if (character.flag & UFLAG_CLAN)
		{
			statement.bind(index++, character.clanID);
		}
		if (character.flag & UFLAG_TOURNAMENT)
		{
			statement.bind(index++, character.tournament);
		}
		if (character.flag & UFLAG_UNK19)
		{
			statement.bind(index++, character.mileagePoints);
		}

		statement.bind(index++, userID);

		statement.exec();
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user character extended
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetCharacterExtended(int userID, CUserCharacterExtended& character)
{
	try
	{
		// format query
		ostringstream query;
		query << OBFUSCATE("SELECT ");
		if (character.flag & EXT_UFLAG_GAMEMASTER)
		{
			query << OBFUSCATE("gameMaster, ");
		}
		if (character.flag & EXT_UFLAG_KILLSTOGETGACHAPONITEM)
		{
			query << OBFUSCATE("killsToGetGachaponItem, ");
		}
		if (character.flag & EXT_UFLAG_NEXTINVENTORYSLOT)
		{
			query << OBFUSCATE("nextInventorySlot, ");
		}
		if (character.flag & EXT_UFLAG_CONFIG)
		{
			query << OBFUSCATE("config, ");
		}
		if (character.flag & EXT_UFLAG_CURLOADOUT)
		{
			query << OBFUSCATE("curLoadout, ");
		}
		if (character.flag & EXT_UFLAG_CHARACTERID)
		{
			query << OBFUSCATE("characterID, ");
		}
		if (character.flag & EXT_UFLAG_BANSETTINGS)
		{
			query << OBFUSCATE("banSettings, ");
		}
		if (character.flag & EXT_UFLAG_2NDPASSWORD)
		{
			query << OBFUSCATE("_2ndPassword, ");
		}
		if (character.flag & EXT_UFLAG_SECURITYQNA)
		{
			query << OBFUSCATE("securityQuestion, securityAnswer, ");
		}
		query << OBFUSCATE("FROM UserCharacterExtended WHERE userID = ?");

		string queryStr = query.str();
		queryStr[queryStr.size() - strlen(OBFUSCATE("FROM UserCharacterExtended WHERE userID = ?")) - 2] = ' ';

		SQLite::Statement statement(m_Database, queryStr);
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
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetCharacterExtended: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		ostringstream query;
		query << OBFUSCATE("UPDATE UserCharacterExtended SET ");
		if (character.flag & EXT_UFLAG_GAMEMASTER)
		{
			query << OBFUSCATE("gameMaster = ?, ");
		}
		if (character.flag & EXT_UFLAG_KILLSTOGETGACHAPONITEM)
		{
			query << OBFUSCATE("killsToGetGachaponItem = ?, ");
		}
		if (character.flag & EXT_UFLAG_NEXTINVENTORYSLOT)
		{
			query << OBFUSCATE("nextInventorySlot = ?, ");
		}
		if (character.flag & EXT_UFLAG_CONFIG)
		{
			query << OBFUSCATE("config = ?, ");
		}
		if (character.flag & EXT_UFLAG_CURLOADOUT)
		{
			query << OBFUSCATE("curLoadout = ?, ");
		}
		if (character.flag & EXT_UFLAG_CHARACTERID)
		{
			query << OBFUSCATE("characterID = ?, ");
		}
		if (character.flag & EXT_UFLAG_BANSETTINGS)
		{
			query << OBFUSCATE("banSettings = ?, ");
		}
		if (character.flag & EXT_UFLAG_2NDPASSWORD)
		{
			query << OBFUSCATE("_2ndPassword = ?, ");
		}
		if (character.flag & EXT_UFLAG_SECURITYQNA)
		{
			query << OBFUSCATE("securityQuestion = ?, securityAnswer = ?, ");
		}
		query << OBFUSCATE("WHERE userID = ?");

		string queryStr = query.str();
		queryStr[queryStr.size() - strlen(OBFUSCATE("WHERE userID = ?")) - 2] = ' ';

		SQLite::Statement statement(m_Database, queryStr);
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
			statement.bind(index++, config, character.config.size());
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
			statement.bind(index++, _2ndPassword, character._2ndPassword.size());
		}
		if (character.flag & EXT_UFLAG_SECURITYQNA)
		{
			statement.bind(index++, character.securityQuestion);
			void* securityAnswer = character.securityAnswer.data();
			statement.bind(index++, securityAnswer, character.securityAnswer.size());
		}

		statement.bind(index++, userID);

		statement.exec();
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateCharacterExtended: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserBan WHERE userID = ?"));
		statement.bind(1, userID);

		if (statement.executeStep())
		{
			ban.banType = statement.getColumn(1);
			string reason = statement.getColumn(2);
			ban.reason = reason;
			ban.term = statement.getColumn(3);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetUserBan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
				//g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateUserBan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
				return 0;
			}
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateUserBan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets user loadout
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetLoadouts(int userID, CUserLoadout& loadout)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserLoadout WHERE userID = ?"));
		statement.bind(1, userID);

		while (statement.executeStep())
		{
			vector<int> ld;
			ld.push_back(statement.getColumn(2));
			ld.push_back(statement.getColumn(3));
			ld.push_back(statement.getColumn(4));
			ld.push_back(statement.getColumn(5));

			loadout.m_Loadouts.push_back(ld);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetLoadout: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user loadout
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateLoadout(int userID, int loadoutID, int slot, int value)
{
	try
	{
		string query;
		query = OBFUSCATE("UPDATE UserLoadout SET ");
		query += OBFUSCATE("slot") + to_string(slot) + OBFUSCATE(" = ? ");
		query += OBFUSCATE("WHERE userID = ? AND loadoutID = ?");

		SQLite::Statement statement(m_Database, query);
		statement.bind(1, value);
		statement.bind(2, userID);
		statement.bind(3, loadoutID);
		if (!statement.exec())
		{
			SQLite::Statement statement(m_Database, OBFUSCATE("INSERT INTO UserLoadout VALUES (?, ?, ?, ?, ?, ?)"));
			statement.bind(1, userID);
			statement.bind(2, loadoutID);
			statement.bind(3, slot == 0 ? value : 0);
			statement.bind(4, slot == 1 ? value : 0);
			statement.bind(5, slot == 2 ? value : 0);
			statement.bind(6, slot == 3 ? value : 0);
			statement.exec();
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateLoadout: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserFastBuy WHERE userID = ?"));
		statement.bind(1, userID);

		while (statement.executeStep())
		{
			fastBuy.push_back(CUserFastBuy(statement.getColumn(2), deserialize_array_int(statement.getColumn(3))));
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetFastBuy: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// updates user fast buy
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::UpdateFastBuy(int userID, int slot, string name, vector<int> items)
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateFastBuy: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserBuyMenu WHERE userID = ?"));
		statement.bind(1, userID);

		while (statement.executeStep())
		{
			vector<int> bm;
			for (int i = 2; i < 11; i++)
			{
				bm.push_back(statement.getColumn(i));
			}

			buyMenu.push_back(CUserBuyMenu(bm));
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetBuyMenu: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
			g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBuyMenu: UserBuyMenu is empty(userID: %d)???\n"), userID);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBuyMenu: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetBookmark(int userID, vector<int>& bookmark)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserBookmark WHERE userID = ?"));
		query.bind(1, userID);

		while (query.executeStep())
		{
			bookmark.push_back(query.getColumn(2));
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetBookmark: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateBookmark(int userID, int bookmarkID, int itemID)
{
	try
	{
		SQLite::Statement query(m_Database, "UPDATE UserBookmark SET itemID = ? WHERE userID = ? AND bookmarkID = ?");
		query.bind(1, itemID);
		query.bind(2, userID);
		query.bind(3, bookmarkID);
		if (!query.exec())
		{
			SQLite::Statement query(m_Database, "INSERT INTO UserBookmark VALUES (?, ?, ?)");
			query.bind(1, userID);
			query.bind(2, bookmarkID);
			query.bind(3, itemID);
			query.exec();
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBookmark: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserCostumeLoadout WHERE userID = ?"));
		query.bind(1, userID);
		if (query.executeStep())
		{
			loadout.m_nHeadCostumeID = query.getColumn(1);
			loadout.m_nBackCostumeID = query.getColumn(2);
			loadout.m_nArmCostumeID = query.getColumn(3);
			loadout.m_nPelvisCostumeID = query.getColumn(4);
			loadout.m_nFaceCostumeID = query.getColumn(5);
			loadout.m_nTattooID = query.getColumn(6);
			loadout.m_nPetCostumeID = query.getColumn(7);
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserZBCostumeLoadout WHERE userID = ?"));
			query.bind(1, userID);
			while (query.executeStep())
			{
				int slot = query.getColumn(1);
				int itemID = query.getColumn(2);

				loadout.m_ZombieSkinCostumeID[slot] = itemID;
			}
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetCostumeLoadout: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
				SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserZBCostumeLoadout SET slot = ?, itemID = ? WHERE userID = ?"));
				query.bind(1, zbSlot);
				query.bind(2, loadout.m_ZombieSkinCostumeID[zbSlot]);
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateCostumeLoadout: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserRewardNotice WHERE userID = ?"));
		statement.bind(1, userID);

		while (statement.executeStep())
		{
			notices.push_back(statement.getColumn(1));
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetRewardNotices: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateRewardNotices: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetExpiryNotices(int userID, vector<int>& notices)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserExpiryNotice WHERE userID = ?"));
		statement.bind(1, userID);

		while (statement.executeStep())
		{
			notices.push_back(statement.getColumn(1));
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetExpiryNotices: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateRewardNotices: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetDailyRewards(int userID, UserDailyRewards& dailyRewards)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT day, canGetReward FROM UserDailyReward WHERE userID = ?"));
		statement.bind(1, userID);

		if (statement.executeStep())
		{
			dailyRewards.day = statement.getColumn(0);
			dailyRewards.canGetReward = (char)statement.getColumn(1);
		}

		SQLite::Statement statement_getItems(m_Database, OBFUSCATE("SELECT * FROM UserDailyRewardItems WHERE userID = ?"));
		statement_getItems.bind(1, userID);

		while (statement_getItems.executeStep())
		{
			RewardItem item;
			item.itemID = statement_getItems.getColumn(1);
			item.count = statement_getItems.getColumn(2);
			item.duration = statement_getItems.getColumn(3);
			item.eventFlag = statement_getItems.getColumn(4);

			dailyRewards.randomItems.push_back(item);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetDailyRewards: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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

		SQLite::Statement statement_updateItems(m_Database, OBFUSCATE("SELECT * FROM UserDailyRewardItems WHERE userID = ?"));
		statement_updateItems.bind(1, userID);

		if (statement_updateItems.executeStep())
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
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateDailyRewards: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetQuestsProgress(int userID, vector<UserQuestProgress>& questsProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserQuestProgress WHERE userID = ?"));
		statement.bind(1, userID);
		while (statement.executeStep())
		{
			UserQuestProgress progress;
			progress.questID = statement.getColumn(1);
			progress.status = statement.getColumn(2);
			progress.favourite = (char)statement.getColumn(3);
			progress.started = (char)statement.getColumn(4);

			questsProgress.push_back(progress);
		}

		{
			SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserQuestTaskProgress WHERE userID = ?"));
			statement.bind(1, userID);
			while (statement.executeStep())
			{
				UserQuestTaskProgress progress;
				int questID = statement.getColumn(1);
				progress.taskID = statement.getColumn(2);
				progress.unitsDone = statement.getColumn(3);
				progress.taskVar = statement.getColumn(4);

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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetQuestProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetQuestProgress(int userID, int questID, UserQuestProgress& questProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserQuestProgress WHERE userID = ? AND questID = ?"));
		statement.bind(1, userID);
		statement.bind(2, questID);
		if (statement.executeStep())
		{
			questProgress.questID = statement.getColumn(1);
			questProgress.status = statement.getColumn(2);
			questProgress.favourite = (char)statement.getColumn(3);
			questProgress.started = (char)statement.getColumn(4);
		}
		else
		{
			questProgress.questID = questID;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetQuestProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateQuestProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetQuestTaskProgress(int userID, int questID, int taskID, UserQuestTaskProgress& taskProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserQuestTaskProgress WHERE userID = ? AND questID = ? AND taskID = ?"));
		statement.bind(1, userID);
		statement.bind(2, questID);
		statement.bind(3, taskID);
		if (statement.executeStep())
		{
			taskProgress.taskID = statement.getColumn(2);
			taskProgress.unitsDone = statement.getColumn(3);
			taskProgress.taskVar = statement.getColumn(4);
			taskProgress.finished = (char)statement.getColumn(5);
		}
		else
		{
			taskProgress.taskID = taskID;
			//return 0;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetQuestTaskProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateQuestTaskProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

bool CUserDatabaseSQLite::IsQuestTaskFinished(int userID, int questID, int taskID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserQuestTaskProgress WHERE userID = ? AND questID = ? AND taskID = ? AND finished = 0"));
		query.bind(1, userID);
		query.bind(2, questID);
		query.bind(3, taskID);
		if (query.executeStep())
		{
			return false;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::IsAllQuestTasksFinished: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return true;
}

int CUserDatabaseSQLite::GetQuestStat(int userID, int flag, UserQuestStat& stat)
{
	try
	{
		string query = OBFUSCATE("SELECT ");

		if (flag & 0x2)
		{
			query += OBFUSCATE("continiousSpecialQuest, ");
		}
		if (flag & 0x8)
		{
			query += OBFUSCATE("dailyMissionsCompletedToday, ");
		}
		if (flag & 0x20)
		{
			query += OBFUSCATE("dailyMissionsCleared, ");
		}
		query[query.size() - 2] = ' ';
		query += OBFUSCATE("FROM UserQuestStat WHERE userID = ?");

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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetQuestStat: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateQuestStat(int userID, int flag, UserQuestStat& stat)
{
	try
	{
		// format query
		string query = OBFUSCATE("UPDATE UserQuestStat SET ");

		if (flag & 0x2)
		{
			query += OBFUSCATE("continiousSpecialQuest = ?, ");
		}
		if (flag & 0x8)
		{
			query += OBFUSCATE("dailyMissionsCompletedToday = ?, ");
		}
		if (flag & 0x20)
		{
			query += OBFUSCATE("dailyMissionsCleared = ?, ");
		}

		query[query.size() - 2] = ' ';
		query += OBFUSCATE("WHERE userID = ?");

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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateQuestStat: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetBingoProgress(int userID, UserBingo& bingo)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserMiniGameBingo WHERE userID = ?"));
		statement.bind(1, userID);
		if (statement.executeStep())
		{
			bingo.status = statement.getColumn(1);
			bingo.canPlay = (char)statement.getColumn(2);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetBingoProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBingoProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetBingoSlot(int userID, vector<UserBingoSlot>& slots)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserMiniGameBingoSlot WHERE userID = ?"));
		statement.bind(1, userID);
		while (statement.executeStep())
		{
			UserBingoSlot slot;
			slot.number = statement.getColumn(1);
			slot.opened = (char)statement.getColumn(2);

			slots.push_back(slot);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetBingoSlot: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
			statement.executeStep();
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBingoSlot: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetBingoPrizeSlot(int userID, vector<UserBingoPrizeSlot>& prizes)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserMiniGameBingoPrizeSlot WHERE userID = ?"));
		statement.bind(1, userID);
		while (statement.executeStep())
		{
			UserBingoPrizeSlot slot;
			slot.index = statement.getColumn(1);
			slot.opened = (char)statement.getColumn(2);
			slot.item.itemID = statement.getColumn(3);
			slot.item.count = statement.getColumn(4);
			slot.item.duration = statement.getColumn(5);
			slot.bingoIndexes = deserialize_array_int(statement.getColumn(6));

			prizes.push_back(slot);
		}
		statement.reset();
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetBingoPrizeSlot: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBingoPrizeSlot: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserRank WHERE userID = ?"));
		statement.bind(1, userID);

		if (statement.executeStep())
		{
			for (int i = 0; i < 4; i++)
				character.tier[i] = statement.getColumn(i+1);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetUserRank: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateUserRank: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// gets ban list
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::GetBanList(int userID, vector<UserBanList>& banList)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT gameName, (SELECT NOT EXISTS(SELECT 1 FROM UserCharacter WHERE gameName = UserBanList.gameName)) FROM UserBanList WHERE userID = ?"));
		query.bind(1, userID);
		while (query.executeStep())
		{
			UserBanList ban;
			ban.gameName = (const char*)query.getColumn(0);
			ban.isNotExists = (char)query.getColumn(1);
			banList.push_back(ban);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
				SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM UserCharacter WHERE gameName = ?)"));
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
				SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM UserBanList WHERE userID = ? AND gameName = ?)"));
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
				SQLite::Statement query(m_Database, OBFUSCATE("SELECT COUNT(*) FROM UserBanList WHERE userID = ?"));
				query.bind(1, userID);
				if (query.executeStep())
				{
					if ((int)query.getColumn(0) >= 30)
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT NOT EXISTS(SELECT 1 FROM UserBanList WHERE userID = ? AND gameName = (SELECT gameName FROM UserCharacter WHERE userID = ?))"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::IsInBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return false;
	}

	return true;
}

bool CUserDatabaseSQLite::IsSurveyAnswered(int userID, int surveyID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM UserSurveyAnswer WHERE userID = ? AND surveyID = ?)"));
		query.bind(1, userID);
		query.bind(2, surveyID);
		if (query.executeStep())
		{
			return (char)query.getColumn(0);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::IsSurveyAnswered: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::SurveyAnswer: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetWeaponReleaseProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetWeaponReleaseRow(int userID, UserWeaponReleaseRow& row)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT character, opened FROM UserMiniGameWeaponReleaseItemProgress WHERE userID = ? AND slot = ?"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetWeaponReleaseRow: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateWeaponReleaseRow(int userID, UserWeaponReleaseRow& row)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserMiniGameWeaponReleaseItemProgress SET slot = ?, character = ?, opened = ? WHERE userID = ?"));
		query.bind(1, userID);
		query.bind(2, row.id);
		query.bind(3, row.progress);
		query.bind(4, row.opened);
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateWeaponReleaseRow: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetWeaponReleaseCharacters: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetWeaponReleaseCharacter(int userID, UserWeaponReleaseCharacter& character)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT count FROM UserMiniGameWeaponReleaseCharacters WHERE userID = ? AND character = ?"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetWeaponReleaseCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateWeaponReleaseCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateWeaponReleaseCharacter: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM Clan WHERE name = ?"));
			query.bind(1, clanCfg.name);
			if (query.executeStep())
			{
				return -1;
			}
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ?"));
			query.bind(1, clanCfg.masterUserID);
			if (query.executeStep())
			{
				return -2;
			}
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMemberRequest WHERE userID = ?"));
			query.bind(1, clanCfg.masterUserID);
			if (query.executeStep())
			{
				return -2;
			}
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT points FROM UserCharacter WHERE userID = ?"));
			query.bind(1, clanCfg.masterUserID);
			if (query.executeStep())
			{
				int points = query.getColumn(0);
				if (points < 100000) // 100k points
				{
					return -3;
				}
			}
		}

		SQLite::Transaction transaction(m_Database);
		{
			CUser* user = g_pUserManager->GetUserById(clanCfg.masterUserID);
			if (!user->UpdatePoints(-100000))
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
		character.flag = UFLAG_GAMENAME;
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::CreateClan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return clanID;
}

int CUserDatabaseSQLite::JoinClan(int userID, int clanID, string& clanName)
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND clanID = ?"));
			query.bind(1, userID);
			query.bind(2, clanID);
			if (query.executeStep())
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
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT maxMemberCount, memberCount FROM Clan WHERE clanID = ?"));
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
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT joinMethod FROM Clan WHERE clanID = ?"));
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
						SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMemberRequest WHERE userID = ?"));
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
								SQLite::Statement query(m_Database, OBFUSCATE("SELECT name FROM Clan WHERE clanID = ?"));
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
						SQLite::Statement query(m_Database, OBFUSCATE("SELECT userID FROM ClanInvite WHERE destUserID = ? AND clanID = ?"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::JoinClan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::CancelJoin: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade != 0"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::LeaveClan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::DissolveClan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
			static SQLite::Statement query(m_Database, OBFUSCATE("SELECT COUNT(*) / 15 + 1 FROM Clan"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetClanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanInfo(int clanID, Clan_s& clan)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT Clan.clanID, gameName, name, notice, gameModeID, mapID, time, memberCount, expBonus, pointBonus, markID, maxMemberCount FROM Clan, UserCharacter WHERE userID = Clan.masterUserID AND Clan.clanID = ?"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetClanInfo: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= (SELECT accessGrade FROM ClanStoragePage WHERE clanID = ClanMember.clanID AND pageID = ?)"));
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
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT slot FROM ClanStorageItem WHERE clanID = ? AND pageID = ? AND itemID = 0"));
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
		if (UpdateInventoryItem(userID, item) <= 0)
		{
			return -2;
		}

		transaction.commit();
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::AddClanStorageItem: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= (SELECT accessGrade FROM ClanStoragePage WHERE clanID = ClanMember.clanID AND pageID = ?)"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::DeleteClanStorageItem: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT itemID, itemCount FROM ClanStorageItem INNER JOIN ClanStoragePage ON ClanStorageItem.clanID = ClanStoragePage.clanID AND ClanStoragePage.pageID = ClanStorageItem.pageID INNER JOIN ClanMember ON ClanMember.userID = ? WHERE ClanMember.memberGrade <= ClanStoragePage.accessGrade AND ClanStorageItem.clanID = ClanMember.clanID AND ClanStorageItem.pageID = ? AND ClanStorageItem.slot = ? AND ClanStorageItem.itemID != 0"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetClanStorageItem: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanStoragePage(int userID, ClanStoragePage& clanStoragePage)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM ClanStorageItem WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ?) AND pageID = ? AND itemID != 0"));
		statement.bind(1, userID);
		statement.bind(2, clanStoragePage.pageID);
		while (statement.executeStep())
		{
			RewardItem item;
			item.selectID = statement.getColumn(2);
			item.itemID = statement.getColumn(3);
			item.count = statement.getColumn(4);
			item.duration = statement.getColumn(5);
			item.enhValue = statement.getColumn(6);

			clanStoragePage.items.push_back(item);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetClanStoragePage: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT accessGrade FROM ClanStoragePage WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ?)"));
		query.bind(1, userID);
		while (query.executeStep())
		{
			accessGrade.push_back(query.getColumn(0));
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetClanStorageAccessGrade: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateClanStorageAccessGrade(int userID, int pageID, int accessGrade)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("UPDATE ClanStoragePage SET accessGrade = ? WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ?) AND pageID = ? AND (SELECT memberGrade FROM ClanMember WHERE userID = 1) <= 1"));
		statement.bind(1, accessGrade);
		statement.bind(2, userID);
		statement.bind(3, pageID);
		if (!statement.exec())
		{
			return 0;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateClanStorageAccessGrade: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
			strQuery = OBFUSCATE("SELECT ClanMember.userID, gameName, userName, memberGrade FROM ClanMember INNER JOIN UserCharacter ON UserCharacter.userID = ClanMember.userID AND ClanMember.clanID = (SELECT clanID FROM UserCharacter WHERE userID = ?) INNER JOIN User ON UserCharacter.userID = User.userID");
		}

		SQLite::Statement query(m_Database, strQuery);
		query.bind(1, id);
		while (query.executeStep())
		{
			ClanUser clanUser = {};
			clanUser.userID = query.getColumn(0);
			clanUser.character.gameName = (const char*)query.getColumn(1);
			clanUser.userName = (const char*)query.getColumn(2);
			clanUser.user = g_pUserManager->GetUserById(clanUser.userID);
			clanUser.memberGrade = query.getColumn(3);

			users.push_back(clanUser);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetClanUserList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanMemberList(int userID, vector<ClanUser>& users)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT ClanMember.userID, memberGrade, gameName, userName, level, kills, deaths FROM ClanMember INNER JOIN UserCharacter ON ClanMember.userID = UserCharacter.userID INNER JOIN User ON UserCharacter.userID = User.userID WHERE (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1) = ClanMember.clanID"));
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
			clanUser.user = g_pUserManager->GetUserById(clanUser.userID);

			users.push_back(clanUser);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetClanMemberList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanMemberJoinUserList(int userID, vector<ClanUserJoinRequest>& users)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT ClanMemberRequest.userID, userName, gameName, level, kills, deaths, (SELECT gameName FROM UserCharacter WHERE userID = inviterUserID), date FROM ClanMemberRequest INNER JOIN UserCharacter ON ClanMemberRequest.userID = UserCharacter.userID INNER JOIN ClanMember ON ClanMember.userID = ? AND ClanMember.memberGrade <= 1 AND ClanMember.clanID = ClanMemberRequest.clanID INNER JOIN User ON User.userID = ClanMemberRequest.userID"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetClanMemberJoinUserList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClan(int userID, int flag, Clan_s& clan)
{
	try
	{
		// format query
		ostringstream query;
		query << OBFUSCATE("SELECT ");
		if (flag & CFLAG_ID)
		{
			query << OBFUSCATE("clanID, ");
		}
		if (flag & CFLAG_NAME)
		{
			query << OBFUSCATE("name, ");
		}
		if (flag & CFLAG_MASTERUID)
		{
			query << OBFUSCATE("masterUserID, ");
		}
		if (flag & CFLAG_CLANMASTER)
		{
			query << OBFUSCATE("(SELECT gameName FROM UserCharacter WHERE userID = Clan.masterUserID), ");
		}
		if (flag & CFLAG_TIME)
		{
			query << OBFUSCATE("time, ");
		}
		if (flag & CFLAG_GAMEMODEID)
		{
			query << OBFUSCATE("gameModeID, ");
		}
		if (flag & CFLAG_MAPID)
		{
			query << OBFUSCATE("mapID, ");
		}
		if (flag & CFLAG_REGION)
		{
			query << OBFUSCATE("region, ");
		}
		if (flag & CFLAG_JOINMETHOD)
		{
			query << OBFUSCATE("joinMethod, ");
		}
		if (flag & CFLAG_EXPBOOST)
		{
			query << OBFUSCATE("expBonus, ");
		}
		if (flag & CFLAG_POINTBOOST)
		{
			query << OBFUSCATE("pointBonus, ");
		}
		if (flag & CFLAG_NOTICEMSG)
		{
			query << OBFUSCATE("notice, ");
		}
		if (flag & CFLAG_SCORE)
		{
			query << OBFUSCATE("score, ");
		}
		if (flag & CFLAG_MARKID)
		{
			query << OBFUSCATE("markID, ");
		}
		if (flag & CFLAG_MARKCOLOR)
		{
			query << OBFUSCATE("markColor, ");
		}
		if (flag & CFLAG_MARKCHANGECOUNT)
		{
			query << OBFUSCATE("markChangeCount, ");
		}
		if (flag & CFLAG_MAXMEMBERCOUNT)
		{
			query << OBFUSCATE("maxMemberCount, ");
		}
		if (flag & CFLAG_CHRONICLE)
		{
			// TODO: rewrite
			{
				SQLite::Statement query(m_Database, OBFUSCATE("SELECT date, type, string FROM ClanChronicle WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ?)"));
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

			query << OBFUSCATE("0, ");
		}
		query << OBFUSCATE("FROM Clan WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ?)");

		string queryStr = query.str();
		queryStr[queryStr.size() - strlen(OBFUSCATE("FROM Clan WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ?)")) - 2] = ' ';

		SQLite::Statement statement(m_Database, queryStr.c_str());
		statement.bind(1, userID);
		if (statement.executeStep())
		{
			int index = 0;
			if (flag & CFLAG_ID)
			{
				clan.id = statement.getColumn(index++);
			}
			if (flag & CFLAG_NAME)
			{
				clan.name = (const char*)statement.getColumn(index++);
			}
			if (flag & CFLAG_MASTERUID)
			{
				clan.masterUserID = statement.getColumn(index++);
			}
			if (flag & CFLAG_CLANMASTER)
			{
				clan.clanMaster = (const char*)statement.getColumn(index++);
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
			if (flag & CFLAG_MARKCHANGECOUNT)
			{
				clan.markChangeCount = statement.getColumn(index++);
			}
			if (flag & CFLAG_MAXMEMBERCOUNT)
			{
				clan.maxMemberCount = statement.getColumn(index++);
			}
		}
		else
		{
			return -1;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetClan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetClanMember(int userID, ClanUser& clanUser)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT ClanMember.userID, memberGrade, gameName, userName, level, kills, deaths FROM ClanMember INNER JOIN UserCharacter ON UserCharacter.userID = ClanMember.userID AND ClanMember.clanID = (SELECT clanID FROM UserCharacter WHERE userID = ?) INNER JOIN User ON User.userID = UserCharacter.userID"));
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
			clanUser.user = g_pUserManager->GetUserById(clanUser.userID);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetClanMember: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateClan(int userID, int flag, Clan_s clan)
{
	try
	{
		// format query
		ostringstream query;
		query << OBFUSCATE("UPDATE Clan SET ");
		if (flag & CFLAG_NAME)
		{
			query << OBFUSCATE("name = ?, ");
		}
		if (flag & CFLAG_MASTERUID)
		{
			query << OBFUSCATE("masterUserID = ?, ");
		}
		if (flag & CFLAG_TIME)
		{
			query << OBFUSCATE("time = ?, ");
		}
		if (flag & CFLAG_GAMEMODEID)
		{
			query << OBFUSCATE("gameModeID = ?, ");
		}
		if (flag & CFLAG_MAPID)
		{
			query << OBFUSCATE("mapID = ?, ");
		}
		if (flag & CFLAG_REGION)
		{
			query << OBFUSCATE("region = ?, ");
		}
		if (flag & CFLAG_JOINMETHOD)
		{
			query << OBFUSCATE("joinMethod = ?, ");
		}
		if (flag & CFLAG_EXPBOOST)
		{
			query << OBFUSCATE("expBonus = ?, ");
		}
		if (flag & CFLAG_POINTBOOST)
		{
			query << OBFUSCATE("pointBonus = ?, ");
		}
		if (flag & CFLAG_NOTICEMSG)
		{
			query << OBFUSCATE("notice = ?, ");
		}
		if (flag & CFLAG_SCORE)
		{
			query << OBFUSCATE("score = ?, ");
		}
		if (flag & CFLAG_MARKID)
		{
			query << OBFUSCATE("markID = ?, ");
		}
		if (flag & CFLAG_MARKCOLOR)
		{
			query << OBFUSCATE("markColor = ?, ");
		}
		if (flag & CFLAG_MARKCHANGECOUNT)
		{
			query << OBFUSCATE("markChangeCount = ?, ");
		}
		if (flag & CFLAG_MAXMEMBERCOUNT)
		{
			query << OBFUSCATE("maxMemberCount = ?, ");
		}
		query << OBFUSCATE("WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1)");

		string queryStr = query.str();
		queryStr[queryStr.size() - strlen(OBFUSCATE("WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1)")) - 2] = ' ';

		SQLite::Statement statement(m_Database, queryStr);
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateClan: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateClanMemberGrade(int userID, string targetUserName, int newGrade, ClanUser& targetMember)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("UPDATE ClanMember SET memberGrade = ? WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade = 0) AND userID = (SELECT userID FROM User WHERE userName = ?)"));
		query.bind(1, newGrade);
		query.bind(2, userID);
		query.bind(3, targetUserName);
		if (!query.exec())
		{
			return -1; // not clan master
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT ClanMember.userID, memberGrade, gameName, userName, level, kills, deaths FROM ClanMember INNER JOIN UserCharacter ON UserCharacter.userID = ClanMember.userID INNER JOIN User ON User.userID = UserCharacter.userID AND User.userName = ?"));
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
				targetMember.user = g_pUserManager->GetUserByUsername(targetUserName);
			}
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateClanMemberGrade: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::ClanReject(int userID, string userName)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM ClanMemberRequest WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1) AND userID = (SELECT userID FROM User WHERE userName = ?)"));
		query.bind(1, userID);
		query.bind(2, userName);
		if (!query.exec())
		{
			return -1; // not clan master
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::ClanReject: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::ClanRejectAll(int userID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM ClanMemberRequest WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade = 0)"));
		query.bind(1, userID);
		if (!query.exec())
		{
			return -1; // not clan master
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::ClanRejectAll: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::ClanApprove(int userID, string userName)
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM ClanMemberRequest WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1) AND userID = (SELECT userID FROM User WHERE userName = ?)"));
			query.bind(1, userID);
			query.bind(2, userName);
			if (!query.exec())
			{
				return -1; // not clan master
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO ClanMember VALUES ((SELECT clanID FROM ClanMember WHERE userID = ?), (SELECT userID FROM User WHERE userName = ?), ?)"));
			query.bind(1, userID);
			query.bind(2, userName);
			query.bind(3, 3);
			if (!query.exec())
			{
				return -1;
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE Clan SET memberCount = memberCount + 1 WHERE clanID = (SELECT clanID FROM ClanMember WHERE userID = ?)"));
			query.bind(1, userID);
			if (!query.exec())
			{
				return 0;
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserCharacter SET clanID = (SELECT clanID FROM ClanMember WHERE userID = ?) WHERE userID = (SELECT userID FROM User WHERE userName = ?)"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::ClanApprove: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::IsClanWithMarkExists(int markID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM Clan WHERE markID = ?"));
		query.bind(1, markID);
		if (!query.executeStep())
		{
			return -1;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::IsClanWithMarkExists: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::ClanInvite(int userID, string gameName, CUser*& destUser, int& clanID)
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1"));
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
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT userID FROM UserCharacter WHERE gameName = ?"));
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

		destUser = g_pUserManager->GetUserById(destUserID);
		if (!destUser)
		{
			return -2; // user is offline(TODO: are there more conditions?)
		}

		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID, memberGrade FROM ClanMember WHERE userID = ?"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::ClanInvite: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::ClanKick(int userID, string userName)
{
	try
	{
		int clanID = 0;
		{
			// TODO: rewrite query (family master can't kick admins)
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM ClanMember WHERE userID = ? AND memberGrade <= 1"));
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
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT userID FROM ClanMember WHERE userID = (SELECT userID FROM User WHERE userName = ?) AND memberGrade > (SELECT memberGrade FROM ClanMember WHERE userID = ?)"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::ClanKick: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::ClanMasterDelegate(int userID, string userName)
{
	try
	{
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE ClanMember SET memberGrade = 0 WHERE userID = (SELECT userID FROM User WHERE userName = ?) AND memberGrade = 1 AND (SELECT memberGrade FROM ClanMember WHERE userID = ?) = 0"));
			query.bind(1, userName);
			query.bind(2, userID);
			if (!query.exec())
			{
				return -1;
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE ClanMember SET memberGrade = 0 WHERE userID = ?"));
			query.bind(1, userID);
			if (!query.exec())
			{
				return 0;
			}
		}
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO ClanChronicle VALUES ((SELECT clanID FROM ClanMember WHERE userID = ?), strftime('%Y%m%d', datetime(?, 'unixepoch')), ?, ?)"));
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::ClanMasterDelegate: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::IsClanExists(string clanName)
{
	int clanID = 0;
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT clanID FROM Clan WHERE name = ?"));
		query.bind(1, clanName);
		if (query.executeStep())
		{
			clanID = query.getColumn(0);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::IsClanExists: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return clanID;
}


int CUserDatabaseSQLite::GetQuestEventProgress(int userID, int questID, UserQuestProgress& questProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserQuestEventProgress WHERE userID = ? AND questID = ?"));
		statement.bind(1, userID);
		statement.bind(2, questID);
		if (statement.executeStep())
		{
			questProgress.questID = statement.getColumn(1);
			questProgress.status = statement.getColumn(2);
			questProgress.favourite = (char)statement.getColumn(3);
			questProgress.started = (char)statement.getColumn(4);
		}
		else
		{
			questProgress.questID = questID;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetQuestEventProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateQuestEventProgress(int userID, UserQuestProgress questProgress)
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateQuestEventProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::GetQuestEventTaskProgress(int userID, int questID, int taskID, UserQuestTaskProgress& taskProgress)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT * FROM UserQuestEventTaskProgress WHERE userID = ? AND questID = ? AND taskID = ?"));
		statement.bind(1, userID);
		statement.bind(2, questID);
		statement.bind(3, taskID);
		if (statement.executeStep())
		{
			taskProgress.taskID = statement.getColumn(2);
			taskProgress.unitsDone = statement.getColumn(3);
			taskProgress.taskVar = statement.getColumn(4);
			taskProgress.finished = (char)statement.getColumn(5);
		}
		else
		{
			taskProgress.taskID = taskID;
			//return 0;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetQuestEventTaskProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateQuestEventTaskProgress(int userID, int questID, UserQuestTaskProgress taskProgress)
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateQuestEventTaskProgress: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

bool CUserDatabaseSQLite::IsQuestEventTaskFinished(int userID, int questID, int taskID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT * FROM UserQuestEventTaskProgress WHERE userID = ? AND questID = ? AND taskID = ? AND finished = 1"));
		query.bind(1, userID);
		query.bind(2, questID);
		query.bind(3, taskID);
		if (!query.executeStep())
		{
			return false;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::IsQuestEventTaskFinished: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return true;
}

// checks if user exists
// returns 0 == database error, 1 == user exists
int CUserDatabaseSQLite::IsUserExists(int userID)
{
	g_pConsole->Log(OBFUSCATE("CUserDatabaseSQLite::IsUserExists: need to test!!!\n"));

	int retVal = 0;
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM User WHERE userID = ?)"));
		statement.bind(1, userID);
		if (statement.executeStep())
		{
			return statement.getColumn(0);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::IsUserExists: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return retVal;
}

// checks if user exists
// returns 0 == database error or user does not exists, > 0 == userID
int CUserDatabaseSQLite::IsUserExists(string userName, bool searchByUserName)
{
	int userID = 0;
	try
	{
		string query = searchByUserName ? OBFUSCATE("SELECT userID FROM User WHERE userName = ?") : OBFUSCATE("SELECT userID FROM UserCharacter WHERE gameName = ?");

		SQLite::Statement statement(m_Database, query);
		statement.bind(1, userName);
		if (statement.executeStep())
		{
			userID = statement.getColumn(0);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::IsUserExists: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return userID;
}

#ifndef PUBLIC_RELEASE
// adds user suspect action to suspect actions list
// actionID(0 - DLLMOD, 1 - old client build, 2 - more than 2 accounts with the same HWID)
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::SuspectAddAction(vector<unsigned char>& hwid, int actionID)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("INSERT INTO SuspectAction VALUES (?, ?, ?)"));
		query.bind(1, hwid.data(), hwid.size());
		query.bind(2, actionID);
		query.bind(3, g_pServerInstance->GetCurrentTime());
		query.exec();
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::SuspectAddAction: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT EXISTS(SELECT 1 FROM SuspectAction WHERE hwid = (SELECT lastHWID FROM User WHERE userID = ?))"));
		query.bind(1, userID);
		if (query.executeStep())
		{
			return query.getColumn(0);
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::IsUserSuspect: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 0;
}
#endif

// processes user database every minute
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::OnMinuteTick(time_t curTime)
{
	try
	{
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::OnMinuteTick: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

// processes user database every day
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::OnDayTick()
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
			SQLite::Statement query(m_Database, OBFUSCATE("UPDATE UserQuestProgress SET status = 0 WHERE questID > 0 AND questID < 2000")); // why not DELETE?
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

		g_pConsole->Log(OBFUSCATE("CUserDatabaseSQLite::OnDayTick: daily quest: %d, daily event quest: %d\n"), dailyQuests, dailyEventQuests);

		// get affected users and send quest update
		{
			SQLite::Statement query(m_Database, OBFUSCATE("SELECT userID FROM UserSession WHERE (? - (SELECT lastLogonTime FROM User)) < 1440"));
			query.bind(1, g_pServerInstance->GetCurrentTime());
			while (query.executeStep())
			{
				int userID = query.getColumn(0);
				CUser* user = g_pUserManager->GetUserById(userID);
				if (user)
				{
					g_pConsole->Log(OBFUSCATE("CUserDatabaseSQLite::OnDayTick: TODO: fix user client-side quest update\n"));

					vector<UserQuestProgress> progress;
					GetQuestsProgress(userID, progress);
					g_pPacketManager->SendQuests(user->GetExtendedSocket(), userID, g_pQuestManager->GetQuests(), progress, 0xFFFF, 0, 0, 0);
				}
			}
		}

		g_pConsole->Warn(OBFUSCATE("CUserDatabaseSQLite::OnWeekTick: TODO: impl userdailyreward\n"));
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
		for (auto user : g_pUserManager->users)
		{
			UserDailyRewards dailyReward = {};
			GetDailyRewards(user->GetID(), dailyReward);
			g_pItemManager->UpdateDailyRewardsRandomItems(dailyReward);
			UpdateDailyRewards(user->GetID(), dailyReward);

			g_pPacketManager->SendItemDailyRewardsUpdate(user->GetExtendedSocket(), g_pServerConfig->dailyRewardsItems, dailyReward);
		}*/
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::OnDayTick: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;

	/*vector<UserData_s*> users = g_pUserDatabase->GetUserData();
	for (auto userData : users)
	{
	userData->dailyRewards.randomItems.clear();
	if (!userData->dailyRewards.canGetReward)
	{
	if (userData->dailyRewards.day >= 7)
	userData->dailyRewards.day = 0;
	}
	else
	{
	userData->dailyRewards.day = 0;
	}

	userData->dailyRewards.canGetReward = true;

	CUser* user = g_pUserManager->GetUserById(userData->userId);
	if (user)
	{
	UpdateDailyRewards(userData);

	g_pPacketManager->SendItemDailyRewardsUpdate(user->GetExtendedSocket(), g_pServerConfig->dailyRewardsItems, userData->dailyRewards);
	}

	g_pUserDatabase->UpdateDailyRewards(userData);
	}*/
}

// processes user database every week
// returns 0 == database error, 1 on success
int CUserDatabaseSQLite::OnWeekTick()
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
			query.executeStep();
		}
		// reset task week quest progress
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM UserQuestTaskProgress WHERE questID > 500 AND questID < 1000"));
			query.executeStep();
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
				CUser* user = g_pUserManager->GetUserById(userID);
				if (user)
				{
					g_pConsole->Warn(OBFUSCATE("CUserDatabaseSQLite::OnWeekTick: TODO: fix user client-side quest update\n"));

					vector<UserQuestProgress> progress;
					GetQuestsProgress(userID, progress);
					g_pPacketManager->SendQuests(user->GetExtendedSocket(), userID, g_pQuestManager->GetQuests(), progress, 0xFFFF, 0xFFFF, 0, 0);

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
					g_pPacketManager->SendQuests(user->GetExtendedSocket(), userID, quests, questsProgress, 0x20, 0, 0, 0);*/
				}
			}
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::OnWeekTick: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetUserBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetUsers: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return users;
	}

	return users;
}

int CUserDatabaseSQLite::UpdateIPBanList(string ip, bool remove)
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateIPBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetIPBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return ip;
	}

	return ip;
}

bool CUserDatabaseSQLite::IsIPBanned(string ip)
{
	try
	{
		SQLite::Statement query(m_Database, OBFUSCATE("SELECT ip FROM IPBanList WHERE ip = ?"));
		query.bind(1, ip);
		if (!query.executeStep())
		{
			return 0;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::IsIPBanned: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

int CUserDatabaseSQLite::UpdateHWIDBanList(vector<unsigned char>& hwid, bool remove)
{
	try
	{
		if (remove)
		{
			SQLite::Statement query(m_Database, OBFUSCATE("DELETE FROM HWIDBanList WHERE hwid = ?"));
			query.bind(1, hwid.data(), hwid.size());
			query.exec();
		}
		else
		{
			SQLite::Statement query(m_Database, OBFUSCATE("INSERT or IGNORE INTO HWIDBanList VALUES (?)"));
			query.bind(1, hwid.data(), hwid.size());
			query.exec();
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::UpdateHWIDBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
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
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::GetHWIDBanList: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return hwidList;
	}

	return hwidList;
}

bool CUserDatabaseSQLite::IsHWIDBanned(vector<unsigned char>& hwid)
{
	try
	{
		SQLite::Statement statement(m_Database, OBFUSCATE("SELECT hwid FROM HWIDBanList WHERE hwid = ?"));
		statement.bind(1, hwid.data(), hwid.size());
		if (!statement.executeStep())
		{
			return 0;
		}
	}
	catch (exception& e)
	{
		g_pConsole->Error(OBFUSCATE("CUserDatabaseSQLite::IsHWIDBanned: database internal error: %s, %d\n"), e.what(), m_Database.getErrorCode());
		return 0;
	}

	return 1;
}

chrono::high_resolution_clock::time_point CUserDatabaseSQLite::ExecCalcStart()
{
	return chrono::high_resolution_clock::now();
}

void CUserDatabaseSQLite::ExecCalcEnd(chrono::high_resolution_clock::time_point startTime, string funcName)
{
	auto end = chrono::high_resolution_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(end - startTime).count();

	if (duration > 20)
		g_pConsole->Warn(OBFUSCATE("%s func execution time: %d ms\n"), funcName.c_str(), duration);
}

SQLite::Transaction CUserDatabaseSQLite::CreateTransaction()
{
	return SQLite::Transaction(m_Database);
}

void CUserDatabaseSQLite::CommitTransaction(SQLite::Transaction& trans)
{
	trans.commit();
}