#include "ChannelManager.h"
#include "ItemManager.h"
#include "BuildNum.h"
#include "PacketManager.h"
#include "ServerInstance.h"
#include "CSVTable.h"
#include "ServerConfig.h"
#include <chrono>

using namespace std;

CChannelManager::CChannelManager()
{
	channelServers.push_back(new CChannelServer("Channel server", 1, 1, 1));
}

bool CChannelManager::OnChannelListPacket(CExtendedSocket* socket)
{
	LOG_PACKET;

	CUser* user = g_pUserManager->GetUserBySocket(socket);
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

		g_pConsole->Log("User '%d, %s' left channel\n", user->GetID(), user->GetUsername().c_str());
	}

	g_pConsole->Log("User '%d, %s' requested server list, sending...\n", user->GetID(), user->GetUsername().c_str());

	g_pPacketManager->SendServerList(socket);

	return true;
}

bool CChannelManager::OnRoomRequest(CReceivePacket* msg, CExtendedSocket* socket)
{
	LOG_PACKET;

	CUser* user = g_pUserManager->GetUserBySocket(socket);
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
		g_pPacketManager->SendRoomListFull(socket, channelServers[0]->channels[0]->m_Rooms);
		break;
	case 27:
		break;
	case InRoomType::SetZBAddons:
		return OnRoomSetZBAddonRequest(msg, user);
	default:
		g_pConsole->Warn("Unknown room request %d\n", type);
		break;
	}

	return true;
}

bool CChannelManager::OnRoomListPacket(CReceivePacket* msg, CExtendedSocket* socket)
{
	LOG_PACKET;

	CUser* user = g_pUserManager->GetUserBySocket(socket);
	if (!user)
	{
		return false;
	}

	int channelServerIndex = msg->ReadUInt8();
	int channelIndex = msg->ReadUInt8();

	JoinChannel(user, channelServerIndex, channelIndex, true);

	return true;
}

void CChannelManager::JoinChannel(CUser* user, int channelServerID, int channelID, bool transfer)
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

		g_pConsole->Log("User '%d, %s' joined channel '%s'\n", user->GetID(), user->GetUsername().c_str(), channel->m_szName.c_str());

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

	g_pConsole->Log("User '%d, %s' requested room list successfully, sending...\n", user->GetID(), user->GetUsername().c_str());

	g_pPacketManager->SendRoomListFull(user->GetExtendedSocket(), channel->m_Rooms);
}

bool CChannelManager::OnLobbyMessage(CReceivePacket* msg, CExtendedSocket* socket, CUser* user)
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
			g_pConsole->Log(OBFUSCATE("User '%s' tried to send message but he's not in channel\n"), user->GetLogName());
			return false;
		}

		channel->SendLobbyMessageToAllUser(gameName, message);
	}

	return true;
}

bool CChannelManager::OnWhisperMessage(CReceivePacket* msg, CUser* userSender)
{
	if (userSender == NULL)
		return false;

	CExtendedSocket* socket = userSender->GetExtendedSocket();

	string userNameDest = msg->ReadString();
	string message = msg->ReadString();

	CUser* userDest = g_pUserManager->GetUserByNickname(userNameDest);
	if (!userDest)
	{
		// send no user reply
		g_pPacketManager->SendUMsgSystemReply(socket, 1, "MSG_TELL_USER_NOT_FOUND", vector<string>{ userNameDest });
	}
	else if (userDest->GetExtendedSocket() == socket)
	{
		g_pPacketManager->SendUMsgNoticeMessageInChat(socket, "You can't send whisper to yourself.");
		// you can't send whisper message to yourself
	}
	else
	{
		g_pPacketManager->SendUMsgWhisperMessage(socket, message, userNameDest, userSender, 0);
		CUserCharacter character = userSender->GetCharacter(UFLAG_GAMENAME);
		g_pPacketManager->SendUMsgWhisperMessage(userDest->GetExtendedSocket(), message, character.gameName, userDest, 1);
	}

	return true;
}

bool CChannelManager::OnRoomUserMessage(CReceivePacket* msg, CUser* user)
{
	if (user == NULL || user->GetCurrentRoom() == NULL)
		return false;

	user->GetCurrentRoom()->OnUserMessage(msg, user);

	return true;
}

bool CChannelManager::OnRoomTeamUserMessage(CReceivePacket* msg, CUser* user)
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
		if (server->index == index)
			return server;
	}

	return NULL;
}

