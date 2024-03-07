#include "rankmanager.h"
#include "usermanager.h"
#include "packetmanager.h"
#include "userdatabase.h"

using namespace std;

CRankManager::CRankManager() : CBaseManager("RankManager")
{
}

bool CRankManager::OnRankPacket(CReceivePacket* msg, IExtendedSocket* socket)
{
	LOG_PACKET;

#ifdef PUBLIC_RELEASE
	g_pPacketManager->SendUMsgNoticeMsgBoxToUuid(socket, OBFUSCATE("Rank system is not implemented"));
#else
	IUser* user = g_pUserManager->GetUserBySocket(socket);
	if (user == NULL)
		return false;

	int type = msg->ReadUInt8();
	switch (type)
	{
	case RankPacketType::RequestRankInfo:
		//OnRankInfoRequest(msg, user);
		break;
	case RankPacketType::RequestRankInRoom:
		//OnRankInRoomRequest(msg, user);
		break;
	case RankPacketType::RequestRankSearchNickname:
		//OnRankSearchNicknameRequest(msg, user);
		break;
	case RankPacketType::RequestRankLeague:
	{
		//OnRankLeagueRequest(msg, user);

		/*auto msg = g_pPacketManager->CreatePacket(socket, PacketId::Rank);
		msg->BuildHeader();
		msg->WriteUInt8(3);

		msg->WriteUInt8(2); // a2 | v23[0] = a2[0]
		msg->WriteUInt8(0); // v12 | v23[1] = v12
		msg->WriteString("test 123"); // v10
		msg->WriteUInt32(1); // v7 | v24[3] = v7
		// if ( v7 )
		// {
			msg->WriteUInt32(0); // v6 | v24[4] = v6
			msg->WriteUInt8(50); // v5 | v20 = v5[0]
			for (int j = 0; j < 50; ++j) // for ( j = 0; j < v20; ++j )
			{
				msg->WriteString("test 1"); // v20
				msg->WriteUInt8(1); // a2 | (a3 + 12) = a2[0]
				msg->WriteString("test 2"); // v15
				msg->WriteUInt32(2); // v12 | (a3 + 28) = v12
				msg->WriteUInt32(3); // v11 | (a3 + 32) = v11
				// if ( arg4 == 2 ) | arg4 is v23[0]
				// {
					msg->WriteUInt32(4); // v10 | (a3 + 36) = v10
					msg->WriteUInt8(5); // v9 | (a3 + 40) = v9
				// }
				// else
				// {
					//msg->WriteUInt32(0); // v8 | (a3 + 36) = v8
					//msg->WriteUInt32(0); // v7 | (a3 + 40) = v7
				// }
				msg->WriteUInt8(6); // v6 | (a3 + 44) = v6[0]
			}
		// }
		socket->Send(msg);*/

		/*msg->WriteUInt8(1); // a2 | v31[0] = a2[0]
		msg->WriteUInt32(1); // v19 | v32 = v19
		// if ( v19 )
		// {
			msg->WriteUInt32(1); // v18 | v33 = v18
			msg->WriteUInt8(18); // v17 | v28 = v17[0]
			for (int i = 0; i < 18; i++) // for ( i = 0; i < v28; ++i )
			{
				msg->WriteUInt32(1); // v16 | v24 = v16
				msg->WriteString("111111111"); // v14
				msg->WriteUInt8(1); // v11 | v26 = v11[0]
				msg->WriteString("222222222"); // v9 | v10 = v9
				msg->WriteUInt32(9999); // v5 | v27[3] = v5
			}
		// }
		socket->Send(msg);*/
		break;
	}
	case 5: // switch page
	{
		//int unk = msg->ReadUInt8();
		//int unk2 = msg->ReadUInt32();
		//int unk3 = msg->ReadUInt8();
		//int unk4 = msg->ReadUInt8();

		break;
	}
	case 6: // search by nickname
	{
		//int unk = msg->ReadUInt8();
		//std::string unk2 = msg->ReadString();
		//int unk3 = msg->ReadUInt8();
		//int unk4 = msg->ReadUInt8();

		break;
	}
	case RankPacketType::RequestRankLeagueHallOfFame:
	{
		//int leagueid = msg->ReadUInt8();
		//int page = msg->ReadUInt32();
		//int unk = msg->ReadUInt8();

		//Console().Log(OBFUSCATE("[User '%s'] Packet_Rank type %d leagueid: %d page: %d unk: %d\n"), user->GetLogName(), type, leagueid, page, unk);

		//auto msg = g_pPacketManager->CreatePacket(socket, PacketId::Rank);
		//msg->BuildHeader();
		//msg->WriteUInt8(RankPacketType::RequestRankLeagueHallOfFame);
		//msg->WriteUInt8(1); // a2 | v31[0] = a2[0]
		//msg->WriteUInt32(1); // v20 | v32 = v20
		// if ( v20 )
		//// {
		//msg->WriteUInt32(1); // v19 | v33 = v19
		//msg->WriteUInt8(18); // v18 | v29 = v18[0]
		//for (int i = 0; i < 18; i++) // for ( i = 0; i < v29; ++i )
		//{
		//	msg->WriteString("111111111"); // v16
		//	msg->WriteUInt32(1); // v13 | v25 = v13
		//	msg->WriteUInt32(1); // v12 | v26 = v12
		//	msg->WriteUInt16(1); // v11 | v27[0] = v11
		//	msg->WriteUInt16(1); // v10 | v27[1] = v10
		//	msg->WriteUInt16(1); // v9 | v27[2] = v9
		//	msg->WriteUInt16(1); // v8 | v27[3] = v8
		//	msg->WriteUInt16(1); // v7 | v27[4] = v7
		//	msg->WriteUInt16(1); // v6 | v28 = v6
		//}
		// }
		//socket->Send(msg);
		break;
	}
	case 8:
	{
		//int unk = msg->ReadUInt8();
		//std::string unk2 = msg->ReadString();
		//int unk3 = msg->ReadUInt8();

		break;
	}
	case RankPacketType::RankUserInfo:
		OnRankUserInfoRequest(msg, user);
		break;
	case 10: // request clan ranking
	{
		//int clanID = msg->ReadUInt32();
		//int unk = msg->ReadUInt8();
		break;
	}
	case 11:
	{
		//int unk = msg->ReadUInt8();
		//int unk2 = msg->ReadUInt32();
		//std::string unk3 = msg->ReadString();
		//int unk4 = msg->ReadUInt8();
		break;
	}
	case 12:
	{
		//int unk = msg->ReadUInt8();
		//int unk2 = msg->ReadUInt32();
		//int unk3 = msg->ReadUInt32();
		//int unk4 = msg->ReadUInt8();
		break;
	}
	case 13:
	{
		//int unk = msg->ReadUInt8();
		//int unk2 = msg->ReadUInt32();
		//int unk3 = msg->ReadUInt32();
		//int unk4 = msg->ReadUInt8();
		break;
	}
	case 14:
	{
		//std::string unk = msg->ReadString();
		//int unk2 = msg->ReadUInt32();
		//int unk3 = msg->ReadUInt8();
	}
	case 15:
	{
		//std::string unk = msg->ReadString();
		//std::string unk2 = msg->ReadString();
		//int unk3 = msg->ReadUInt8();
	}
	case RankPacketType::RankChangeLeague:
	{
		//int unk = msg->ReadUInt8();
		//int leagueID = msg->ReadUInt8();

		//Console().Log(OBFUSCATE("[User '%s'] Packet_Rank type %d\n"), user->GetLogName(), type);

		//user->UpdateRank(leagueID);

		//auto msg = g_pPacketManager->CreatePacket(socket, PacketId::Rank);
		//msg->BuildHeader();
		//msg->WriteUInt8(RankPacketType::RankChangeLeague);
		//msg->WriteUInt8(0xFF); // a2 | v8 = a2[0]
		//msg->WriteUInt8(leagueID); // v4 | v7 = v4[0]
		//socket->Send(msg);
		break;
	}
	default:
		Console().Log(OBFUSCATE("[User '%s'] Unknown Packet_Rank type %d\n"), user->GetLogName(), type);
		break;
	}
#endif
	return true;
}

