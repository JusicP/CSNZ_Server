#include "channelmanager.h"

#include "itemmanager.h"
#include "packetmanager.h"
#include "minigamemanager.h"
#include "userdatabase.h"
#include "serverinstance.h"
#include "dedicatedservermanager.h"

#include "user/userinventoryitem.h"

#include "csvtable.h"
#include "serverconfig.h"
#include "common/buildnum.h"
#include "common/utils.h"

#include <chrono>

using namespace std;

CChannelManager::CChannelManager() : CBaseManager("ChannelManager")
{
	channelServers.push_back(new CChannelServer("Channel server", 1, 1, 1));
}

bool CChannelManager::OnChannelListPacket(IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = g_pUserManager->GetUserBySocket(socket);
	if (user == NULL)
	{
		g_pPacketManager->SendServerList(socket);

		return false;
	}

	if (user->GetCurrentRoom())
	{
		user->GetCurrentRoom()->RemoveUser(user);
	}

	CChannel* channel = user->GetCurrentChannel();
	if (channel)
	{
		channel->UserLeft(user);
		user->SetCurrentChannel(NULL);

		Console().Log("User '%d, %s' left channel\n", user->GetID(), user->GetUsername().c_str());
	}

	Console().Log("User '%d, %s' requested server list, sending...\n", user->GetID(), user->GetUsername().c_str());

	g_pPacketManager->SendServerList(socket);

	return true;
}

bool CChannelManager::OnRoomRequest(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = g_pUserManager->GetUserBySocket(socket);
	if (user == NULL)
		return false;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case InRoomType::NewRoomRequest:
		return OnNewRoomRequest(msg, user);
	case InRoomType::GameStartRequest:
		return OnGameStartRequest(user);
	case InRoomType::LeaveRoomRequest:
		return OnLeaveRoomRequest(user);
	case InRoomType::ToggleReadyRequest:
		return OnToggleReadyRequest(user);
	case InRoomType::OnConnectionFailure:
		return OnConnectionFailure(user);
	case InRoomType::UserInviteListRequest:
		g_pPacketManager->SendRoomInviteUserList(socket, user);
		break;
	case InRoomType::JoinRoomRequest:
		return OnJoinRoomRequest(msg, user);
	case InRoomType::RequestUpdateSettings:
		return OnRoomUpdateSettings(msg, user);
	case InRoomType::OnCloseResultWindow:
		return OnCloseResultRequest(user);
	case InRoomType::SetUserTeamRequest:
		return OnSetTeamRequest(msg, user);
	case InRoomType::UserInviteRequest:
		return OnUserInviteRequest(msg, user);
	case InRoomType::RoomListRequest:
		g_pPacketManager->SendRoomListFull(socket, channelServers[0]->GetChannels()[0]->GetRooms());
		break;
	case 27:
		break;
	case InRoomType::SetZBAddonsRequest:
		return OnRoomSetZBAddonRequest(msg, user);
	default:
		Console().Warn("Unknown room request %d\n", type);
		break;
	}

	return true;
}

bool CChannelManager::OnRoomListPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

	IUser* user = g_pUserManager->GetUserBySocket(socket);
	if (!user)
	{
		return false;
	}

	int channelServerIndex = msg->ReadUInt8();
	int channelIndex = msg->ReadUInt8();

	JoinChannel(user, channelServerIndex, channelIndex, true);

	return true;
}

void CChannelManager::JoinChannel(IUser* user, int channelServerID, int channelID, bool transfer)
{
	CChannelServer* channelServer = GetServerByIndex(channelServerID);
	CChannel* channel = NULL;

	if (channelServer)
	{
		channel = channelServer->GetChannelByIndex(channelID);
	}

	if (channelServer == NULL || channel == NULL)
	{
		return;
	}

	if (user->GetCurrentChannel() == NULL)
	{
		/*if (transfer && user->GetLastChannelServer() != channelServer && !channelServer->ip.empty())
		{
			g_pPacketManager->SendTransfer(user->GetExtendedSocket(), channelServer->ip, channelServer->port, user->GetUsername());
			g_pUserDatabase->AddToRestoreList(user->GetID(), channelServer->index, channel->m_nIndex);
			return;
		}*/

		Console().Log("User '%d, %s' joined channel '%s'\n", user->GetID(), user->GetUsername().c_str(), channel->GetName().c_str());

		user->SetCurrentChannel(channel);
		user->SetLastChannelServer(channelServer);

		channel->UserJoin(user);
	}
	else
	{
		//char error[256];
		//sprintf(error, "If you see this message, pls write to dev this: '0x%X (0x2)'", user->m_pCurrentChannel);
		//g_pUserManager->SendNoticeMsgBoxToUuid(socket, error);
	}

	Console().Log("User '%d, %s' requested room list successfully, sending...\n", user->GetID(), user->GetUsername().c_str());

	g_pPacketManager->SendRoomListFull(user->GetExtendedSocket(), channel->GetRooms());
}

void CChannelManager::EndAllGames()
{
	for (auto channel : channelServers)
	{
		if (!channel)
			continue;

		for (auto sub : channel->GetChannels())
		{
			if (!sub)
				continue;

			for (auto room : sub->GetRooms())
			{
				if (!room)
					continue;

				if (room->GetStatus() == STATUS_INGAME)
				{
					Console().Log(OBFUSCATE("Force ending RoomID: %d game match\n"), room->GetID());
					room->EndGame();
				}
			}
		}
	}
}

