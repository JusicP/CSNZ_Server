#include "PacketHelper_FullUserInfo.h"

using namespace std;

CPacketHelper_FullUserInfo::CPacketHelper_FullUserInfo()
{
}

vector<unsigned char> PackAchievementList(vector<int> unpacked)
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
	buf.writeInt32_LE(character.flag);

	if (character.flag & UFLAG_NAMEPLATE)
	{
		buf.writeUInt8(0);
		buf.writeUInt32_LE(character.nameplateID);
	}

	if (character.flag & UFLAG_GAMENAME)
	{
		buf.writeStr(character.gameName);
	}

	if (character.flag & UFLAG_GAMENAME2)
	{
		buf.writeStr(character.gameName);

		buf.writeUInt8(0);
		buf.writeUInt8(0);
		buf.writeUInt8(0);
	}

	if (character.flag & UFLAG_LEVEL)
	{
		buf.writeUInt32_LE(character.level);
	}

	if (character.flag & UFLAG_UNK2)
	{
		buf.writeUInt8(0);
	}

	if (character.flag & UFLAG_EXP)
	{
		buf.writeUInt64_LE(character.exp);
	}

	if (character.flag & UFLAG_CASH)
	{
		buf.writeUInt64_LE(character.cash);
	}

	if (character.flag & UFLAG_POINTS)
	{
		buf.writeUInt64_LE(character.points);
	}

	if (character.flag & UFLAG_STAT)
	{
		buf.writeUInt32_LE(character.battles);
		buf.writeUInt32_LE(character.win);
		buf.writeUInt32_LE(character.kills);
		buf.writeUInt32_LE(character.deaths);
		buf.writeUInt32_LE(0); // zombie kills
		buf.writeUInt32_LE(0); // survivals
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0); // total num of kills
		buf.writeUInt32_LE(0); // total num of death
		buf.writeUInt32_LE(0); // number of CT wins
		buf.writeUInt32_LE(0); // number of T wins
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
		buf.writeUInt32_LE(0);
	}

	if (character.flag & UFLAG_LOCATION)
	{
		buf.writeStr(character.regionName);

		buf.writeUInt16_LE(character.nation);
		buf.writeUInt16_LE(character.city);
		buf.writeUInt16_LE(character.town);

		buf.writeStr(""); // PCBang (PC Cafe) name? If string isn't empty, a network icon will be right beside the player's name
	}

	if (character.flag & UFLAG_CASH2)
	{
		buf.writeUInt32_LE(0); // cash points
	}

	if (character.flag & UFLAG_UNK5)
	{
		buf.writeUInt8(0);
		for (int i = 0; i < 0; i++)
		{
			buf.writeUInt16_LE(0);
			buf.writeUInt8(0);
		}
	}

	if (character.flag & UFLAG_CLAN)
	{
		buf.writeUInt32_LE(character.clanID);
		buf.writeUInt32_LE(character.clanMarkID);
		buf.writeStr(character.clanName);

		buf.writeUInt8(0);
		buf.writeUInt8(0);
		buf.writeUInt8(0);
	}

	if (character.flag & UFLAG_TOURNAMENT)
	{
		buf.writeUInt8(character.tournament);
	}

	if (character.flag & UFLAG_RANK)
	{
		buf.writeUInt8(0xFF); // byte flag

		for (int i = 0; i < 4; i++) // 0 - Original League, 1 - Zombie League, 2 - Zombie PVE League, 3 - Death Match League
		{
			if ((1 << i) & 0xFF)
				buf.writeUInt8(character.tier[i]); // tier
		}

		buf.writeUInt8(character.leagueID); // league
	}

	if (character.flag & UFLAG_UNK4)
	{
		buf.writeUInt8(0);
		buf.writeUInt8(0);
		buf.writeUInt8(0);
	}

	if (character.flag & UFLAG_PASSWORDBOXES)
	{
		buf.writeUInt16_LE(character.passwordBoxes); // кол-во код боксов
	}

	if (character.flag & UFLAG_UNK11)
	{
		buf.writeUInt32_LE(0);
	}

	if (character.flag & UFLAG_ACHIEVEMENT)
	{
		buf.writeUInt16_LE(character.prefixId); // айди префикса перед ником (можно найти в title.csv)
		buf.writeUInt8(0);
		buf.writeUInt8(0);
		buf.writeUInt32_LE(character.honorPoints); // кол-во очков чести
	}

	if (character.flag & UFLAG_ACHIEVEMENTLIST)
	{
		vector<int> list = character.achievementList;
		sort(list.begin(), list.end());
		buf.writeArray(PackAchievementList(character.achievementList));
	}

	if (character.flag & UFLAG_UNK13)
	{
		buf.writeUInt16_LE(0);
		buf.writeUInt16_LE(0);
		buf.writeUInt16_LE(0);
		buf.writeUInt16_LE(0);
		buf.writeUInt16_LE(0);
	}

	if (character.flag & UFLAG_UNK14)
	{
		buf.writeUInt8(0);
		buf.writeUInt32_LE(0);
	}

	if (character.flag & UFLAG_TITLES)
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

	if (character.flag & UFLAG_UNK16)
	{
		buf.writeUInt8(0);
	}

	if (character.flag & UFLAG_UNK18)
	{
		buf.writeUInt16_LE(0);
	}

	if (character.flag & UFLAG_UNK19)
	{
		buf.writeUInt32_LE(character.mileagePoints); 
		buf.writeUInt32_LE(0); // achievement points
	}

	if (character.flag & UFLAG_UNK20)
	{
		buf.writeUInt32_LE(userID); // suid TODO: what is it?
	}

	if (character.flag & UFLAG_UNK21)
	{
		buf.writeUInt8(0);
		for (int i = 0; i < 0; i++)
		{
			buf.writeUInt8(0);
			buf.writeUInt32_LE(0);
		}
	}

	if (character.flag & UFLAG_UNK22)
	{
		buf.writeUInt8(0);
		for (int i = 0; i < 0; i++)
			buf.writeUInt16_LE(0);
	}

	if (character.flag & UFLAG_UNK23)
	{
		buf.writeUInt8(0);
		buf.writeUInt8(0);
	}

	if (character.flag & UFLAG_UNK24)
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
}