bool CRankManager::OnRankInfoRequest(CReceivePacket* msg, IUser* user)
{
	std::string userName = msg->ReadString();

	CUserCharacter character = {};
	character.flag = UFLAG_RANK;
	if (g_pUserDatabase->GetCharacter(user->GetID(), character) <= 0)
	{
		// TODO: send failed reply
		return false;
	}

	//g_pPacketManager->SendRankInfo(user->GetExtendedSocket(), character);

	return true;
}

bool CRankManager::OnRankInRoomRequest(CReceivePacket* msg, IUser* user)
{
	int userID = msg->ReadUInt8();

	CUserCharacter character = {};
	character.flag = UFLAG_RANK;
	if (g_pUserDatabase->GetCharacter(userID, character) <= 0)
	{
		// TODO: send failed reply
		return false;
	}

	//g_pPacketManager->SendRankInfo(user->GetExtendedSocket(), character);

	return true;
}

bool CRankManager::OnRankSearchNicknameRequest(CReceivePacket* msg, IUser* user)
{
	std::string nickname = msg->ReadString();

	int userID = g_pUserDatabase->IsUserExists(nickname, false);
	if (!userID)
	{
		g_pPacketManager->SendRankReply(user->GetExtendedSocket(), RankReplyCode::RankNotFound);
		return false;
	}

	CUserCharacter character = {};
	character.flag = UFLAG_RANK;
	if (g_pUserDatabase->GetCharacter(userID, character) <= 0)
	{
		// TODO: send failed reply
		return false;
	}

	//g_pPacketManager->SendRankInfo(user->GetExtendedSocket(), character);

	return true;
}

bool CRankManager::OnRankLeagueRequest(CReceivePacket* msg, IUser* user)
{
	//g_pPacketManager->SendRankLeague(user->GetExtendedSocket());

	return true;
}

bool CRankManager::OnRankUserInfoRequest(CReceivePacket* msg, IUser* user)
{
	int userID = msg->ReadUInt32();

	CUserCharacter character = {};
	character.flag = -1;
	if (g_pUserDatabase->GetCharacter(userID, character) <= 0)
	{
		g_pPacketManager->SendRankReply(user->GetExtendedSocket(), RankReplyCode::RankErrorData);
		return false;
	}

	g_pPacketManager->SendRankUserInfo(user->GetExtendedSocket(), userID, character);

	return true;
}