bool CChannelManager::OnLobbyMessage(CReceivePacket* msg, IExtendedSocket* socket, IUser* user)
{
	string message = msg->ReadString();

	string gameName = OBFUSCATE("Not logged in");
	if (user)
	{
		CUserCharacter character = user->GetCharacter(UFLAG_GAMENAME);
		gameName = character.gameName;
	}

	if (!OnCommandHandler(socket, user, message) && user)
	{
		CChannel* channel = user->GetCurrentChannel();
		if (!channel)
		{
			Console().Log(OBFUSCATE("User '%s' tried to send message but he's not in channel\n"), user->GetLogName());
			return false;
		}

		channel->SendLobbyMessageToAllUser(gameName, message);
	}

	return true;
}

bool CChannelManager::OnWhisperMessage(CReceivePacket* msg, IUser* userSender)
{
	if (userSender == NULL)
		return false;

	IExtendedSocket* socket = userSender->GetExtendedSocket();

	string userNameDest = msg->ReadString();
	string message = msg->ReadString();

	IUser* userDest = g_pUserManager->GetUserByNickname(userNameDest);
	if (!userDest)
	{
		// send no user reply
		g_pPacketManager->SendUMsgSystemReply(socket, UMsgPacketType::SystemReply_Red, "MSG_TELL_USER_NOT_FOUND", vector<string>{ userNameDest });
	}
	else if (userDest->GetExtendedSocket() == socket)
	{
		g_pPacketManager->SendUMsgNoticeMessageInChat(socket, "You can't send whisper to yourself.");
		// you can't send whisper message to yourself
	}
	else
	{
		CUserCharacterExtended characterExtendedSender = userSender->GetCharacterExtended(EXT_UFLAG_BANSETTINGS);

		if (characterExtendedSender.banSettings & 4)
		{
			// you can't send whisper message if you're blocking all whisper
			g_pPacketManager->SendUMsgSystemReply(socket, UMsgPacketType::SystemReply_Red, "MSG_TELL_SENDER_USING_BAN_CHAT_ALL");
		}
		else
		{
			CUserCharacterExtended characterExtendedDest = userDest->GetCharacterExtended(EXT_UFLAG_BANSETTINGS);

			if (characterExtendedDest.banSettings & 4)
			{
				// you can't send whisper message if dest is blocking all whisper
				g_pPacketManager->SendUMsgSystemReply(socket, UMsgPacketType::SystemReply_Red, "MSG_TELL_LISTENER_USING_BAN_CHAT_ALL");
			}
			else
			{
				g_pPacketManager->SendUMsgWhisperMessage(socket, message, userNameDest, userSender, 0);
				CUserCharacter character = userSender->GetCharacter(UFLAG_GAMENAME);
				g_pPacketManager->SendUMsgWhisperMessage(userDest->GetExtendedSocket(), message, character.gameName, userDest, 1);
			}
		}
	}

	return true;
}

bool CChannelManager::OnRoomUserMessage(CReceivePacket* msg, IUser* user)
{
	if (user == NULL || user->GetCurrentRoom() == NULL)
		return false;

	user->GetCurrentRoom()->OnUserMessage(msg, user);

	return true;
}

bool CChannelManager::OnRoomTeamUserMessage(CReceivePacket* msg, IUser* user)
{
	if (user == NULL || user->GetCurrentRoom() == NULL)
		return false;

	user->GetCurrentRoom()->OnUserTeamMessage(msg, user);

	return true;
}

CChannelServer* CChannelManager::GetServerByIndex(int index)
{
	for (auto server : this->channelServers)
	{
		if (server->GetID() == index)
			return server;
	}

	return NULL;
}

