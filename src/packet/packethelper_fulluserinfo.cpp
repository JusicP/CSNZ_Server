#include "packethelper_fulluserinfo.h"

using namespace std;

CPacketHelper_FullUserInfo::CPacketHelper_FullUserInfo()
{
}

vector<unsigned char> PackAchievementList(const vector<int>& unpacked)
{
	vector<unsigned char> packed;

	int nextIdx = 0;
	for (int i = 0; i < 128; i++)
	{
		int v28 = i * 8;
		unsigned char pack = 0;
		for (int j = 0; j < 8; j++)
		{
			if ((int)unpacked.size() <= nextIdx)
			{
				packed.push_back(pack);
				if (packed.size() != 128)
				{
					packed.insert(packed.end(), 128 - packed.size(), 0);
				}
				return packed;
			}

			if (v28 && v28 == unpacked[nextIdx])
			{
				nextIdx++;
				pack |= 1 << j;
			}
			v28++;
		}
		packed.push_back(pack);
	}

	return packed;
}

void CPacketHelper_FullUserInfo::Build(Buffer& buf, int userID, const CUserCharacter& character)
{
	buf.writeInt32_LE(character.lowFlag);
	buf.writeInt32_LE(character.highFlag);

	if (character.lowFlag & UFLAG_LOW_NAMEPLATE)
	{
		buf.writeUInt8(0);
		buf.writeUInt32_LE(character.nameplateID);
	}

	if (character.lowFlag & UFLAG_LOW_GAMENAME)
	{
		buf.writeStr(character.gameName);
		buf.writeStr(character.gameName);
		buf.writeStr(character.gameName);
	}

	if (character.lowFlag & UFLAG_LOW_GAMENAME2)
	{
		buf.writeStr(character.gameName);
		buf.writeStr(character.gameName);
		buf.writeStr(character.gameName);

		buf.writeUInt8(0);
		buf.writeUInt8(0);
		buf.writeUInt8(0);
	}

	if (character.lowFlag & UFLAG_LOW_LEVEL)
	{
		buf.writeUInt32_LE(character.level);
	}

	if (character.lowFlag & UFLAG_LOW_UNK4)
	{
		buf.writeUInt8(0);
	}

	if (character.lowFlag & UFLAG_LOW_EXP)
	{
		buf.writeUInt64_LE(character.exp);
	}

	if (character.lowFlag & UFLAG_LOW_CASH)
	{
		buf.writeUInt64_LE(character.cash);
	}

	if (character.lowFlag & UFLAG_LOW_POINTS)
	{
		buf.writeUInt64_LE(character.points);
	}

	if (character.lowFlag & UFLAG_LOW_STAT)
	{
		buf.writeUInt32_LE(character.battles);
		buf.writeUInt32_LE(character.win);
		buf.writeUInt32_LE(character.kills);
		buf.writeUInt32_LE(character.deaths);
		buf.writeUInt32_LE(0); // zombie kills
		buf.writeUInt32_LE(0); // survivals
		buf.writeUInt32_LE(0); // total zombie kills
		buf.writeUInt32_LE(0); // total infect count
		buf.writeUInt32_LE(0); // avg round damage
		buf.writeUInt32_LE(0); // avg infect count
		buf.writeUInt32_LE(0); // total num of kills
		buf.writeUInt32_LE(0); // total num of death
		buf.writeUInt32_LE(0); // number of CT wins
		buf.writeUInt32_LE(0); // number of T wins
		buf.writeUInt32_LE(0); // ZombieTendencyTypeA
		buf.writeUInt32_LE(0); // weapon
		buf.writeUInt32_LE(0); // ZombieTendencyTypeC
		buf.writeUInt32_LE(0); // OriginalTendencyTypeA
		buf.writeUInt32_LE(0); // weapon
		buf.writeUInt32_LE(0); // OriginalTendencyTypeC
	}

	if (character.lowFlag & UFLAG_LOW_LOCATION)
	{
		buf.writeStr(character.regionName);

		buf.writeUInt16_LE(character.nation);
		buf.writeUInt16_LE(character.city);
		buf.writeUInt16_LE(character.town);

		buf.writeStr(""); // PCBang (PC Cafe) name? If string isn't empty, a network icon will be right beside the player's name
	}

	if (character.lowFlag & UFLAG_LOW_CASH2)
	{
		buf.writeUInt32_LE(0); // cash points
	}

	if (character.lowFlag & UFLAG_LOW_UNK11)
	{
		buf.writeUInt8(0);
		for (int i = 0; i < 0; i++)
		{
			buf.writeUInt16_LE(0);
			buf.writeUInt8(0);
		}
	}

	if (character.lowFlag & UFLAG_LOW_CLAN)
	{
		buf.writeUInt32_LE(character.clanID);
		buf.writeUInt32_LE(character.clanMarkID);
		buf.writeStr(character.clanName);

		buf.writeUInt8(0);
		buf.writeUInt8(0);
		buf.writeUInt8(0);
	}

	if (character.lowFlag & UFLAG_LOW_TOURNAMENT)
	{
		buf.writeUInt8(character.tournament);
	}

	if (character.lowFlag & UFLAG_LOW_RANK)
	{
		buf.writeUInt8(0xFF); // byte flag

		for (int i = 0; i < 4; i++) // 0 - Original League, 1 - Zombie League, 2 - Zombie PVE League, 3 - Death Match League
		{
			if ((1 << i) & 0xFF)
				buf.writeUInt8(character.tier[i]); // tier
		}

		buf.writeUInt8(character.leagueID); // league
	}

	if (character.lowFlag & UFLAG_LOW_UNK15)
	{
		buf.writeUInt8(0);
		buf.writeUInt8(0);
	}

	if (character.lowFlag & UFLAG_LOW_PASSWORDBOXES)
	{
		buf.writeUInt16_LE(character.passwordBoxes); // кол-во код боксов
	}

	if (character.lowFlag & UFLAG_LOW_UNK17)
	{
		buf.writeUInt32_LE(0);
	}

	if (character.lowFlag & UFLAG_LOW_ACHIEVEMENT)
	{
		buf.writeUInt16_LE(character.prefixId); // айди префикса перед ником (можно найти в title.csv)
		buf.writeUInt8(0);
		buf.writeUInt8(0);
		buf.writeUInt32_LE(character.honorPoints); // кол-во очков чести
	}

	if (character.lowFlag & UFLAG_LOW_ACHIEVEMENTLIST)
	{
		vector<int> list = character.achievementList;
		sort(list.begin(), list.end());
		buf.writeArray(PackAchievementList(character.achievementList));
	}

	if (character.lowFlag & UFLAG_LOW_UNK20)
	{
		buf.writeUInt16_LE(0);
		buf.writeUInt16_LE(0);
		buf.writeUInt16_LE(0);
		buf.writeUInt16_LE(0);
		buf.writeUInt16_LE(0);
	}

	if (character.lowFlag & UFLAG_LOW_UNK21)
	{
		buf.writeUInt8(0);
		buf.writeUInt32_LE(0);
	}

	if (character.lowFlag & UFLAG_LOW_TITLES)
	{
		// TODO: what is it?
		if (character.titles.size() == 5)
		{
			for (auto titleID : character.titles)
				buf.writeUInt16_LE(titleID);
		}
		else
		{
			for (int i = 0; i < 5; i++)
				buf.writeUInt16_LE(0);
		}
	}

	if (character.lowFlag & UFLAG_LOW_UNK23)
	{
		buf.writeUInt8(0);
	}

	if (character.lowFlag & UFLAG_LOW_UNK24)
	{
		buf.writeUInt8(0);
	}

	if (character.lowFlag & UFLAG_LOW_UNK25)
	{
		buf.writeUInt16_LE(0);
	}

	if (character.lowFlag & UFLAG_LOW_UNK26)
	{
		buf.writeUInt32_LE(character.mileagePoints); 
		buf.writeUInt32_LE(0); // achievement points
	}

	if (character.lowFlag & UFLAG_LOW_UNK27)
	{
		buf.writeUInt32_LE(userID); // suid TODO: what is it?
	}

	if (character.lowFlag & UFLAG_LOW_UNK28)
	{
		buf.writeUInt8(0);
		for (int i = 0; i < 0; i++)
		{
			buf.writeUInt8(0);
			buf.writeUInt32_LE(0);
		}
	}

	if (character.lowFlag & UFLAG_LOW_UNK29)
	{
		buf.writeUInt8(0);
		for (int i = 0; i < 0; i++)
			buf.writeUInt16_LE(0);
	}

	if (character.lowFlag & UFLAG_LOW_UNK30)
	{
		buf.writeUInt8(0);
		buf.writeUInt8(0);
	}

	if (character.lowFlag & UFLAG_LOW_UNK31)
	{
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt8(0);
		buf.writeUInt8(0);
	}

	if (character.highFlag & UFLAG_HIGH_CHATCOLOR)
	{
		buf.writeInt16_LE(character.chatColorID);
	}

	if (character.highFlag & UFLAG_HIGH_UNK2)
	{
		buf.writeInt8(0);
	}
}