bool CChannelManager::OnCommandHandler(CExtendedSocket* socket, CUser* user, string message)
{
	vector<string> args = ParseArguments(message.c_str());
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
					g_pNetwork->RemoveSocket(socket);
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
				sprintf_s(buf, sizeof(buf), OBFUSCATE("Server build: %s"), build_number());
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

				CUser* user = g_pUserManager->GetUserById(userID);
				int status = g_pItemManager->AddItem(userID, user, itemID, count, duration); // add permanent item by default
				switch (status)
				{
				case ITEM_ADD_INVENTORY_FULL:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("User's inventory is full"));
					break;
				case ITEM_ADD_UNKNOWN_ITEMID:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Item ID you wrote does not exist in the item database"));
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

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/addallitems"))
			{
				vector<int> items = g_pItemTable->GetColumn<int>(OBFUSCATE("ID"));
				vector<RewardItem> rewardItems;
				CUserInventoryItem userItem;
				for (auto item : items)
				{
					if (userItem.IsItemDefaultOrPseudo(item))
						continue;

					RewardItem rewardItem;
					rewardItem.itemID = item;
					rewardItem.count = 1;
					rewardItem.duration = 0;
					rewardItems.push_back(rewardItem);
				}

				int status = g_pItemManager->AddItems(user->GetID(), user, rewardItems);
				switch (status)
				{
				case ITEM_ADD_INVENTORY_FULL:
					g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Your inventory is full"));
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
				message.erase(0, strlen(OBFUSCATE("/sendnotice ")));

				g_pUserManager->SendNoticeMsgBoxToAll(message);

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
					g_pConsole->Warn("/addexp out of range! %s\n", ex.what());
					return true;
				}

				return true;
			}
			else if (args[0] == (char*)OBFUSCATE("/status"))
			{
				g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, g_pServerInstance->GetMemoryInfo());

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

				CUser* user = g_pUserManager->GetUserById(userID);
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
					for (auto u : g_pUserManager->users)
					{
						g_pPacketManager->SendUMsgNoticeMessageInChat(u->GetExtendedSocket(), va(OBFUSCATE("%s is banned. Reason: %s"), character.gameName.c_str(), reason.c_str()));
					}
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

				CUser* user = g_pUserManager->GetUserById(userID);
				if (user)
				{
					g_pUserManager->DisconnectUser(user);
				}

				CUserData data;
				data.flag |= UDATA_FLAG_LASTHWID;
				g_pUserDatabase->GetUserData(userID, data);

				g_pServerInstance->AddBanHWID(data.lastHWID);

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

				CUser* user = g_pUserManager->GetUserById(userID);
				if (user)
				{
					g_pUserManager->DisconnectUser(user);
				}

				CUserData data;
				data.flag |= UDATA_FLAG_LASTIP;
				g_pUserDatabase->GetUserData(userID, data);

				g_pServerInstance->AddBanIP(data.lastIP);

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

				g_pServerInstance->AddBanIP(data.lastIP, true);
				g_pServerInstance->AddBanHWID(data.lastHWID, true);

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
					IRoomOptions_s options;
					options.roomName = (char*)OBFUSCATE("GHOST ROOM NAME");
					options.gameModeId = 15;
					options.mapId = 128;

					g_pConsole->Error("wtf RoomOptions is useless!\n");

					CRoom* room = channel->CreateRoom(NULL, options);
					CRoomSettings* settings = room->GetSettings();
					settings->status = 0;
					settings->password = (char*)OBFUSCATE("r,=j$b5}a@dgN&^g0_!}['WH}l5i]#ugAhfQ ? dS; Qh1Ckk`R}bz, o[QMgp4]0");
					settings->zsDifficulty = 1;
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

				CUser* userDest = g_pUserManager->GetUserById(userID);
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


bool CChannelManager::OnNewRoomRequest(CReceivePacket* msg, CUser* user)
{
	// don't allow the user to create a new room while in another one
	CRoom* room = user->GetCurrentRoom();
	if (room)
	{
		g_pConsole->Warn("User '%d, %s' tried to create a new room, but he is already playing in other room, curRoomId: %d\n", user->GetID(), user->GetUsername().c_str(), room->GetID());
		return false;
	}

	CChannel* channel = user->GetCurrentChannel();
	if (channel == NULL)
	{
		g_pConsole->Warn("User '%d, %s' tried to create a new room without channel\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	IRoomOptions_s roomCfg;
	roomCfg.roomName = "Test room name";
	roomCfg.gameModeId = 14;
	CRoom* newRoom = channel->CreateRoom(user, roomCfg);

	user->SetCurrentRoom(newRoom);

	OnRoomUpdateSettings(msg, user);

	newRoom->SendJoinNewRoom(user);
	newRoom->SendRoomSettings(user);
	newRoom->SendTeamChange(user, user, (RoomTeamNum)2);

	channel->SendAddRoomToRoomList(newRoom);

	// hide user from channel users list
	channel->UserLeft(user, true);

	g_pConsole->Log("User '%d, %s' created a new room (RID: %d, name: '%s')\n", user->GetID(), user->GetUsername().c_str(), newRoom->GetID(), roomCfg.roomName.c_str());

	return true;
}

bool CChannelManager::OnJoinRoomRequest(CReceivePacket* msg, CUser* user)
{
	int unk = msg->ReadUInt8();
	int roomID = msg->ReadUInt16();
	string password = msg->ReadString();

	CChannel* channel = user->GetCurrentChannel();
	if (channel == NULL)
	{
		g_pConsole->Warn("User '%d, %s' tried to join room (RID: %d) without channel\n", user->GetID(), user->GetUsername().c_str(), roomID);
		return false;
	}

	if (user->GetCurrentRoom() != NULL)
	{
		// don't let user to join room if he in another one
		return false;
	}

	CRoom* room = channel->GetRoomById(roomID);
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

	CUserCharacterExtended hostCharacterExtended = user->GetCharacterExtended(EXT_UFLAG_BANSETTINGS);
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
	room->SendRoomSettings(user);

	user->SetCurrentRoom(room);

	// tell other room members about the new addition
	for (auto u : room->GetUsers())
	{
		room->SendUserReadyStatus(user, u);

		if (u == user)
			continue;

		room->SendNewUser(u, user);
		//room->SendUserReadyStatus(user, u);
		//room->SendTeamChange(u, user, (RoomTeamNum)2);
	}

	// hide user from channel users list
	channel->UserLeft(user, true);

	channel->SendUpdateRoomList(room);

	g_pConsole->Log("User '%d, %s' joined a room (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), room->GetID());

	return true;
}

bool CChannelManager::OnSetTeamRequest(CReceivePacket* msg, CUser* user)
{
	CRoom* currentRoom = user->GetCurrentRoom();
	if (currentRoom == NULL)
	{
		g_pConsole->Warn("User '%d, %s' tried to change room settings, but he isn't in room\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	if (currentRoom->IsUserReady(user))
	{
		g_pConsole->Warn("User '%d, %s' tried to change team in a room, but he is ready\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	int newTeam = msg->ReadUInt8();

	currentRoom->SetUserToTeam(user, (RoomTeamNum)newTeam);

	// inform every user in the room of the changes
	for (auto u : currentRoom->GetUsers())
	{
		currentRoom->SendTeamChange(u, user, (RoomTeamNum)newTeam);
	}

	g_pConsole->Log("User '%d, %s' changed room team to %d (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), newTeam, currentRoom->GetID());

	return true;
}

bool CChannelManager::OnLeaveRoomRequest(CUser* user)
{
	CRoom* currentRoom = user->GetCurrentRoom();
	CChannel* currentChannel = user->GetCurrentChannel();

	if (currentRoom == NULL || currentChannel == NULL)
	{
		g_pConsole->Log(OBFUSCATE("User '%d' tried to leave room without curRoom or curChannel\n"), user->GetID());

		return false;
	}

	if (currentRoom->IsUserIngame(user) == true)
	{
		currentRoom->SendPlayerLeaveIngame(user);
	}

	g_pConsole->Log("User '%d, %s' left a room (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), currentRoom->GetID());

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

bool CChannelManager::OnToggleReadyRequest(CUser* user)
{
	CRoom* room = user->GetCurrentRoom();
	if (room == NULL)
	{
		g_pConsole->Warn("User '%d, %s' tried to toggle ready status\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	RoomReadyStatus readyStatus = room->ToggleUserReadyStatus(user);

	g_pConsole->Log("User '%d, %s' toggled ready status %d\n", user->GetID(), user->GetUsername().c_str(), readyStatus);

	// inform every user in the room of the changes
	for (auto u : room->GetUsers())
	{
		g_pPacketManager->SendRoomSetPlayerReady(u->GetExtendedSocket(), user, readyStatus);
	}

	return true;
}

bool CChannelManager::OnConnectionFailure(CUser* user)
{
	CRoom* room = user->GetCurrentRoom();
	if (room == NULL)
	{
		return false;
	}

	if (room->GetGameMatch() == NULL)
	{
		return false;
	}

	CUser* hostUser = room->GetHostUser();
	if (hostUser == NULL)
	{
		return false;
	}

	CDedicatedServer* server = room->GetServer();

	if (server)
	{
		char ip[INET_ADDRSTRLEN];
		int iIp = server->GetIP();
		inet_ntop(AF_INET, &iIp, ip, sizeof(ip));

		g_pConsole->Log("User '%d, %s' unsuccessfully tried to connect to the game match(%s:%d)\n", user->GetID(), user->GetUsername().c_str(), ip, server->GetPort());

		g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(user->GetExtendedSocket(), "ROOM_JOIN_FAILED_INVALID_GAME_IP");
	}
	else
	{
		g_pConsole->Log("User '%d, %s' unsuccessfully tried to connect to the game match(%s:%d)\n", user->GetID(), user->GetUsername().c_str(), hostUser->GetNetworkConfig().m_szExternalIpAddress.c_str(), hostUser->GetNetworkConfig().m_nExternalServerPort);

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

bool CChannelManager::OnGameStartRequest(CUser* user)
{
	CRoom* currentRoom = user->GetCurrentRoom();
	CChannel* currentChannel = user->GetCurrentChannel();
	if (currentRoom == NULL || currentChannel == NULL)
	{
		g_pConsole->Warn("User '%d, %s' isn't in room or channel but he tried to start room match.\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

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

bool CChannelManager::OnCloseResultRequest(CUser* user)
{
	CRoom* room = user->GetCurrentRoom();
	if (room == NULL)
		return false;

	room->SendCloseResultWindow(user);

	return true;
}

bool CChannelManager::OnRoomUpdateSettings(CReceivePacket* msg, CUser* user)
{
	CPacketHelper_RoomUpdateSettings newSettingsReq(msg->GetData());

	CRoom* currentRoom = user->GetCurrentRoom();
	CChannel* currentChannel = user->GetCurrentChannel();
	if (currentRoom == NULL)
	{
		g_pConsole->Warn("User '%d, %s' tried to update a room\'s settings, although it isn\'t in any\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	if (currentChannel == NULL)
	{
		g_pConsole->Warn("User '%d, %s' tried to update a room\'s settings without current channel\n", user->GetID(), user->GetUsername().c_str());
		return false;
	}

	if (user != currentRoom->GetHostUser())
	{
		g_pConsole->Warn("User '%d, %s' tried to update a room\'s settings, although it isn\'t the host (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), currentRoom->GetID());
		return false;
	}

	CRoomSettings* updatedSettings = currentRoom->UpdateSettings(newSettingsReq);

	// inform every user in the room of the changes
	for (auto u : currentRoom->GetUsers())
	{
		currentRoom->SendUpdateRoomSettings(u, updatedSettings, newSettingsReq.lowFlag, newSettingsReq.lowMidFlag, newSettingsReq.highMidFlag, newSettingsReq.highFlag);
	}

	delete updatedSettings;

	currentChannel->SendUpdateRoomList(currentRoom);

	g_pConsole->Log("Host '%d, %s' updated room settings (RID: %d)\n", user->GetID(), user->GetUsername().c_str(), currentRoom->GetID());

	return true;
}

bool CChannelManager::OnUserInviteRequest(CReceivePacket* msg, CUser* user)
{
	string inviteMsg = msg->ReadString();
	int userCount = msg->ReadUInt16();
	for (int i = 0; i < userCount; i++)
	{
		string userGameName = msg->ReadString();

		CUser* destUser = g_pUserManager->GetUserByNickname(userGameName);
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

bool CChannelManager::OnRoomSetZBAddonRequest(CReceivePacket* msg, CUser* user)
{
	CRoom* currentRoom = user->GetCurrentRoom();
	if (currentRoom == NULL)
	{
		return false;
	}

	CRoomUser* roomUser = user->GetRoomData();
	if (roomUser == NULL)
	{
		return false;
	}

	vector<int> addons;
	int size = msg->ReadUInt16();
	for (int i = 0; i < size; i++)
	{
		addons.push_back(msg->ReadUInt16());
	}

	// TODO: check if addons is real

	roomUser->m_Addons = addons;

	// update addon list on client side
	g_pPacketManager->SendAddonPacket(user->GetExtendedSocket(), addons);

	//g_pPacketManager->SendRoomZBAddonSurvey(user->GetExtendedSocket(), addons);

	return true;
}