bool CChannelManager::OnCommandHandler(IExtendedSocket* socket, IUser* user, const string& message)
{
	vector<string> args = ParseArguments(message);
	if (args.size() == 0 || args[0][0] != '/') // all lobby commands starts with '/' character
	{
		return false;
	}

	if (!user)
	{
		if (args[0] == (char*)OBFUSCATE("/login"))
		{
			if (args.size() == 3)
			{
				string login = args[1];
				string password = args[2];

				int loginResult = g_pUserManager->LoginUser(socket, login, password);
				switch (loginResult)
				{
				case LOGIN_DB_ERROR:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("DB_QUERY_FAILED"));
					break;
				case LOGIN_NO_SUCH_USER:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Wrong password or username."));
					break;
				case LOGIN_USER_BANNED:
					g_pServerInstance->DisconnectClient(socket);
					break;
				case LOGIN_SERVER_CANNOT_VALIDATE_CLIENT:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Failed to validate client. Contact administrator and try to reinstall the game."));
					break;
				}
			}
			else
			{
				g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/login arguments: <username> <password>"));
			}
		}
		else if (args[0] == (char*)OBFUSCATE("/register"))
		{
			if (args.size() == 3)
			{
				string login = args[1];
				string password = args[2];

				int regResult = g_pUserManager->RegisterUser(socket, login, password);
				switch (regResult)
				{
				case REGISTER_DB_ERROR:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("DB_QUERY_FAILED"));
					break;
				case REGISTER_OK:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("You have successfully registered."));
					break;
				case REGISTER_USERNAME_EXIST:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("User with this username already exists."));
					break;
				case REGISTER_USERNAME_WRONG:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Username must contain at least 5 characters and not more than 15, English letters."));
					break;
				case REGISTER_PASSWORD_WRONG:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Password must contain at least 5 characters and not more than 15, not only numbers"));
					break;
				case REGISTER_IP_LIMIT:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("You have exceeded the account limit for one IP"));
					break;
				}
			}
			else
			{
				g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/register arguments: <username> <password>"));
			}
		}
	}
	else
	{
		CUserCharacterExtended character = user->GetCharacterExtended(EXT_UFLAG_GAMEMASTER);
		if (character.gameMaster)
		{
			if (args[0] == (char*)OBFUSCATE("/version"))
			{
				char buf[128];
				snprintf(buf, sizeof(buf), OBFUSCATE("Server build: %s"), build_number());
				g_pPacketManager->SendUMsgNoticeMessageInChat(socket, buf);
				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/help"))
			{
				g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("Available commands: /help, /version, /giveitem, /additem, /removeitem, /getfreeslots, /sendnotice, /addallitems"));
				g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/addexp, /status, /ban, /hban, /ipban, /unban, /getuid, /tournament, /givereward, /giverewardtoall"));
				g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/addwpnreleasechar, /addpoints, /givepoints, /weaponrelease"));
				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/disconnect"))
			{
				g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("Not implemented\n"));
				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/getfreeslots"))
			{
				g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("Not implemented\n"));
				return true;
				//g_pPacketManager->SendUMsgNoticeMessageInChat(socket, va("You still have %d spaces.", uData->inventory->AvailableInventorySpace()));
			}
			else if (args[0] == (char*)OBFUSCATE("/additem"))
			{
				int itemID, count = 1, duration = 0;

				if (!(args.size() >= 2))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/additem usage: /additem <itemID> <count> <duration>"));
					return true;
				}

				if (!isNumber(args[1]))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/additem usage: /additem <itemID> <count> <duration>"));
					return true;
				}

				istringstream iss(args[1]);
				iss >> itemID;

				if (iss.fail())
					return true;

				if (args.size() >= 3 && isNumber(args[2]))
				{
					istringstream iss2(args[2]);
					iss2 >> count;

					if (iss2.fail())
						return true;
				}

				if (args.size() >= 4 && isNumber(args[3]))
				{
					iss.clear();
					iss.str(args[3]);
					iss >> duration;

					if (iss.fail())
						return true;
				}

				int status = g_pItemManager->AddItem(user->GetID(), user, itemID, count, duration); // add permanent item by default
				switch (status)
				{
				case ITEM_ADD_INVENTORY_FULL:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Your inventory is full"));
					break;
				case ITEM_ADD_UNKNOWN_ITEMID:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Item ID you wrote does not exist in the item database"));
					break;
				case ITEM_ADD_DB_ERROR:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Database error"));
					break;
				case ITEM_ADD_SUCCESS:
					// send notification about new item
					RewardItem rewardItem;
					rewardItem.itemID = itemID;
					rewardItem.count = count;
					rewardItem.duration = duration;

					RewardNotice rewardNotice;
					rewardNotice.rewardId = 1;
					rewardNotice.exp = 0;
					rewardNotice.points = 0;
					rewardNotice.honorPoints = 0;
					rewardNotice.items.push_back(rewardItem);
					g_pPacketManager->SendUMsgRewardNotice(socket, rewardNotice);

					break;
				}

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/giveitem"))
			{
				if (!(args.size() >= 3))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/giveitem usage: /giveitem <gameName/userID> <itemID> <count> <duration>"));
					return true;
				}

				int itemID, userID, count = 1, duration = 0;

				if (isNumber(args[1]))
				{
					istringstream iss(args[1]);
					iss >> userID;

					if (iss.fail())
						return true;

					if (!g_pUserDatabase->IsUserExists(userID))
						userID = 0;
				}
				else
				{
					userID = g_pUserDatabase->IsUserExists(args[1], false);
				}

				if (!userID)
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("User not found."));
					return true;
				}

				istringstream iss(args[2]);
				iss >> itemID;

				if (iss.fail())
					return true;

				if (args.size() >= 4 && isNumber(args[3]))
				{
					istringstream iss2(args[3]);
					iss2 >> count;

					if (iss2.fail())
						return true;
				}

				if (args.size() >= 5 && isNumber(args[4]))
				{
					iss.clear();
					iss.str(args[4]);
					iss >> duration;

					if (iss.fail())
						return true;
				}

				IUser* user = g_pUserManager->GetUserById(userID);
				int status = g_pItemManager->AddItem(userID, user, itemID, count, duration); // add permanent item by default
				switch (status)
				{
				case ITEM_ADD_INVENTORY_FULL:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("User's inventory is full"));
					break;
				case ITEM_ADD_UNKNOWN_ITEMID:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Item ID you wrote does not exist in the item database"));
					break;
				case ITEM_ADD_DB_ERROR:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Database error"));
					break;
				case ITEM_ADD_SUCCESS:
				{
					// send notification about new item
					RewardItem rewardItem;
					rewardItem.itemID = itemID;
					rewardItem.count = count;
					rewardItem.duration = duration;

					RewardNotice rewardNotice;
					rewardNotice.rewardId = 1;
					rewardNotice.exp = 0;
					rewardNotice.points = 0;
					rewardNotice.honorPoints = 0;
					rewardNotice.items.push_back(rewardItem);

					if (!user)
					{
						// TODO: should we use custom rewards here?
					}
					else
					{
						g_pPacketManager->SendUMsgRewardNotice(user->GetExtendedSocket(), rewardNotice);
						g_pPacketManager->SendUMsgRewardNotice(user->GetExtendedSocket(), rewardNotice, "", "", true);
					}

					break;
				}
				}

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/addallitems"))
			{
				vector<string> items = g_pItemTable->GetRowNames();
				vector<RewardItem> rewardItems;
				CUserInventoryItem userItem;
				int itemID = 0;
				for (auto item : items)
				{
					itemID = stoi(item);
					if (userItem.IsItemDefaultOrPseudo(itemID))
						continue;

					RewardItem rewardItem;
					rewardItem.itemID = itemID;
					rewardItem.count = 1;
					rewardItem.duration = 0;
					rewardItems.push_back(rewardItem);
				}

				int status = g_pItemManager->AddItems(user->GetID(), user, rewardItems);
				switch (status)
				{
				case ITEM_ADD_SUCCESS:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Done"));
					break;
				case ITEM_ADD_INVENTORY_FULL:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Your inventory is full"));
					break;
				case ITEM_ADD_DB_ERROR:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Database error"));
					break;
				}

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/removeitem"))
			{
				if (args.size() >= 2)
				{
					if (isNumber(args[1]))
					{
						int itemID = stoi(args[1]);

						vector<CUserInventoryItem> items;
						g_pUserDatabase->GetInventoryItemsByID(user->GetID(), itemID, items);

						if (items.size() > 0)
						{
							if (args.size() >= 3 && isNumber(args[1]))
							{
								int slot = stoi(args[2]);
								if (slot >= (int)items.size())
								{
									g_pPacketManager->SendUMsgNoticeMessageInChat(socket, va(OBFUSCATE("You have written an invalid slot ID %d"), slot));
									return true;
								}

								if (!g_pItemManager->RemoveItem(user->GetID(), user, items[slot]))
								{
									g_pPacketManager->SendUMsgNoticeMessageInChat(socket, va(OBFUSCATE("Item (%d, %d) cannot be removed"), itemID, slot));
									return true;
								}

								g_pPacketManager->SendUMsgNoticeMessageInChat(socket, va(OBFUSCATE("Item (%d, %d) removed"), itemID, slot));
								return true;
							}

							vector<string> texts;
							texts.push_back(OBFUSCATE("Select one of the available item slots to remove:\n"));

							int i = 0;
							for (auto& item : items)
							{
								texts.push_back(string(va(OBFUSCATE("[%d], count: %d\n"), i++, item.m_nCount)));
							}

							texts.push_back(OBFUSCATE("Enter /removeitem <itemID> <slot>"));

							string result;
							for (auto const& s : texts) { result += s; }

							g_pPacketManager->SendUMsgNoticeMessageInChat(socket, result);
						}
						else
						{
							g_pPacketManager->SendUMsgNoticeMessageInChat(socket, va(OBFUSCATE("Items with %d ID cannot be found in your inventory"), itemID));
						}
					}
					return true;
				}
				else
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/removeitem usage: /removeitem <itemID>"));
					return true;
				}
			}
			else if (args[0] == (char*)OBFUSCATE("/sendnotice") && args.size() >= 2)
			{
				string text = message;
				text.erase(0, strlen(OBFUSCATE("/sendnotice ")));
				
				g_pUserManager->SendNoticeMsgBoxToAll(text);

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/addexp") && args.size() == 2 && isNumber(args[1]))
			{
				try
				{
					uint64_t exp = stoll(args[1]);
					user->UpdateExp(exp);
				}
				catch (exception& ex)
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/addexp out of range!"));
					Console().Warn("/addexp out of range! %s\n", ex.what());
					return true;
				}

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/status"))
			{
				g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, g_pServerInstance->GetMainInfo());

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/ban"))
			{
				if (!(args.size() >= 5) || !isNumber(args[1]) || !isNumber(args[2]) || !isNumber(args[4]))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/ban usage: /ban <userID> <type> <reason> <term>. Ban user's account"));
					return true;
				}

				int userID = atoi(args[1].c_str());
				int banType = atoi(args[2].c_str());
				string reason = args[3];
				int term = atoi(args[4].c_str());

				if (!g_pUserDatabase->IsUserExists(userID))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("User does not exist"));
					return true;
				}

				if (!term)
				{
					term = 999999;
				}

				UserBan ban;
				ban.banType = banType;
				ban.reason = reason;
				ban.term = term * CSO_24_HOURS_IN_MINUTES + g_pServerInstance->GetCurrentTime();

				IUser* user = g_pUserManager->GetUserById(userID);
				if (user)
				{
					// disconnect user right now
					g_pUserManager->DisconnectUser(user);
				}

				g_pUserDatabase->UpdateUserBan(userID, ban);

				CUserCharacter character = {};
				character.flag = UFLAG_GAMENAME;
				g_pUserDatabase->GetCharacter(userID, character);
				if (banType == 1)
				{
					g_pUserManager->SendNoticeMessageToAll(va(OBFUSCATE("%s is banned. Reason: %s"), character.gameName.c_str(), reason.c_str()));
				}

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/hban"))
			{
				if (!(args.size() >= 2) || !isNumber(args[1]))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/hban usage: /hban <userID>. Ban user by HWID"));
					return true;
				}

				int userID = atoi(args[1].c_str());

				if (!g_pUserDatabase->IsUserExists(userID))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("User does not exist"));
					return true;
				}

				IUser* user = g_pUserManager->GetUserById(userID);
				if (user)
				{
					g_pUserManager->DisconnectUser(user);
				}

				CUserData data;
				data.flag |= UDATA_FLAG_LASTHWID;
				g_pUserDatabase->GetUserData(userID, data);

				g_pUserDatabase->UpdateHWIDBanList(data.lastHWID);

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/ipban"))
			{
				if (!(args.size() >= 2) || !isNumber(args[1]))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/ipban usage: /ipban <userID>. Ban user by IP"));
					return true;
				}

				int userID = atoi(args[1].c_str());

				if (!g_pUserDatabase->IsUserExists(userID))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("User does not exist"));
					return true;
				}

				IUser* user = g_pUserManager->GetUserById(userID);
				if (user)
				{
					g_pUserManager->DisconnectUser(user);
				}

				CUserData data;
				data.flag |= UDATA_FLAG_LASTIP;
				g_pUserDatabase->GetUserData(userID, data);

				g_pUserDatabase->UpdateIPBanList(data.lastIP);

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/unban"))
			{
				if (!(args.size() >= 2) || !isNumber(args[1]))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/unban usage: /unban <userID>. Unban last IP, HWID and user account"));
					return true;
				}

				int userID = atoi(args[1].c_str());
				if (!g_pUserDatabase->IsUserExists(userID))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("User does not exist"));
					return true;
				}

				UserBan ban;
				if (g_pUserDatabase->GetUserBan(userID, ban))
				{
					ban.banType = 0;
					ban.reason = "";
					ban.term = 0;

					g_pUserDatabase->UpdateUserBan(userID, ban);
				}

				CUserData data;
				data.flag |= UDATA_FLAG_LASTIP | UDATA_FLAG_LASTHWID;
				g_pUserDatabase->GetUserData(userID, data);

				g_pUserDatabase->UpdateIPBanList(data.lastIP, true);
				g_pUserDatabase->UpdateHWIDBanList(data.lastHWID, true);

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/getuid"))
			{
				if (!(args.size() >= 2))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/getuid usage: /getuid <gameName>"));
					return true;
				}

				int userID = g_pUserDatabase->IsUserExists(args[1], false);
				if (!userID)
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("User does not exist"));
					return true;
				}

				g_pPacketManager->SendUMsgNoticeMessageInChat(socket, va("%d", userID));

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/tournament"))
			{
				// TODO: what the fuck?
				CUserCharacter character = user->GetCharacter(UFLAG_TOURNAMENT);
				user->UpdateTournament(character.tournament ? 0 : 0xFF);

				character = user->GetCharacter(UFLAG_TOURNAMENT);

				g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, character.tournament ? (char*)OBFUSCATE("Tournament HUD is ON") : (char*)OBFUSCATE("Tournament HUD is OFF"));

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/givereward"))
			{
				if (args.size() <= 2 || !isNumber(args[1]) || !isNumber(args[2]))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/givereward usage: /givereward <userID> <rewardID>"));
					return true;
				}

				int userID = atoi(args[1].c_str());
				int rewardID = atoi(args[2].c_str());
				if (rewardID <= 0 || userID <= 0)
					return true;

				g_pItemManager->GiveReward(userID, g_pUserManager->GetUserById(userID), rewardID);

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/giverewardtoall"))
			{
				if (args.size() <= 1 || !isNumber(args[1]))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/giverewardtoall usage: giverewardtoall <rewardID>. Give a reward from ItemRewards.txt to all users' accounts"));
					return true;
				}

				int rewardID = atoi(args[1].c_str());
				if (rewardID <= 0)
					return true;

				auto users = g_pUserDatabase->GetUsers();
				for (auto userID : users)
				{
					g_pItemManager->GiveReward(userID, g_pUserManager->GetUserById(userID), rewardID);
				}

				return true;
			}
#ifndef PUBLIC_RELEASE
			else if (args[0] == (char*)OBFUSCATE("/addgroom"))
			{
				CChannel* channel = user->GetCurrentChannel();
				if (channel)
				{
					CRoomSettings* roomSettings = new CRoomSettings();
					roomSettings->lowFlag |= ROOM_LOW_ROOMNAME | ROOM_LOW_PASSWORD | ROOM_LOW_GAMEMODEID | ROOM_LOW_MAPID | ROOM_LOW_MAXPLAYERS | ROOM_LOW_WINLIMIT | ROOM_LOW_KILLLIMIT;
					roomSettings->lowMidFlag |= ROOM_LOWMID_ZSDIFFICULTY;
					roomSettings->roomName = (char*)OBFUSCATE("GHOST ROOM NAME");
					roomSettings->password = (char*)OBFUSCATE("r,=j$b5}a@dgN&^g0_!}['WH}l5i]#ugAhfQ ? dS; Qh1Ckk`R}bz, o[QMgp4]0");
					roomSettings->gameModeId = 15;
					roomSettings->mapId = 128;
					roomSettings->maxPlayers = 10;
					roomSettings->zsDifficulty = 1;

					if (g_pServerConfig->room.validateSettings && !roomSettings->CheckSettings(user))
					{
						g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), "Unable to create a room due to incorrect settings");
						delete roomSettings;
						return false;
					}

					IRoom* room = channel->CreateRoom(user, roomSettings);
				}

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/reqsnapshot"))
			{
				return true;
			}
#endif
			else if (args[0] == (char*)OBFUSCATE("/addwpnreleasechar"))
			{
				if (args.size() <= 2 || !isNumber(args[2]))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/addwpnreleasechar usage: /addwpnreleasechar <char> <count>"));
					return true;
				}

				g_pMiniGameManager->WeaponReleaseAddCharacter(user, args[1][0], stoi(args[2]));

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/addpoints"))
			{
				if (args.size() <= 1 || !isNumber(args[1]))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/addpoints usage: /addpoints <points>"));
					return true;
				}

				int points = stoi(args[1]);

				user->UpdatePoints(points);

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/givepoints"))
			{
				if (args.size() <= 2 || !isNumber(args[1]) || !isNumber(args[2]))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/givepoints usage: /givepoints <userID> <points>"));
					return true;
				}

				int userID = stoi(args[1]);
				int points = stoi(args[2]);

				if (!g_pUserDatabase->IsUserExists(userID))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("User does not exist"));
					return true;
				}

				IUser* userDest = g_pUserManager->GetUserById(userID);
				if (userDest)
				{
					userDest->UpdatePoints(points);
				}
				else
				{
					CUserCharacter character = {};
					character.flag = UFLAG_POINTS;
					if (g_pUserDatabase->GetCharacter(userID, character) <= 0)
					{
						g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("User character does not exist"));
						return true;
					}

					character.points += points;

					if (g_pUserDatabase->UpdateCharacter(userID, character) <= 0)
					{
						g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("User character does not exist"));
						return true;
					}
				}

				return true;
			}
			else if (args[0] == "/setitemstatus")
			{
				if (args.size() <= 2 || !isNumber(args[1]) || !isNumber(args[2]))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/setitemstatus usage: /setitemstatus <slot> <status>"));
					return true;
				}

				int slot = stoi(args[1]);
				int status = stoi(args[2]);

				CUserInventoryItem item;
				if (g_pUserDatabase->GetInventoryItemBySlot(user->GetID(), slot, item) <= 0 || !item.m_nItemID)
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("Item does not exist"));
					return true;
				}

				item.m_nStatus = status;
				vector<CUserInventoryItem> items;
				items.push_back(item);

				if (g_pUserDatabase->UpdateInventoryItem(user->GetID(), item) <= 0)
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("Database error"));
					return true;
				}

				g_pPacketManager->SendInventoryAdd(user->GetExtendedSocket(), items);

				return true;
			}
			else if (args[0] == "/setiteminuse")
			{
				if (args.size() <= 2 || !isNumber(args[1]) || !isNumber(args[2]))
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("/setiteminuse usage: /setiteminuse <slot> <inUse>"));
					return true;
				}

				int slot = stoi(args[1]);
				int inUse = stoi(args[2]);

				CUserInventoryItem item;
				if (g_pUserDatabase->GetInventoryItemBySlot(user->GetID(), slot, item) <= 0 || !item.m_nItemID)
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("Item does not exist"));
					return true;
				}

				item.m_nInUse = inUse;
				vector<CUserInventoryItem> items;
				items.push_back(item);

				if (g_pUserDatabase->UpdateInventoryItem(user->GetID(), item) <= 0)
				{
					g_pPacketManager->SendUMsgNoticeMessageInChat(socket, OBFUSCATE("Database error"));
					return true;
				}

				g_pPacketManager->SendInventoryAdd(user->GetExtendedSocket(), items);

				return true;
			}
		}

		if (args[0] == "/weaponrelease")
		{
			if (g_pServerConfig->activeMiniGamesFlag & kEventFlag_WeaponRelease)
				g_pMiniGameManager->SendWeaponReleaseUpdate(user);
			return true;
		}
		else if (args[0] == "/bingo")
		{
			if (g_pServerConfig->activeMiniGamesFlag & kEventFlag_Bingo_NEW || g_pServerConfig->activeMiniGamesFlag & kEventFlag_Bingo)
				g_pMiniGameManager->OnBingoUpdateRequest(user);
			return true;
		}
	}

	return false;
}

bool CChannelManager::OnNewRoomRequest(CReceivePacket* msg, IUser* user)
{
	// don't allow the user to create a new room while in another one
	IRoom* room = user->GetCurrentRoom();
	if (room)
	{
		Console().Warn("User '%d, %s' tried to create a new room, but he is already playing in other room, curRoomId: %d\n", user->GetID(), user->GetUsername().c_str(), room->GetID());
		return false;
	}

	CChannel* channel = user->GetCurrentChannel();
	if (channel == NULL)
	{
		Console().Warn("User '%d, %s' tried to create a new room without channel\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	CRoomSettings* roomSettings = new CRoomSettings(msg->GetData());
	if (!roomSettings->CheckSettings(user))
	{
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), "Unable to create a room due to incorrect settings");
		delete roomSettings;
		return false;
	}

	IRoom* newRoom = channel->CreateRoom(user, roomSettings);

	user->SetCurrentRoom(newRoom);

	newRoom->SendJoinNewRoom(user);

	// hide user from channel users list
	channel->UserLeft(user, true);

	Console().Log("User '%d, %s' created a new room (RID: %d, name: '%s')\n", user->GetID(), user->GetUsername().c_str(), newRoom->GetID(), roomSettings->roomName.c_str());

	return true;
}

bool CChannelManager::OnJoinRoomRequest(CReceivePacket* msg, IUser* user)
{
	int unk = msg->ReadUInt8();
	int roomID = msg->ReadUInt16();
	string password = msg->ReadString();

	CChannel* channel = user->GetCurrentChannel();
	if (channel == NULL)
	{
		Console().Warn("User '%d, %s' tried to join room (RID: %d) without channel\n", user->GetID(), user->GetUsername().c_str(), roomID);
		return false;
	}

	if (user->GetCurrentRoom() != NULL)
	{
		// don't let user to join room if he in another one
		return false;
	}

	IRoom* room = channel->GetRoomById(roomID);
	if (room == NULL)
	{
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("ROOM_JOIN_FAILED_CLOSED"));
		return false;
	}

	if (room->HasFreeSlots() == false)
	{
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("ROOM_JOIN_FAILED_FULL"));
		return false;
	}

	CUserCharacterExtended characterExtended = user->GetCharacterExtended(EXT_UFLAG_GAMEMASTER);
	if (room->HasPassword() == true && !characterExtended.gameMaster)
	{
		if (room->GetSettings()->password != password)
		{
			g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("ROOM_JOIN_FAILED_INVALID_PASSWD"));
			return false;
		}
	}

	CUserCharacterExtended hostCharacterExtended = room->GetHostUser()->GetCharacterExtended(EXT_UFLAG_BANSETTINGS);
	if (hostCharacterExtended.banSettings & 1 && g_pUserDatabase->IsInBanList(room->GetHostUser()->GetID(), user->GetID()))
	{
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("ROOM_JOIN_FAILED_BAN"));
		return false;
	}

	if (room->IsUserKicked(user->GetID()))
	{
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("ROOM_JOIN_FAILED_KICKED"));
		return false;
	}

	room->AddUser(user);
	room->SendJoinNewRoom(user);

	user->SetCurrentRoom(room);

	// tell other room members about the new addition
	for (auto u : room->GetUsers())
	{
		room->SendUserReadyStatus(user, u);

		if (u == user)
			continue;

		room->SendNewUser(u, user);
		//room->SendUserReadyStatus(user, u);
		room->SendTeamChange(u, user, user->GetRoomData()->m_Team);
	}

	// hide user from channel users list
	channel->UserLeft(user, true);

	channel->SendUpdateRoomList(room);

	Console().Log("User '%d, %s' joined a room (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), room->GetID());

	return true;
}

bool CChannelManager::OnSetTeamRequest(CReceivePacket* msg, IUser* user)
{
	IRoom* currentRoom = user->GetCurrentRoom();
	if (currentRoom == NULL)
	{
		Console().Warn("User '%d, %s' tried to change room settings, but he isn't in room\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	if (currentRoom->IsUserReady(user))
	{
		Console().Warn("User '%d, %s' tried to change team in a room, but he is ready\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	int newTeam = msg->ReadUInt8();

	currentRoom->SetUserToTeam(user, (RoomTeamNum)newTeam);

	// inform every user in the room of the changes
	for (auto u : currentRoom->GetUsers())
	{
		currentRoom->SendTeamChange(u, user, (RoomTeamNum)newTeam);
	}

	Console().Log("User '%d, %s' changed room team to %d (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), newTeam, currentRoom->GetID());

	return true;
}

bool CChannelManager::OnLeaveRoomRequest(IUser* user)
{
	IRoom* currentRoom = user->GetCurrentRoom();
	CChannel* currentChannel = user->GetCurrentChannel();

	if (currentRoom == NULL || currentChannel == NULL)
	{
		Console().Log(OBFUSCATE("User '%d' tried to leave room without curRoom or curChannel\n"), user->GetID());

		return false;
	}

	if (currentRoom->IsUserIngame(user) == true)
	{
		currentRoom->SendPlayerLeaveIngame(user);
	}

	Console().Log("User '%d, %s' left a room (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), currentRoom->GetID());

	currentRoom->RemoveUser(user);

	if (!currentRoom->GetUsers().size())
	{
		currentChannel->SendRemoveFromRoomList(currentRoom->GetID());

		CDedicatedServer* server = currentRoom->GetServer();
		if (currentRoom->GetServer())
		{
			server->SetRoom(NULL);
			g_pPacketManager->SendHostStop(server->GetSocket());
		}
	}
	else
	{
		currentChannel->SendUpdateRoomList(currentRoom);
	}

	currentChannel->SendFullUpdateRoomList(user);

	// add user to channel user list back
	currentChannel->UserJoin(user, true);
	g_pPacketManager->SendLobbyJoin(user->GetExtendedSocket(), currentChannel);

	return true;
}

bool CChannelManager::OnToggleReadyRequest(IUser* user)
{
	IRoom* room = user->GetCurrentRoom();
	if (room == NULL)
	{
		Console().Warn("User '%d, %s' tried to toggle ready status\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	RoomReadyStatus readyStatus = room->ToggleUserReadyStatus(user);

	Console().Log("User '%d, %s' toggled ready status %d\n", user->GetID(), user->GetUsername().c_str(), readyStatus);

	// inform every user in the room of the changes
	for (auto u : room->GetUsers())
	{
		g_pPacketManager->SendRoomSetPlayerReady(u->GetExtendedSocket(), user, readyStatus);
	}

	return true;
}

bool CChannelManager::OnConnectionFailure(IUser* user)
{
	IRoom* room = user->GetCurrentRoom();
	if (room == NULL)
	{
		return false;
	}

	if (room->GetGameMatch() == NULL)
	{
		return false;
	}

	IUser* hostUser = room->GetHostUser();
	if (hostUser == NULL)
	{
		return false;
	}

	CDedicatedServer* server = room->GetServer();

	if (server)
	{
		string ip = ip_to_string(server->GetIP());

		Console().Log("User '%d, %s' unsuccessfully tried to connect to the game match(%s:%d)\n", user->GetID(), user->GetUsername().c_str(), ip.c_str(), server->GetPort());

		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), "ROOM_JOIN_FAILED_INVALID_GAME_IP");
	}
	else
	{
		Console().Log("User '%d, %s' unsuccessfully tried to connect to the game match(%s:%d)\n", user->GetID(), user->GetUsername().c_str(), hostUser->GetNetworkConfig().m_szExternalIpAddress.c_str(), hostUser->GetNetworkConfig().m_nExternalServerPort);

		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), va("Cannot establish connection to %s:%d. Possible reasons:\n"
			"1. Host connected to the master server with localhost(127.0.0.1) ip\n"
			"2. Host has a closed %d port or 30002 port (TCP and UDP)\n"
			"3. You are trying to connect to the host with private IP.\n"
			"Host address: %s\n"
			"Your address: %s",
			hostUser->GetNetworkConfig().m_szExternalIpAddress.c_str(), hostUser->GetNetworkConfig().m_nExternalServerPort, hostUser->GetNetworkConfig().m_nExternalServerPort, hostUser->GetNetworkConfig().m_szExternalIpAddress.c_str(), user->GetNetworkConfig().m_szExternalIpAddress.c_str()));
	}

	RoomReadyStatus readyStatus = room->ToggleUserReadyStatus(user);
	for (auto u : room->GetUsers())
	{
		g_pPacketManager->SendRoomSetPlayerReady(u->GetExtendedSocket(), user, readyStatus);
	}

	return true;
}

bool CChannelManager::OnGameStartRequest(IUser* user)
{
	IRoom* currentRoom = user->GetCurrentRoom();
	CChannel* currentChannel = user->GetCurrentChannel();
	if (currentRoom == NULL || currentChannel == NULL)
	{
		Console().Warn("User '%d, %s' isn't in room or channel but he tried to start room match.\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	CRoomSettings* roomSettings = currentRoom->GetSettings();
	g_pPacketManager->SendLeagueGaugePacket(user->GetExtendedSocket(), roomSettings->gameModeId);

	// send to the host game start request
	if (currentRoom->GetHostUser() == user)
	{
		currentRoom->HostStartGame();

		return true;
	}
	else if (currentRoom->GetStatus() == RoomStatus::STATUS_INGAME)
	{
		currentRoom->UserGameJoin(user);

		return true;
	}
	else
	{
		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), OBFUSCATE("ROOM_JOIN_FAILED_CLOSED"));
	}

	return false;
}

bool CChannelManager::OnCloseResultRequest(IUser* user)
{
	IRoom* room = user->GetCurrentRoom();
	if (room == NULL)
		return false;

	room->SendCloseResultWindow(user);

	return true;
}

bool CChannelManager::OnRoomUpdateSettings(CReceivePacket* msg, IUser* user)
{
	IRoom* currentRoom = user->GetCurrentRoom();
	CChannel* currentChannel = user->GetCurrentChannel();
	if (currentRoom == NULL)
	{
		Console().Warn("User '%d, %s' tried to update a room\'s settings, although it isn\'t in any\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	if (currentChannel == NULL)
	{
		Console().Warn("User '%d, %s' tried to update a room\'s settings without current channel\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	if (user != currentRoom->GetHostUser())
	{
		Console().Warn("User '%d, %s' tried to update a room\'s settings, although it isn\'t the host (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), currentRoom->GetID());
		return false;
	}

	if (currentRoom->GetGameMatch() != NULL)
	{
		Console().Warn("User '%d, %s' tried to update a room\'s settings, but m_pGameMatch != NULL (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), currentRoom->GetID());
		return false;
	}

	CRoomSettings* roomSettings = currentRoom->GetSettings();

	if (roomSettings->mapPlaylistIndex > 1)
	{
		Console().Warn("User '%d, %s' tried to update a room\'s settings while mapPlaylist is on-going (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), currentRoom->GetID());
		return false;
	}

	CRoomSettings newSettings(msg->GetData());
	if (!newSettings.CheckNewSettings(user, roomSettings))
		return false;

	currentRoom->UpdateSettings(newSettings);

	// inform every user in the room of the changes
	for (auto u : currentRoom->GetUsers())
	{
		currentRoom->SendUpdateRoomSettings(u, roomSettings, newSettings.lowFlag, newSettings.lowMidFlag, newSettings.highMidFlag, newSettings.highFlag);
	}

	currentChannel->SendUpdateRoomList(currentRoom);

	Console().Log("Host '%d, %s' updated room settings (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), currentRoom->GetID());

	return true;
}

bool CChannelManager::OnUserInviteRequest(CReceivePacket* msg, IUser* user)
{
	string inviteMsg = msg->ReadString();
	int userCount = msg->ReadUInt16();
	for (int i = 0; i < userCount; i++)
	{
		string userGameName = msg->ReadString();

		IUser* destUser = g_pUserManager->GetUserByNickname(userGameName);
		if (!destUser)
		{
			// user does not exists
			continue;
		}
		else if (destUser == user)
		{
			// you can't invite yourself
			continue;
		}
		/*else if (!user->GetCurrentChannel()->GetUserById(destUser->GetData()->userId))
		{
			// user isn't in channel
			continue;
		}*/

		CUserCharacter character = user->GetCharacter(UFLAG_GAMENAME);
		g_pPacketManager->SendSearchRoomNotice(destUser->GetExtendedSocket(), user->GetCurrentRoom(), character.gameName, inviteMsg);
	}

	return true;
}

bool CChannelManager::OnRoomSetZBAddonRequest(CReceivePacket* msg, IUser* user)
{
	IRoom* currentRoom = user->GetCurrentRoom();
	if (currentRoom == NULL)
	{
		return false;
	}

	int size = msg->ReadUInt16();
	if (size > 6)
	{
		return false;
	}

	vector<int> addons;
	for (int i = 0; i < size; i++)
	{
		int itemID = msg->ReadUInt16();

		if (g_pItemTable->GetCell<string>("ClassName", to_string(itemID)) == "zbsaddonitem")
		{
			vector<CUserInventoryItem> items;
			if (g_pUserDatabase->GetInventoryItemsByID(user->GetID(), itemID, items) == 1
				&& g_pItemManager->OnItemUse(user, items[0]))
			{
				addons.push_back(itemID);
			}
		}
	}

	g_pUserDatabase->SetAddons(user->GetID(), addons);

	return true;
}